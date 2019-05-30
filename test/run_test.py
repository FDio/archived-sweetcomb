#!/usr/bin/env python3
#
#  Copyright (c) 2019 PANTHEON.tech.
#  Copyright (c) 2019 Cisco and/or its affiliates.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#

import unittest
import util
import argparse
import os
import importlib
import sys
import fnmatch


from framework import SweetcombTestCase, SweetcombTestRunner


class SplitToSuitesCallback:
    def __init__(self):
        self.suites = {}
        self.suite_name = 'default'

    def __call__(self, file_name, cls, method):
        test_method = cls(method)

        self.suite_name = file_name + cls.__name__
        if self.suite_name not in self.suites:
            self.suites[self.suite_name] = unittest.TestSuite()
        self.suites[self.suite_name].addTest(test_method)


def discover_tests(directory, callback, ignore_path):
    do_insert = True
    for _f in os.listdir(directory):
        f = "%s/%s" % (directory, _f)
        if os.path.isdir(f):
            if ignore_path is not None and f.startswith(ignore_path):
                continue
            discover_tests(f, callback, ignore_path)
            continue
        if not os.path.isfile(f):
            continue
        if do_insert:
            sys.path.insert(0, directory)
            do_insert = False
        if not _f.startswith("test_") or not _f.endswith(".py"):
            continue
        name = "".join(f.split("/")[-1].split(".")[:-1])
        module = importlib.import_module(name)
        for name, cls in module.__dict__.items():
            if not isinstance(cls, type):
                continue
            if not issubclass(cls, unittest.TestCase):
                continue
            if name == "SweetcombTestCase" or name.startswith("Template"):
                continue

            for method in dir(cls):
                if not callable(getattr(cls, method)):
                    continue
                if method.startswith("test_"):
                    callback(_f, cls, method)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Sweetcomb tests")
    parser.add_argument("-d", "--dir", action='append', type=str,
                        help="directory containing test files "
                             "(may be specified multiple times)")
    args = parser.parse_args()

    util.import_yang_modules()

    ddir = list()
    if args.dir is None:
        ddir.append(os.getcwd())
    else:
        ddir = args.dir

    cb = SplitToSuitesCallback()

    ignore_path = 'conf'
    for d in ddir:
        print("Adding tests from directory tree {}".format(d))
        discover_tests(d, cb, ignore_path)

    suites = []
    for testcase_suite in cb.suites.values():
        suites.append(testcase_suite)

    full_suite = unittest.TestSuite()
    #map(full_suite.addTests, suites)
    for suite in suites:
        full_suite.addTests(suite)
    result = SweetcombTestRunner(print_summary=True).run(full_suite)


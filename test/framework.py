#
# Copyright (c) 2019 PANTHEON.tech.
# Copyright (c) 2019 Cisco and/or its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import unittest

from topology import Topology
import vppctl
import sys
import log


class SweetcombTestCase(unittest.TestCase):

    @classmethod
    def instance(cls):
        """Return the instance of this testcase"""

        return cls.instance

    @classmethod
    def create_topology(cls, debug=False):

        cls.topology = Topology()
        cls.topology.create_topology(debug)

        cls.vpp = cls.topology.get_vpp()
        cls.netopeer_cli = cls.topology.get_netopeer_cli()
        cls.vppctl = vppctl.Vppctl()

    @classmethod
    def setUpClass(cls):

        super(SweetcombTestCase, cls).setUpClass()
        cls.logger = log.get_logger(cls.__name__)

    def runTest(self):
        pass


class SweetcombTestResult(unittest.TestResult):

    def __init__(self, stream=None, descriptions=None, verbosity=None,
                 runner=None):
        """
        :param stream File descriptor to store where to report test results.
            Set to the standard error stream by default.
        :param descriptions Boolean variable to store information if to use
            test case descriptions.
        :param verbosity Integer variable to store required verbosity level.
        """
        super(SweetcombTestResult, self).__init__(stream, descriptions, verbosity)
        self.stream = stream
        self.descriptions = descriptions
        self.verbosity = verbosity
        self.result_string = None
        self.runner = runner


class SweetcombTestRunner(unittest.TextTestRunner):
    """
    A basic test runner implementation which prints results to standard error.
    """

    @property
    def resultclass(self):
        return SweetcombTestResult

    def __init__(self, keep_alive_pipe=None, descriptions=True, verbosity=1,
                 result_pipe=None, failfast=False, buffer=False,
                 resultclass=None, print_summary=True, **kwargs):
        # ignore stream setting here, use hard-coded stdout to be in sync
        # with prints from VppTestCase methods ...
        super(SweetcombTestRunner, self).__init__(sys.stdout, descriptions,
                                            verbosity, failfast, buffer,
                                            resultclass, **kwargs)
        #KeepAliveReporter.pipe = keep_alive_pipe

        self.orig_stream = self.stream
        self.resultclass.test_framework_result_pipe = result_pipe

        self.print_summary = print_summary

    def _makeResult(self):
        return self.resultclass(self.stream,
                                self.descriptions,
                                self.verbosity,
                                self)

    def run(self, test):
        """
        Run the tests

        :param test:

        """

        result = super(SweetcombTestRunner, self).run(test)
        if not self.print_summary:
            self.stream = self.orig_stream
            result.stream = self.orig_stream
        return result



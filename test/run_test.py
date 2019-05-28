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

from framework import SweetcombTestCase
from test_ietf_interfaces import TestIetfInterfaces
from test_oc_interfaces import TestOcInterfaces


def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestIetfInterfaces('test_ipv4'))
    suite.addTest(TestIetfInterfaces('test_interface'))
    suite.addTest(TestOcInterfaces('test_interface'))
    suite.addTest(TestOcInterfaces('test_interface_ipv4'))
    return suite


if __name__ == '__main__':
    util.import_yang_modules()
    runner = unittest.TextTestRunner()
    runner.run(suite())


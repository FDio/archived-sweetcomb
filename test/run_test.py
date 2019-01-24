#!/usr/bin/env python3
#
#  Copyright (c) 2019 PANTHEON.tech.
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

from framewrok import SweetcombTestCase
from test_ietf_interfaces import TestIetfInterfaces

def suite():
    suite = unittest.TestSuite()
    suite.addTest(TestIetfInterfaces('test_interface_up'))
    suite.addTest(TestIetfInterfaces('test_ip_addr'))
    return suite

if __name__ == '__main__':
    util.import_yang_modules()
    runner = unittest.TextTestRunner()
    runner.run(suite())


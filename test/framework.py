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

    def check_response(self, resps, expected_result, checks):
        assert resps[1] == expected_result

        for key, val in checks.items():
            for resp in resps:
                r = str(resp).strip()
                if r.find("<"+key+">") == 0:
                    assert r[r.find("<"+key+">")+len("<"+key+">"):r.rfind("</"+key+">")] == val

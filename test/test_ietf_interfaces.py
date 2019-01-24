#
# Copyright (c) 2019 PANTHEON.tech.
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

import util
from framewrok import SweetcombTestCase

class TestIetfInterfaces(SweetcombTestCase):

    def setUp(self):
        super(TestIetfInterfaces, self).setUp()

        self.create_topoly()

    def tearDown(self):
        super(TestIetfInterfaces, self).setUp()

        self.topology.close_topology()

    def test_interface_up(self):

        self.vpp.show_interface()

        self.netopeer_cli.get("/ietf-interfaces:*")
        ts = '''<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">
  <interface>
    <name>local0</name>
    <enabled>true</enabled>
  </interface>
  <interface>
    <name>host-vpp1</name>
    <enabled>true</enabled>
  </interface>
</interfaces>'''

        self.netopeer_cli.edit_config(ts)
        self.netopeer_cli.get("/ietf-interfaces:*")

        self.vpp.show_interface()

    def test_ip_addr(self):
        self.vpp.show_address()
        self.netopeer_cli.get("/ietf-interfaces:*")

        util.ping('192.168.0.1')

        ts = '''<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\" xmlns:ip=\"urn:ietf:params:xml:ns:yang:ietf-ip\">
  <interface>
    <name>host-vpp1</name>
    <enabled>true</enabled>
        <ip:ipv4>
            <ip:enabled>true</ip:enabled>
            <ip:address>
              <ip:ip>192.168.0.1</ip:ip>
              <ip:prefix-length>24</ip:prefix-length>
            </ip:address>
        </ip:ipv4>
  </interface>
</interfaces>'''

        self.netopeer_cli.edit_config(ts)
        self.netopeer_cli.get("/ietf-interfaces:*")

        self.vpp.show_address()

        util.ping('192.168.0.1')

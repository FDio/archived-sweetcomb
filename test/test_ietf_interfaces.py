#!/usr/bin/env python3
#
# Copyright (c) 2019 PANTHEON.tech.
# Copyright (c) 2019 Cisco and/or its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import unittest

import util
from framework import SweetcombTestCase, SweetcombTestRunner
from ydk.models.ietf import ietf_interfaces
from ydk.models.ietf import iana_if_type
from ydk.services import CRUDService
from ydk.errors import YError


class TestIetfInterfaces(SweetcombTestCase):

    def setUp(self):
        super(TestIetfInterfaces, self).setUp()

        self.create_topology()

    def tearDown(self):
        super(TestIetfInterfaces, self).setUp()

        self.topology.close_topology()

    def test_interface(self):

        self.logger.info("IETF_INTERFACE_TEST_START_001")

        name = "host-vpp1"
        crud_service = CRUDService()

        interface = ietf_interfaces.Interfaces.Interface()
        interface.name = name
        interface.type = iana_if_type.EthernetCsmacd()
        interface.enabled = True
        interface.ipv4 = interface.Ipv4()
        interface.ipv4.mtu = 1500

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)

        self.assertEquals(interface.enabled, p.State)
        #FIXME: MTU assert
        #self.assertEquals(interface.ipv4.mtu, p.MTU)

        interface.enabled = False

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)
        self.assertEquals(interface.enabled, p.State)

        self.logger.info("IETF_INTERFACE_TEST_FINISH_001")

    def test_ipv4(self):

        self.logger.info("IETF_INTERFACE_TEST_START_002")

        name = "host-vpp1"
        crud_service = CRUDService()

        interface = ietf_interfaces.Interfaces.Interface()
        interface.name = name
        interface.type = iana_if_type.EthernetCsmacd()
        interface.ipv4 = interface.Ipv4()
        addr = interface.Ipv4().Address()
        addr.ip = "192.168.0.1"
        addr.prefix_length = 24
        interface.ipv4.address.append(addr)
        addr1 = interface.Ipv4().Address()
        addr1.ip = "142.168.0.1"
        addr1.prefix_length = 14
        interface.ipv4.address.append(addr1)

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        a = self.vppctl.show_address(name)
        self.assertIsNotNone(a)

        prefix = interface.ipv4.address[0].ip + "/" + \
                                str(interface.ipv4.address[0].prefix_length)
        self.assertIn(prefix, a.addr)

        prefix = interface.ipv4.address[1].ip + "/" + \
                                str(interface.ipv4.address[1].prefix_length)
        self.assertIn(prefix, a.addr)

        try:
            crud_service.delete(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        a = self.vppctl.show_address(name)

        self.assertIsNone(a)

        self.logger.info("IETF_INTERFACE_TEST_FINISH_002")


if __name__ == '__main__':
    unittest.main(testRunner=SweetcombTestRunner)

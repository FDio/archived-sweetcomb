#!/usr/bin/env python3
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

import util
from framework import SweetcombTestCase, SweetcombTestRunner
from ydk.models.openconfig import openconfig_interfaces
from ydk.models.ietf import iana_if_type
from ydk.services import CRUDService
from ydk.errors import YError


class TestOcInterfaces(SweetcombTestCase):
    cleanup = False

    def setUp(self):
        super(TestOcInterfaces, self).setUp()
        self.create_topology()

    def tearDown(self):
        super(TestOcInterfaces, self).setUp()

        self.topology.close_topology()

    def test_interface(self):
        name = "host-vpp1"
        crud_service = CRUDService()

        interface = openconfig_interfaces.Interfaces.Interface()
        interface.name = name
        interface.config.type = iana_if_type.EthernetCsmacd()
        interface.config.enabled = True

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)
        self.assertEquals(interface.config.enabled, p.State)

        interface.config.enabled = False

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)
        self.assertEquals(interface.config.enabled, p.State)

    @unittest.skip("YDK return error when try set IP address")
    def test_interface_ipv4(self):
        name = "host-vpp1"
        crud_service = CRUDService()

        interface = openconfig_interfaces.Interfaces.Interface()
        interface.name = name
        subinterface = interface.Subinterfaces.Subinterface()

        addr = subinterface.ipv4.addresses.Address()
        addr.ip = "10.0.0.2"
        addr.config = addr.Config()
        # FIXME It return error, why ??????????????????????
        addr.config.ip = "10.0.0.2"
        addr.config.prefix_length = 24
        subinterface.index = 0
        subinterface.ipv4.addresses.address.append(addr)
        interface.subinterfaces.subinterface.append(subinterface)

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        a = self.vppctl.show_address(name)

if __name__ == '__main__':
    unittest.main(testRunner=SweetcombTestRunner)

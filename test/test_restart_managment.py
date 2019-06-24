#!/usr/bin/env python3
#
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
import time
from framework import SweetcombTestCase, SweetcombTestRunner
from ydk.models.ietf import ietf_interfaces
from ydk.models.ietf import iana_if_type
from ydk.services import CRUDService
from ydk.errors import YError

import os
import psutil


class TestRestartManagment(SweetcombTestCase):
    def setUp(self):
        super(TestRestartManagment, self).setUp()

        self.create_topology()

    def tearDown(self):
        super(TestRestartManagment, self).setUp()

        self.topology.close_topology()

    def test_vpp_restart(self):

        self.logger.info("RESTART_VPP_START_001")

        name = "vpp1"
        crud_service = CRUDService()

        interface = ietf_interfaces.Interfaces.Interface()
        interface.name = name
        interface.type = iana_if_type.EthernetCsmacd()
        interface.enabled = True
        interface.ipv4 = interface.Ipv4()
        interface.ipv4.mtu = 1500
        addr = interface.Ipv4().Address()
        addr.ip = "192.168.0.1"
        addr.prefix_length = 24
        interface.ipv4.address.append(addr)

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)

        self.assertEquals(interface.enabled, p.State)

        a = self.vppctl.show_address(name)
        self.assertIsNotNone(a)

        prefix = interface.ipv4.address[0].ip + "/" + \
            str(interface.ipv4.address[0].prefix_length)
        self.assertIn(prefix, a.addr)

        #print("Kill VPP")
        self.vpp.kill()

        for proc in psutil.process_iter(attrs=['pid', 'name']):
            pname = proc.info['name']
            if 'vpp' in pname:
                proc.kill()

        time.sleep(2)

        #print("Start VPP")
        self.topology.start_vpp()

        #print("Wait 7 s")
        time.sleep(7)
        #print("Stop wait")

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)

        self.assertEquals(interface.enabled, p.State)

        a = self.vppctl.show_address(name)
        self.assertIsNotNone(a)

        prefix = interface.ipv4.address[0].ip + "/" + \
            str(interface.ipv4.address[0].prefix_length)
        self.assertIn(prefix, a.addr)

        self.logger.info("RESTART_VPP_FINISH_001")

    def test_sweetcomb_restart(self):

        self.logger.info("RESTART_VPP_START_002")

        name = "vpp1"
        crud_service = CRUDService()

        interface = ietf_interfaces.Interfaces.Interface()
        interface.name = name
        interface.type = iana_if_type.EthernetCsmacd()
        interface.enabled = True
        interface.ipv4 = interface.Ipv4()
        interface.ipv4.mtu = 1500
        addr = interface.Ipv4().Address()
        addr.ip = "192.168.0.3"
        addr.prefix_length = 24
        interface.ipv4.address.append(addr)

        try:
            crud_service.create(self.netopeer_cli, interface)
        except YError as err:
            print("Error create services: {}".format(err))
            assert()

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)

        self.assertEquals(interface.enabled, p.State)

        a = self.vppctl.show_address(name)
        self.assertIsNotNone(a)

        prefix = interface.ipv4.address[0].ip + "/" + \
            str(interface.ipv4.address[0].prefix_length)
        self.assertIn(prefix, a.addr)

        try:
            tmp_dir = os.listdir('/tmp')
            tmp = tmp_dir.index("sweetcomb")
            #print(tmp_dir[tmp])
        except ValueError as err:
            #print("Not found, {}".format(err))
            pass

        #print("Kill Sweetcomb [sysrepo-plugind]")

        for proc in psutil.process_iter(attrs=['pid', 'name']):
            pname = proc.info['name']
            if 'sysrepo-plugind' in pname:
                proc.kill()

        time.sleep(2)

        try:
            tmp_dir = os.listdir('/tmp')
            tmp = tmp_dir.index("sweetcomb")
            #print(tmp_dir[tmp])
        except ValueError as err:
            #print("Not found, {}".format(err))
            pass

        #print("Start Sweetcomb ")
        self.topology.start_sysrepo_plugins()

        #print("Wait 2 s")
        time.sleep(2)
        #print("Stop wait")

        p = self.vppctl.show_interface(name)
        self.assertIsNotNone(p)

        self.assertEquals(interface.enabled, p.State)

        a = self.vppctl.show_address(name)
        self.assertIsNotNone(a)

        prefix = interface.ipv4.address[0].ip + "/" + \
            str(interface.ipv4.address[0].prefix_length)
        self.assertIn(prefix, a.addr)

        self.logger.info("RESTART_VPP_SWEETCOMB_002")


if __name__ == '__main__':
    unittest.main(testRunner=SweetcombTestRunner)

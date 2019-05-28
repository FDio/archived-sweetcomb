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

import os
import subprocess
from vpp_controler import Vpp_controler
from netconf_client import NetConfClient
from socket import AF_INET
from pyroute2 import IPRoute
import psutil
import time
from ydk.providers import NetconfServiceProvider

class Topology:
    debug = False

    def __init__(self):
        self.process = []

    def __del__(self):
        self._kill_process()

    def _kill_process(self):
        if self.debug:
            return
        if not self.process:
            return

        for process in self.process:
            process.terminate()

        for proc in psutil.process_iter(attrs=['pid', 'name']):
            name = proc.info['name']
            if 'vpp' in name or 'sysrepo' in name or 'netopeer' in name:
                proc.kill()

        self.process = []

    def _prepare_linux_enviroment(self):
        ip = IPRoute()
        exist = ip.link_lookup(ifname='vpp1')
        if exist:
            return

        ip.link('add', ifname='vpp1', peer='virtual1', kind='veth')
        ip.link('add', ifname='vpp2', peer='virtual2', kind='veth')

        vpp1 = ip.link_lookup(ifname='virtual1')[0]
        vpp2 = ip.link_lookup(ifname='virtual2')[0]

        ip.link('set', index=vpp1, state='up')
        ip.link('set', index=vpp2, state='up')

        ip.addr('add', index=vpp1, address='192.168.0.2', prefixlen=24)
        ip.addr('add', index=vpp2, address='192.168.1.2', prefixlen=24)


    def _start_sysrepo(self):
        print("Start sysrepo deamon.")
        #TODO: Need property close.
        err = open("/var/log/sysrepod", 'wb')
        if self.debug:
            params = "-l 4"
        else:
            params = "-l 3"
        self.sysrepo = subprocess.Popen(["sysrepod", "-d", params],
                                        stdout=subprocess.PIPE, stderr=err)
        self.process.append(self.sysrepo)

    def _start_sysrepo_plugins(self):
        print("Start sysrepo plugins.")
        #TODO: Need property close.
        err = open("/var/log/sysrepo-plugind", 'wb')
        if self.debug:
            params = "-l 4"
        else:
            params = "-l 3"
        self.splugin = subprocess.Popen(["sysrepo-plugind", "-d", params],
                                        stdout=subprocess.PIPE, stderr=err)
        self.process.append(self.splugin)

    def _start_netopeer_server(self):
        print("Start netopeer server.")
        self.netopeer_server = subprocess.Popen("netopeer2-server",
                                                stdout=subprocess.PIPE)
        self.process.append(self.netopeer_server)

    def _start_netopeer_cli(self):
        print("Start netopeer client.")
        self.netopeer_cli = Netopeer_controler()
        self.process.append(self.netopeer_cli)
        self.netopeer_cli.spawn()

    def _start_vpp(self):
        print("Start VPP.")
        self.vpp = Vpp_controler(self.debug)
        self.vpp.spawn()
        self.process.append(self.vpp)

    def _start_netconfclient(self):
        print("Start NetconfClient")
        self.netconf_client = NetConfClient(address="127.0.0.1",
                                            username="root", password="0000")
        self.process.append(self.netconf_client)

    def get_vpp(self):
        return self.vpp

    def get_netopeer_cli(self):
        #return self.netopeer_cli
        return self.netconf_client

    def create_topology(self, debug=False):
        #try:
        self.debug = debug
        self._prepare_linux_enviroment()
        self._start_vpp()
        self._start_sysrepo()
        time.sleep(1)
        self._start_sysrepo_plugins()
        self._start_netopeer_server()

        #Wait for netopeer server
        time.sleep(1)
        #self._start_netopeer_cli()
        self._start_netconfclient()
        #except:
            #self._kill_process()

    def close_topology(self):
        self._kill_process()

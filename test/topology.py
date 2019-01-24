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

import os
import subprocess
from netopeer_controler import Netopeer_controler
from vpp_controler import Vpp_controler
from socket import AF_INET
from pyroute2 import IPRoute
import psutil
import time

class Topology:
    def __init__(self):
        self.process = []

    def __del__(self):
        self._kill_process()

    def _kill_process(self):
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
        self.sysrepo = subprocess.Popen(["sysrepod", "-d", "-l 3"],
                                        stdout=subprocess.PIPE, stderr=err)
        self.process.append(self.sysrepo)

    def _start_sysrepo_plugins(self):
        print("Start sysrepo plugins.")
        #TODO: Need property close.
        err = open("/var/log/sysrepo-plugind", 'wb')
        self.splugin = subprocess.Popen(["sysrepo-plugind", "-d", "-l 3"],
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
        self.vpp = Vpp_controler()
        self.vpp.spawn()
        self.process.append(self.vpp)

    def get_vpp(self):
        return self.vpp

    def get_netopeer_cli(self):
        return self.netopeer_cli

    def create_topology(self):
        #try:
        self._prepare_linux_enviroment()
        self._start_vpp()
        self._start_sysrepo()
        time.sleep(1)
        self._start_sysrepo_plugins()
        self._start_netopeer_server()

        #Wait for netopeer server
        time.sleep(1)
        self._start_netopeer_cli()
        #except:
            #self._kill_process()

    def close_topology(self):
        self._kill_process()

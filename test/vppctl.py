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

import subprocess
import re


class VppInterface:
    def __init__(self, str):
        self.str = str
        self.name = None
        self.Idx = None
        self.State = None
        self.MTU = None
        self.Counter = None
        self.Count = None
        self._parse()

    def _parse(self):
        columns = re.split('\s+', self.str)
        j = 0
        for column in columns:
            j += 1
            if 1 == j:
                self.name = column
            elif 2 == j:
                self.Idx = column
            elif 3 == j:
                if 'up' == column:
                    self.State = True
                elif 'down' == column:
                    self.State = False
            elif 4 == j:
                self.MTU = column.split("/")[0]
            elif 5 == j:
                self.Counter = column
            elif 6 == j:
                self.Count = column


class VppIpv4Address:
    def __init__(self, name):
        self.name = name
        self.str = str
        self.addr = list()

    def addAddress(self, str):
        tmp = re.split('\s+', str)
        self.addr.append(tmp[2])


class Vppctl:
    def __init__(self):
        self.cmd = "vppctl"

    def show_interface(self, name=None):
        interfaces = list()
        p = subprocess.run(self.cmd + " show int", shell=True,
                           stdout=subprocess.PIPE)
        str = p.stdout.decode("utf-8")
        lines = str.split("\n")
        i = 0
        for line in lines:
            i += 1
            if 1 == i:
                continue

            interfaces.append(VppInterface(line))

        if name is None:
            return interfaces
        else:
            for intf in interfaces:
                if intf.name == name:
                    return intf

        return None

    def show_address(self, ifName=None):
        interfaces = dict()
        p = subprocess.run(self.cmd + " show int addr", shell=True,
                           stdout=subprocess.PIPE)
        str = p.stdout.decode("utf-8")
        lines = str.split("\n")
        for line in lines:
            if re.match('^\s+', line) is None:
                tmp = line.split(" ")
                name = tmp[0]
            else:
                if name is None:
                    continue

                if name in interfaces:
                    interface = interfaces[name]
                else:
                    interface = VppIpv4Address(name)
                    interfaces[name] = interface

                interface.addAddress(line)

        if ifName is None:
            return interfaces
        elif ifName in interfaces:
            return interfaces[ifName]
        else:
            return None

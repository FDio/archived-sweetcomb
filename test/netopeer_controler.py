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

import pexpect

class Netopeer_controler:
    def __init__(self):
        self.name = "netopeer2-cli"
        self.password = "0000"

    def __del__(self):
        self.kill()

    def kill(self):
        if self.child is None:
            return

        self.child.sendline("exit\r")
        self.child.logfile.close()
        #self.child.kill()
        self.child = None

    def terminate(self):
        self.kill()

    def set_password(self, password):
        self.password = password;

    def spawn(self):
        self.child = pexpect.spawn(self.name)
        self.child.logfile = open('/var/log/Netopeer_controler.log', 'wb')
        self.pid = self.child.pid
        self.child.expect(">")
        self.child.sendline("connect\r")
        i = self.child.expect(["Password:", "Are you sure you want to continue connecting (yes/no)?"])
        if 0 == i:
            self.child.sendline(self.password + '\r')
        elif 1 == i:
            self.child.sendline("yes\r")
            self.child.expect("Password:")
            self.child.sendline(self.password + '\r')

        self.child.expect(">")

    def get(self, msg):
        self.child.sendline("get --filter-xpath {}\r".format(msg))
        self.child.expect("> ")
        print(self.child.before.decode('ascii'))

    def edit_config(self, msg):
        f = open("/tmp/tmp_example.xml", "w")
        f.write(msg)
        f.close()

        self.child.sendline("edit-config --target running --config=/tmp/tmp_example.xml\r")
        self.child.expect("> ")
        print(self.child.before.decode('ascii'))

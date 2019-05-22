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

import subprocess
import time
import psutil

class Vpp_controler:
    debug = False

    def __init__(self, debug=False):
        self.cmd = "vpp"
        self.ccmd = "vppctl"
        self.configuration = "/root/src/sweetcomb/test/conf/vpp.conf"
        self.process = None
        self.debug = debug

    def __del__(self):
        #self.kill()
        self.terminate()

    def _default_conf_vpp(self):
        if self.process is None:
            return

        subprocess.run(self.ccmd + " create host name vpp1", shell=True)
        subprocess.run(self.ccmd + " create host name vpp2", shell=True)

    def spawn(self):
        self.process = subprocess.Popen([self.cmd, "-c", self.configuration],
                                        stdout=subprocess.PIPE)
        time.sleep(4)
        self._default_conf_vpp()

    def kill(self):
        if self.process is None:
            return

        self.process.kill()
        self.process = None

    def terminate(self):
        if self.debug:
            return
        if self.process is None:
            return

        self.process.terminate()
        for proc in psutil.process_iter(attrs=['pid', 'name']):
            if 'vpp' in proc.info['name']:
                proc.kill()

        self.process = None

    def show_interface(self):
        subprocess.run(self.ccmd + " show int", shell=True)

    def show_address(self):
        subprocess.run(self.ccmd + " show int addr", shell=True)

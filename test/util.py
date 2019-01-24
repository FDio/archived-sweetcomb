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
import os
import time

def ping(ip):
    subprocess.run("ping -c 4 " + ip, shell=True)

def import_yang_modules():
    print("Import YANG modules to sysrepo.")

    directory = '/root/src/sweetcomb/'
    os.chdir(directory + "src/plugins/yang/ietf/")
    subprocess.run(["sysrepoctl", "--install", "-S", ".",
                    "--yang=./ietf-interfaces.yang"])

    os.chdir(directory + "src/plugins/yang/openconfig/")
    subprocess.run(["sysrepoctl", "--install", "-S", ".",
                    "--yang=./openconfig-interfaces.yang"])
    subprocess.run(["sysrepoctl", "--install", "-S", ".",
                    "--yang=./openconfig-if-ip.yang"])
    subprocess.run(["sysrepoctl", "--install", "-S", ".",
                    "--yang=./openconfig-local-routing.yang"])

    time.sleep(5)

    os.chdir(directory + "test/conf/")

    print("Import configuration to sysrepo datastore.")
    subprocess.run(["sysrepocfg", "--import=ietf-interfaces.xml",
                    "--datastore=startup", "--format=xml", "--leve=0",
                    "ietf-interfaces"])
    subprocess.run(["sysrepocfg", "--import=ietf-interfaces.xml",
                    "--datastore=running", "--format=xml", "--leve=0",
                    "ietf-interfaces"])
    os.chdir(directory)
    time.sleep(2)

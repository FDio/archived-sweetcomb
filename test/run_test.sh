#!/bin/bash

# Copyright (c) 2019 PANTHEON.tech.
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


IMAGE="sweetcomb_img"
CONTAINER="sweetcomb_test"

function build_enviroment {
    FIND=`docker images | grep ${IMAGE}`

    if [ -n "${FIND}" ]; then
        return
    fi

    docker build -t ${IMAGE} .
}

function create_container {
    docker run -id --privileged --name ${CONTAINER} \
        -v $(pwd)/../sweetcomb:/root/src/sweetcomb ${IMAGE}
}

function start_container {
    FIND=`docker container ls -a | grep ${CONTAINER}`

    if [ -z "${FIND}" ]; then
        create_container
    else
        FIND=`docker container ps | grep ${CONTAINER}`
        if [ -z "${FIND}" ]; then
            docker start ${CONTAINER}
        fi
    fi
}

function build_sweetcomb {
    docker exec -it ${CONTAINER} bash -c "apt-get install -y python3-pip && pip3 install pexpect && pip3 install pyroute2"
}

function main {
    build_enviroment
    start_container
    build_sweetcomb

    docker exec -it ${CONTAINER} bash -c "

    apt-get install -y python3-pip

    pip3 install pyroute2 pexpect psutil

    echo -e \"0000\n0000\" | passwd
    "


    docker exec -it ${CONTAINER} bash -c "/root/src/sweetcomb/test/run_test.py"

    echo "Wait 20s, then remove the container."
    sleep 20

    docker stop ${CONTAINER}
    docker container rm ${CONTAINER}
}

main $@

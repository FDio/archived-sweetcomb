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
CONTAINER="sweetcomb"

function build_enviroment {
    FIND=`docker images | grep ${IMAGE}`

    if [ -n "${FIND}" ]; then
        return
    fi

    docker build -t ${IMAGE} ./src/Docker/Build
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
    docker exec -it ${CONTAINER} bash -c "/root/src/sweetcomb/build-root/scripts/de_build.sh"
}

function main {
    build_enviroment
    start_container
    build_sweetcomb
}

main $@

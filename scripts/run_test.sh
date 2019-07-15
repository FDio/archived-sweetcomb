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
IMAGE_TEST="sweetcomb_test_img"
CONTAINER="sweetcomb_test"

FIND=`docker container ls -a | grep ${CONTAINER}`
if [ -n "${FIND}" ]; then
    echo "Remove previous test container"
    docker stop ${CONTAINER} > /dev/null
    docker rm ${CONTAINER} -f > /dev/null
fi

FIND=`docker images | grep ${IMAGE}`
if [ -z "${FIND}" ] || [ "$REBUILD_DOCKER_IMAGE" == "yes" ]; then
    ./scripts/docker.sh
fi

FIND=`docker images | grep ${IMAGE_TEST}`
if [ -z "${FIND}" ] || [ "$REBUILD_DOCKER_IMAGE" == "yes" ]; then
    echo "Rebuild test image"
    docker rmi ${IMAGE_TEST} -f > /dev/null 2>&1
    docker build -t ${IMAGE_TEST} -f ./scripts/Test.Dockerfile .
fi

echo "Start container"
docker run -id --privileged --name ${CONTAINER} ${IMAGE_TEST}
docker cp . ${CONTAINER}:/root/src/sweetcomb
docker exec -it ${CONTAINER} bash -c "
    cd /root/src/sweetcomb &&
    make build-plugins"
if [ "$?" == 0 ]; then
    echo "Run tests"
    docker exec -it ${CONTAINER} bash -c "
        useradd user
        echo -e \"user\nuser\" | passwd user
    "
    docker exec -it ${CONTAINER} bash -c "
        echo -e \"0000\n0000\" | passwd
        mkdir /var/log/vpp"
fi

docker exec -it ${CONTAINER} bash -c "cd /root/src/sweetcomb && make test-plugins"

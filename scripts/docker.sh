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

echo "Remove previous container and image"
FIND=`docker container ls -a | grep ${CONTAINER}`
if [ -n "${FIND}" ]; then
    docker stop ${CONTAINER}
    docker rm ${CONTAINER} -f
fi
FIND=`docker images | grep ${IMAGE}`
if [ -n "${FIND}" ]; then
    docker rmi ${IMAGE} -f
fi

echo "Rebuild image and start container"
docker build -t ${IMAGE} -f ./scripts/Dockerfile .
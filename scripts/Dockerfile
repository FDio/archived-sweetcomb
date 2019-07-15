FROM ubuntu:18.04

ARG vpp_version=master

# Layer0: Prepare sweetcomb environement by installing sysrepo, netopeer2 & vpp
# Layer1: Install vpp
# Layer2: Install sweetcomb

#Layer 0
RUN mkdir -p /opt/dev && apt-get update &&\
    apt-get install -y build-essential sudo
COPY . /opt/dev
WORKDIR /opt/dev
RUN make install-dep && make install-dep-extra
RUN rm -rf /opt/dev/*

#Layer1
RUN apt-get install -y curl inetutils-ping \
    && curl -s https://packagecloud.io/install/repositories/fdio/$vpp_version/script.deb.sh | bash \
    && apt-get -y --force-yes install vpp libvppinfra* vpp-plugin-* vpp-dev vom

#Layer2
COPY . /root/src/sweetcomb
WORKDIR /root/src/sweetcomb

RUN make install-dep && make build-plugins

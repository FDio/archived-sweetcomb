FROM sysrepo/sysrepo-netopeer2:v0.7.7

# Layer1: Prepare sweetcomb environement by installing sysrepo, netopeer2 & vpp
# Layer2: Install sweetcomb


#Clean sysrepo previous build because they forgot to do it
RUN rm -rf /opt/dev/*

#Install VPP
RUN apt-get update && apt-get install -y \
    #build utils
    sudo curl \
    #test utils
    inetutils-ping \
    && curl -s https://packagecloud.io/install/repositories/fdio/1904/script.deb.sh | sudo bash \
    && apt-get -y --force-yes install vpp libvppinfra* vpp-plugin-* vpp-dev

# Install sweetcomb
COPY . /root/src/sweetcomb
WORKDIR /root/src/sweetcomb

RUN make install-dep && make build-scvpp && make build-plugins

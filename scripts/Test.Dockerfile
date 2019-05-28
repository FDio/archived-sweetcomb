FROM sweetcomb_img:latest

#Install utils for testing
RUN apt-get update; \
    apt-get install -y vim clang-format python3-pip python-pip; \
    apt-get install -y gdebi-core python3-dev python-dev libtool-bin; \
    apt-get install -y libcurl4-openssl-dev libpcre3-dev libssh-dev libxml2-dev libxslt1-dev cmake python-git; \
    pip3 install pexpect pyroute2 psutil;

WORKDIR /root/src

RUN git clone https://github.com/CiscoDevNet/ydk-gen.git

WORKDIR /root/src/ydk-gen

RUN pip install -r requirements.txt && \
    ./generate.py --libydk -i && ./generate.py --python --core && \
    pip3 install gen-api/python/ydk/dist/ydk*.tar.gz && \
    ./generate.py --python --bundle profiles/bundles/ietf_0_1_5.json && \
    ./generate.py --python --bundle profiles/bundles/openconfig_0_1_5.json && \
    pip3 install gen-api/python/ietf-bundle/dist/ydk*.tar.gz && \
    pip3 install gen-api/python/openconfig-bundle/dist/ydk*.tar.gz

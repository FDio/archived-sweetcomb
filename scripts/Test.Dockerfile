FROM sweetcomb_img:latest

#Install utils for testing
RUN apt-get update; \
    apt-get install -y vim clang-format python3-pip; \
    pip3 install pexpect pyroute2 psutil;
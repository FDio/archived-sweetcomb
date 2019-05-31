FROM sweetcomb_img:latest

# Install test dependence

COPY . /root/src/sweetcomb
WORKDIR /root/src/sweetcomb

RUN make install-test-extra

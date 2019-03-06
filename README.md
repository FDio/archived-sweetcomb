Sweetcomb
========================

## Introduction

Sweetcomb is a management agent that runs on the same host as a VPP instance, 
and exposes yang models via NETCONF or RESTCONF or gRPC to allow the management of that VPP instance from out-of-box. 

For more information on VPP and its features please visit the
[Sweetcomb website](https://wiki.fd.io/view/Sweetcomb)


## Changes

Details of the changes leading up to this version of Sweetcomb can be found under
@ref release notes.


## Directory layout

| Directory name         | Description                                 |
| ---------------------- | ------------------------------------------- |
|      build-root        | Build output directory                      |
| @ref src/plugins       | Sweetcomb bundled plugins directory         |
| @ref src/scvpp         | Sweetcomb to VPP communication              |

## Getting started

Make sure you have added FD.io repository using https://packagecloud.io/fdio/release/
installation script.
You should have a sight on the release package, the package name may be different depending on the distribution.
(ex: vpp-plugins.deb for VPP 19.01 and vpp-plugin-core.deb and vpp-plugin-dpdk.deb in 19.04)

Firstly, please follow below steps to install dependencies and build code:
```
   cd $/sweetcomb/
   make install-dep
   make install-dep-extra
   make install-vpp
   make build-scvpp
   make build-plugins
```

Next, install YANG models in sysrepo:
```
    make install-models
```

Then, please start each daemon one by one:
```
   start vpp (for example on Ubuntu: systemctl start vpp)
   sysrepod
   sysrepo-plugind
   netopeer2-server
   netopeer2-cli
```

Now you can utilize Sweetcomb.

## Manual Test
For example, if you want to configure ipv4 address on HW interface TenGigabitEthernet5/0/0,
You can follow below steps to verify if Sweetcomb is working well.

Firstly, set interface up:
`vppctl set interface state TenGigabitEthernet5/0/0 up`

Then, starting netopeer2-cli on any host:
  netopeer2-cli
```
> connect --host <ip address running Sweetcomb> --login <user>
> edit-config --target running --config 
```

```
<interfaces xmlns="urn:ietf:params:xml:ns:yang:ietf-interfaces">
  <interface>
    <name>TenGigabitEthernet5/0/0</name>
    <description>eth0</description>
    <type xmlns:ianaift="urn:ietf:params:xml:ns:yang:iana-if-type">ianaift:ethernetCsmacd</type>
    <ipv4 xmlns="urn:ietf:params:xml:ns:yang:ietf-ip">
      <enabled>true</enabled>
      <mtu>1514</mtu>
      <address>
        <ip>192.168.50.72</ip>
        <prefix-length>24</prefix-length>
      </address>
    </ipv4>
    <enabled>true</enabled>
  </interface>
</interfaces>
```

Finally, check the configuration result.
    `vppctl show interface address`
If you configure above successfully, you will get ip address set up on interface TenGigabitEthernet5/0/0.


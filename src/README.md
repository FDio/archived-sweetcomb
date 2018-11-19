# Sysrepo - VPP Integration

This repository contains a work-in-progress code of the integration of [Sysrepo datastore](https://github.com/sysrepo/sysrepo/) with [VPP](https://fd.io/).

It consists of two components:
- [srvpp](srvpp/) library that provides convenient API for managing VPP from Sysrepo plugins,
- [plugins](plugins/) for Sysrepo that provide the VPP management functionality.

As of now, basic VPP interface management (interface enable/disble, IPv4/IPv6 address config) has been implemented in [vpp-interfaces plugin](plugins/vpp-interfaces.c).

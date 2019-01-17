# sweetcomb

1. This version is based on [FDio/vpp](https://github.com/FDio/vpp) stable/18.04.

2. `sweetcomb` will compile to be a sysrepo-plugin share-object(.so) file, and it should be put at specified path(e.g. `/usr/lib64/sysrepo/plugins`).

3. `netopeer2-server` deal the ssh connection and yang models' parse.

4. `sweetcomb` subscribes and deals with the specified yang in sysrepo.

5. `sweetcomb` operates vpp by `vapi`.

6. The subdir in `src/plugins` are corresponding to VPP's VAPI.

    `1 *.api.vapi.h   -->   1 subdir`

7. the requireed functions of sysrepo-plugin are in `src/common/sc_vpp_operation.[hc]`

  ```C
	int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx);
	void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx);
  ```

8. The boot sequence should be : 
	
	`sysrepod -> sysrepo-plugind -> netopeer2-server`
	
   Compilation and installation of sysrepo && netopeer2 refer to :

    - [CESNET/Netopeer2](https://github.com/CESNET/Netopeer2)
    - [sysrepo/sysrepo](https://github.com/sysrepo/sysrepo)


9. How to add functions?

	**STEP1)**
	 
		write the yang file according to the struct in *.api.vapi.h.

	**STEP2)**
	 
	 	install yang.(sysrepoctl -h)
  
	**STEP3)**
	 
	 	add the file or subdir at src/plugins. 
  
	**STEP4)**
   	 
	 	you can use `build-root/scripts/scblder.sh` to build something.

	```BASH
	source build-root/scripts/scblder.sh;scblder src/scvpp
	```


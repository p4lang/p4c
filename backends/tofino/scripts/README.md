# Helper scripts

## Binary distribution support

bf_p4c_distro.sh and p4c_build_for_arch.sh are supporting scripts to
build a binary distribution for the compiler.

bf_p4c_distro.sh clones the master branch and invokes p4c_build_for_arch.sh
to build and install MacOS and Linux 64-bit versions of the compiler. It
assumes it is running on a Mac, and it uses an Ubuntu 16.04 docker
image that has all the necessary dependencies installed (as the one
defined by Dockerfile). The name of the image should be
p4c-tofino:ubuntu16.04. These pre-requisites will need to be adjusted
when building in Jenkins/Travis.

The result is a tar file with the following structure:
```
p4c-<version>
   |-share
   |---p4c
   |-----p4_14include
   |-------tofino
   |-----p4c_src
   |-----p4include
   |-----tofino
   |-x86_64-apple-darwin
   |---bin
   |-x86_64-linux-gnu
   |---bin
```

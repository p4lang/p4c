# BAREFOOT MODEL REPOSITORY

This repository is the parent for Behavioral and Validation
model development.

## Repository Overview

The code here is based on the infrastructure available from the
Switch Light project.  This provides a build infrastructure, utility
functionality modules and network simulation plumbing infrastructure.
See http://github.com/floodlight for some of the related
repositories.

The repository is divided into library modules (in the 'modules'
subdirectory) and build targets (in the 'targets' subdirectory).
External dependencies are downloaded on demand are managed in the
'submodules' subdirectory.  The 'infra' submodule is always
required.  For the OpenFlow connector targets (see below) other
Switch Light/Indigo targets are required.

The documentation below is focused on getting started with the unit
test and switch simulation targets.  For API level documentation and
implementation details, build the documentation with 'make doc'; then
use a browser to open the file doc/html/index.html.

## Caveats

There may be package dependencies that are not listed here

Note that for all switch applications you need to be root because only
veth interfaces are currently supported (and you need to be root to
open raw sockets on these interfaces).

## Set Up VETH Interfaces

For running most switch simulations, you need to tell the kernel to
create veth interfaces.  These will stick around unless you reboot or
remove them explicitly, so you only need to do this once.  There is a
script to make this easy.

### TODO

    user@box$ cd model/tools
    user@box$ sudo ./veth_setup.sh

This creates four veth pairs which can be checked by running 'ifconfig'.
You can remove them with the veth_teardown.sh if you like, but they're
mostly harmless when not in use.

### Unit Tests

After cloning the repo, you should be able to build and run the
tests with:

    user@box$ cd model/targets/utests/*tbd*
    user@box$ make

    ...
    Linking[debug]: TBD_utest::utest_TBD

#### Building the code

The target is TBD

    user@box$ cd model/targets/*tbd*
    user@box$ make

The executable is placed in `build/gcc-local/bin/*tbd*`.  As above,
you need to execute as root.

    user@box$ cd build/gcc-local/bin
    user@box$ sudo ./*tbd*

#! /bin/bash

# params:
#  workspace
#  architecture: one of x86_64-apple-darwin, x86_64-linux-gnu, i386-linux-gnu
#  p4c-version

set -e

WORKSPACE=$1
arch=$2
P4C_VERSION=$3

build_dir=build.$arch
install_dir=$WORKSPACE/install/p4c-${P4C_VERSION}

parallel_make=4
if [[ $arch =~ .*-linux-gnu ]]; then
    # building on docker
    parallel_make=2
fi

# remap the bootstrapping files for the architecture
cd $WORKSPACE/bf-p4c-compilers
sed -e "s/build/${build_dir}/g" -e "s/bootstrap.sh/bootstrap.${arch}.sh/g" bootstrap_bfn_compilers.sh > bootstrap.${arch}.sh
chmod 755 bootstrap.${arch}.sh

# compiler
cd $WORKSPACE/bf-p4c-compilers/p4c
sed -e "s/build/${build_dir}/g" bootstrap.sh > bootstrap.${arch}.sh
chmod 755 bootstrap.${arch}.sh

cd $WORKSPACE/bf-p4c-compilers/
./bootstrap.${arch}.sh  --prefix $install_dir --exec-prefix=$install_dir/$arch

rm -f ./bootstrap.${arch}.sh p4c/bootstrap.${arch}.sh

cd ${build_dir}
make -j $parallel_make install

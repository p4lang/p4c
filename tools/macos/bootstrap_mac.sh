#! /bin/bash

set -e
set -x

tools_dir=`dirname $0`

# check for brew,
if [[ -z `which brew` ]]; then
    # install homebrew
    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    # python pip installs in /usr/local/tests
    sudo mkdir -p /usr/local/tests
    sudo chown ${USER}:admin /usr/local/tests
fi

# check for clang version
cxx_full_version=`clang++ --version | grep version`
cxx_version=`echo $cxx_full_version | cut -f 4 -d " "`
if [[ $cxx_version < "8.0.0" ]]; then
    echo "Please install Xcode 8 or command line tools v8 or higher."
    echo "It is a requirement for thread_local support"
    echo ""
    echo "You have: $cxx_full_version"
    exit 1
fi

# install basic tools
brew install automake autoconf bison boost clang-format cmake coreutils \
     doxygen homebrew/versions/gcc5 gmp libevent openssl pkg-config wget
# bison needs a bit more nudging
brew link --overwrite --force bison

# we need realpath from coreutils
brews_dir=`realpath $tools_dir`/brews

# thrift
brew install --HEAD ${brews_dir}/thrift.rb --with-python --with-libevent

brew install python
/usr/local/bin/pip install ipaddr
/usr/local/bin/pip install pcapy

# nanomsg
brew install nanomsg

# # scapy
brew install homebrew/python/scapy

# nnpy
/usr/local/bin/pip install pycparser
brew install ${brews_dir}/nnpy.rb

# libjudy
brew install ${brews_dir}/libjudy.rb

/usr/local/bin/pip install PyYAML

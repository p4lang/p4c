#! /bin/bash

# Install some custom requirements on OS X using brew
BREW=/usr/local/bin/brew
if [[ ! -x $BREW ]]; then
    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

$BREW update
$BREW install autoconf automake bdw-gc bison boost ccache cmake \
      libtool openssl pkg-config python
$BREW install protobuf

# Prefer Homebrew's bison over the macOS-provided version
$BREW link --force bison
echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
export PATH="/usr/local/opt/bison/bin:$PATH"

# install pip and required pip packages
# curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
# python get-pip.py --user
# use scapy 2.4.5, which is the version on which ptf depends
pip3 install --user scapy==2.4.5 ply==3.8

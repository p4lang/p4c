#! /bin/bash

# Install some custom requirements on OS X using brew
BREW=/usr/local/bin/brew
if [[ ! -x $BREW ]]; then
    /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

# Google introduced another breaking change with protobuf 22.x by adding abseil as a new dependency.
# https://protobuf.dev/news/2022-08-03/#abseil-dep
# We do not want abseil, so we stay with 21.x for now.
PROTOBUF_LIB="protobuf@21"
BOOST_LIB="boost@1.76"

$BREW update
$BREW install autoconf automake bdw-gc bison ${BOOST_LIB} ccache cmake \
      libtool openssl pkg-config python coreutils grep ${PROTOBUF_LIB}

# Prefer Homebrew's bison, grep, and protobuf over the macOS-provided version
$BREW link --force bison grep ${PROTOBUF_LIB} ${BOOST_LIB}
echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="/usr/local/opt/${PROTOBUF_LIB}/bin:$PATH"' >> ~/.bash_profile
eecho 'export PATH="/usr/local/opt/grep/libexec/gnubin:$PATH"' >> ~/.bash_profile
export PATH="/usr/local/opt/bison/bin:$PATH"
export PATH="/usr/local/opt/grep/libexec/gnubin:$PATH"

# install pip and required pip packages
# curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
# python get-pip.py --user
# use scapy 2.4.5, which is the version on which ptf depends
pip3 install --user scapy==2.4.5 ply==3.8

#! /bin/bash

# Script for building P4C on MacOS.

set -e  # Exit on error.
set -x  # Make command execution verbose

# Install some custom requirements on OS X using brew
BREW=/usr/local/bin/brew
if [[ ! -x $BREW ]]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

BOOST_LIB="boost@1.76"

$BREW install autoconf automake bdw-gc ccache cmake \
      libtool openssl pkg-config python coreutils

# We need to link boost.
$BREW install ${BOOST_LIB}
$BREW link ${BOOST_LIB}
# Prefer Homebrew's bison and grep over the macOS-provided version
$BREW install bison
echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
$BREW install grep
echo 'export PATH="/usr/local/opt/grep/libexec/gnubin:$PATH"' >> ~/.bash_profile
source ~/.bash_profile


# install pip and required pip packages
# curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
# python get-pip.py --user
# use scapy 2.4.5, which is the version on which ptf depends
pip3 install --user scapy==2.4.5 ply==3.8

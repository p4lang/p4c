#! /bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    # Install some custom requirements on OS X using brew
    BREW=/usr/local/bin/brew
    if [[ ! -x $BREW ]]; then
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi

    $BREW update
    $BREW install autoconf automake bdw-gc bison boost ccache cmake git libtool openssl pkg-config protobuf
    $BREW install gmp --c++11

    # Prefer Homebrew's bison over the macOS-provided version
    $BREW link --force bison
    echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
    export PATH="/usr/local/opt/bison/bin:$PATH"

    # install pip and required pip packages
    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    python get-pip.py --user
    pip install --user scapy==2.4.0 ply
fi

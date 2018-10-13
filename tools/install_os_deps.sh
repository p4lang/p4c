#! /bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    # Install some custom requirements on OS X using brew
    BREW=/usr/local/bin/brew
    if [[ ! -x $BREW ]]; then
        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi

    $BREW update
    $BREW install autoconf automake bdw-gc bison boost ccache cmake git libtool openssl pkg-config protobuf
    $BREW link --force bison
    $BREW install gmp --c++11
fi

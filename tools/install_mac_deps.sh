#! /bin/bash

# Script for building P4C on MacOS.

set -e  # Exit on error.
set -x  # Make command execution verbose


# Set up Homebrew differently for arm64.
if [[ $(uname -m) == 'arm64' ]]; then
    (echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"
else
    (echo; echo 'eval "$(/usr/local/bin/brew shellenv)"') >> ~/.zprofile
    eval "$(/usr/local/bin/brew shellenv)"
fi

if [[ ! -x brew ]]; then
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi
HOMEBREW_PREFIX=$(brew --prefix)

# Install some custom requirements on OS X using brew
BOOST_LIB="boost@1.84"
brew install autoconf automake bdw-gc ccache cmake \
      libtool openssl pkg-config coreutils bison grep \
      ${BOOST_LIB}

# We need to link boost and openssl.
brew link ${BOOST_LIB} openssl
# Prefer Homebrew's bison and grep over the macOS-provided version.
# For Bison only `$(brew --prefix bison)/bin` seems to work...
echo 'export PATH="$(brew --prefix bison)/bin:$PATH"' >> ~/.bash_profile
echo 'export PATH="$HOMEBREW_PREFIX/opt/grep/libexec/gnubin:$PATH"' >> ~/.bash_profile
source ~/.bash_profile


# Fixes for stuck grpcio installation.
export GRPC_PYTHON_BUILD_SYSTEM_OPENSSL=1
export GRPC_PYTHON_BUILD_SYSTEM_ZLIB=1
# Install required pip packages
pip3 install --user -r requirements.txt

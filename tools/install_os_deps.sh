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
    echo 'echo "===== RAN BASH_PROFILE ====="' >> ~/.bash_profile
    echo 'echo "===== RAN BASH_PROFILE =====" >&2' >> ~/.bash_profile
    echo 'echo "===== RAN BASHRC ====="' >> ~/.bashrc
    echo 'echo "===== RAN BASHRC =====" >&2' >> ~/.bashrc

    ls -Al ~

    cat >/tmp/moo <<EOF
#!/bin/bash

which bison
bison --version
EOF

    chmod a+x /tmp/moo
    /tmp/moo

    export PATH="/usr/local/opt/bison/bin:$PATH"
    find / -name bison \( -type f -o -type l \) 2>/dev/null
    find / -name bison \( -type f -o -type l \) -exec {} --version \; 2>/dev/null

    # install pip and required pip packages
    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    python get-pip.py --user
    pip install --user scapy ply
fi

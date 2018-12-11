#! /bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
    echo 'echo "===== RAN BASH_PROFILE ====="' >> ~/.bash_profile
    echo 'echo "===== RAN BASH_PROFILE =====" >&2' >> ~/.bash_profile
    echo 'echo "===== RAN BASH_PROFILE ===== >> /tmp/log"' >> ~/.bash_profile
    echo 'echo "===== RAN BASHRC ====="' >> ~/.bashrc
    echo 'echo "===== RAN BASHRC =====" >&2' >> ~/.bashrc
    echo 'echo "===== RAN BASHRC =====" >> /tmp/log' >> ~/.bashrc

    echo 'env ========================================='

    env | sort

    echo 'profile ======================================'

    cat ~/.bash_profile

    echo 'rc ==========================================='

    cat ~/.bashrc

    echo 'job_stages ==================================='

    cat /Users/travis/.travis/job_stages

    echo 'moo =========================================='

    cat > /tmp/moo <<EOF
#!/bin/bash

env | sort
echo 'RAN MOO' >> /tmp/log
EOF

    chmod a+x /tmp/moo
    /tmp/moo

    echo 'log =========================================='

    cat /tmp/log

    echo '=============================================='
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

    # install pip and required pip packages
    curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
    python get-pip.py --user
    pip install --user scapy ply
fi

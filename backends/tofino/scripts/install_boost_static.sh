#! /bin/bash

SUDO=sudo
boost_lib_type=static

die () {
    if [ $# -gt 0 ]; then
        echo >&2 "$@"
    fi
    exit 1
}

function version_LT() {
    test "$(echo "$@" | tr " " "\n" | sort -rV | head -n 1)" != "$1";
}


function install_boost() {
    # ARG: boost_version
    # test for ubuntu version, and check if we have a local package

    if [[ ($linux_distro == "Ubuntu" && $linux_version =~ "14.04") || \
              $linux_distro == "Debian" ]]; then
        $SUDO apt-get install -y python-dev libbz2-dev
    fi
    b_ver=$1
    boost_ver=$(echo $b_ver | tr "." "_")

    install_pkg=true
    installed_ver="1_0"
    if [ -f /usr/local/include/boost/version.hpp ]; then
        installed_ver=$(grep -m 1 "define BOOST_LIB_VERSION" /usr/local/include/boost/version.hpp | cut -d ' ' -f 3 | cut -d '_' -f 1-2 | tr -d '"')
    fi
    if [ ! -z "$installed_ver" ]; then
        boost_ver_trim=${boost_ver%_0}
        if version_LT $installed_ver $boost_ver_trim ; then
            echo "Installed boost version is not sufficient: $installed_ver, proceeding with installing required boost: $boost_ver_trim"
        else
            echo "Installed boost version is sufficient: $installed_ver"
            if [[ "$boost_lib_type" == "static" ]]; then
                boost_static_io=`find /usr/local/ -name libboost_iostreams.a`
                if [ ! -z $boost_static_io ]; then
                    echo "And static library is available"
                    install_pkg=false
                fi
            else
                install_pkg=false
            fi
        fi
    fi
    if [ $install_pkg = true ]; then
        cd /tmp
        wget http://downloads.sourceforge.net/project/boost/boost/${b_ver}/boost_${boost_ver}.tar.bz2 && \
            tar xjf ./boost_${boost_ver}.tar.bz2 && \
            cd boost_${boost_ver} && \
            ./bootstrap.sh --prefix=/usr/local && \
            ./b2 -j8 --build-type=minimal link=${boost_lib_type} \
                 runtime-link=${boost_lib_type} variant=release && \
            $SUDO ./b2 install --build-type=minimal variant=release \
                  link=${boost_lib_type} runtime-link=${boost_lib_type} || \
            die "failed to install boost"
        cd /tmp && rm -rf boost_${boost_ver}
    fi
}

install_boost "1.67.0"

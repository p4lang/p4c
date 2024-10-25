#!/bin/bash

umask 0022
cd $(dirname $0)
curdir=$(basename $PWD)
topdir=$(dirname $PWD)

keep_files=false
skip_os_deps=false

show_help () {
    echo >&2 "bootstrap_bfn_env script options"
    echo >&2 "   -keep_files    keep the build repo and files"
    echo >&2 "   -skip_os_deps  skip running os deps script"
}

while [ $# -gt 0 ]; do
    case $1 in
    -h|-help|--help)
        show_help
        exit 0
        ;;
    -keep_files|--keep_files)
        keep_files=true
        ;;
    -skip_os_deps|--skip_os_deps)
	skip_os_deps=true
	;;
    *)
        echo "Invalid argument supplied"
        show_help
        exit 0
        ;;
    esac
    shift
done

installdir="$topdir/install"
rm -rf $installdir
mkdir -p $installdir

if [ $skip_os_deps = false ]; then
    os_deps=${topdir}/${curdir}/install_os_deps.sh
    if [ ! -x $os_deps ]; then
        echo "Can not find script to install OS dependencies: $os_deps"
        exit 1
    fi
fi

confirm () {
    # call with a prompt string or use a default
    read -r -p "${1:-Are you sure? [y/N]} " response
    case $response in
        [yY][eE][sS]|[yY])
            true
            ;;
        *)
            false
            ;;
    esac
}

gitclone() {
    # arguments:
    # $1 - the repository
    # $2 - the directory name where to checkout
    # $3 - optional branch to checkout
    local branch="master"
    if [ ! -z $3 ]; then branch=$3; fi
    [ -e "$2" ] && die "$2 already exists!"
    [ ! -d "$(dirname $2)" ] && die "$(dirname $2) not a directory"
    git clone --recursive -b $branch $1 $2 || { rm -rf $2; die "can't clone $1"; }
}

echo "Using $topdir as top level directory for git repositories"
echo "Using MAKEFLAGS=${MAKEFLAGS:=-j 4}"
export MAKEFLAGS

reuse_asis=false
rerun_autoconf=false
clean_before_rebuild=false
pull_before_rebuild=false
rebase_option=""

cd $topdir
need_manual_update=false
for repodir in model bf-drivers PI; do
    if [ -d $repodir ] && [ -d $repodir/.git ]; then
        need_manual_update=true
        echo >&2 "$repodir is a git repo; need to move it to $repodir/master or $repodir/jbay_main or remove it"
    fi
done

if $need_manual_update; then
    echo >&2 "Manual cleanup required before bootstrap can continue"
    exit 1
fi

found=""
for repo in behavioral-model model/master bf-syslibs bf-utils bf-drivers/master PI/master; do
    if [ -d $repo ]; then
        if [ -d $repo/.git ]; then
            found=$found$'\n'"$repo in $topdir/$repo"
        else
            echo >&2 "$topdir/$repo exists but is not a git repository"
            exit
        fi
    fi
done

if [ -z "$found" ]; then
    echo >&2 "No exisiting repositories found"
else
    echo "Found:$found"
    keep_files=true   # do NOT delete preexisting repositories!!!!
    if confirm "Rebuild these repositories before using them?"; then
        reuse_asis=false
    else
        reuse_asis=true
    fi
    if ! $reuse_asis && confirm "Pull latest changes?"; then
        pull_before_rebuild=true
        if confirm "Use --rebase option on pull?"; then
            rebase_option="--rebase"
        fi
    fi
    if ! $reuse_asis && confirm "Rerun autoconf/automake/configure?"; then
        rerun_autoconf=true
    else
        rerun_autoconf=false
    fi
    if ! $reuse_asis && confirm "Clean before rebuild?"; then
        clean_before_rebuild=true
    fi
fi

# Separate var: if protobuf / grpc are re-installed for some reason, the PI
# needs to be rebuilt from scratch
PI_clean_before_rebuild=$clean_before_rebuild

# install all necessary packages
# and set SUDO, LDCONFIG, LDLIB_EXT as appropriate for the OS
# also sets PI_clean_before_rebuild if the protobuf/grpc have been re-installed
if [ $skip_os_deps = false ]; then
    . $os_deps
fi


### Behavioral Model setup
if [ ! -d behavioral-model/.git ]; then
    gitclone git@github.com:p4lang/behavioral-model.git behavioral-model
elif $pull_before_rebuild; then
    pushd behavioral-model >/dev/null
        git pull $rebase_option origin master
    popd >/dev/null
fi
pushd behavioral-model >/dev/null
    if $reuse_asis && [ -x "$(which simple_switch_CLI)" ]; then
        echo "Reusing installed $(which simple_switch_CLI) as is"
    else
        if $clean_before_rebuild || $rerun_autoconf || [ ! -r Makefile ]; then
            ./install_deps.sh
            ./autogen.sh
            ./configure
            if $clean_before_rebuild; then
                make clean
            fi
        fi
        make || die "BMV2 build failed"
        # p4c bmv2 tests end up using this simple_switch
        # it is a libtool wrapper script and is slow to execute the first time
        # to avoid a potential timeout in STF tests, we "warm up" here
        ./targets/simple_switch/simple_switch -h > /dev/null 2>&1
        $SUDO make install
    fi
popd >/dev/null

### Model dependencies that need manual build
## libcrafter
if [ ! -r /usr/local/include/crafter.h -o ! -x /usr/local/lib/libcrafter.${LDLIB_EXT} ]; then
    git clone https://github.com/pellegre/libcrafter
    cd libcrafter/libcrafter
    ./autogen.sh
    make || die "Failed to build libcrafter"
    $SUDO make install
    $SUDO $LDCONFIG
    cd ../..
    rm -rf libcrafter
else
    echo "libcrafter already installed"
fi
## libcli
if [ ! -r /usr/local/include/libcli.h -o ! -x /usr/local/lib/libcli.${LDLIB_EXT} ]; then
    git clone git@github.com:dparrish/libcli.git
    cd libcli
    make || die "Failed to build libcli"
    $SUDO make install
    $SUDO $LDCONFIG
    cd ..
    rm -rf libcli
else
    echo "libcli already installed"
fi

### bf-syslibs and bf-utils

install_bf_repo () {
    local bf_repo=$1
    local branch=$2
    local x_path_check=$3
    local configure_flags=$4
    if [ ! -d $bf_repo/.git ]; then
        gitclone git@github.com:barefootnetworks/$bf_repo.git $bf_repo $branch
    elif $pull_before_rebuild; then
        pushd $bf_repo >/dev/null
        git pull $rebase_option origin $branch
        popd >/dev/null
    fi
    pushd $bf_repo >/dev/null
    if $reuse_asis && [ -x "$x_path_check" ]; then
        echo "Reusing $bf_repo as is"
    else
        git submodule update --init --recursive
        if $rerun_autoconf || [ ! -r Makefile ]; then
            ./autogen.sh
            ./configure $configure_flags --prefix=$installdir
        fi
        if $clean_before_rebuild; then
            make clean
        fi
        make
        make install && \
        $SUDO $LDCONFIG || \
        die "Failed to install $bf_repo"
    fi
    popd >/dev/null
}

# build the modules only on Linux
if [ $(uname -s) == 'Linux' ]; then
    install_bf_repo "bf-syslibs" "brig-stable" "$installdir/lib/libbfsys.${LDLIB_EXT}" ""
    install_bf_repo "bf-utils" "brig-stable" "$installdir/lib/libbfutils.${LDLIB_EXT}" "--enable-shared"
fi

# Get a specific branch of given repo
get_repo () {
    local repo_root=$1
    local repo_name=$2
    local dirname=$3
    local branch=$4
    if [ ! -d $repo_name ]; then
        $(mkdir -p $repo_name)
    fi
    pushd $repo_name >/dev/null
    if [ ! -d $dirname ]; then
        gitclone git@github.com:$repo_root/$repo_name.git $dirname $branch
    elif $pull_before_rebuild; then
        pushd $dirname >/dev/null
            git pull $rebase_option origin $branch
        popd >/dev/null
    fi
    popd >/dev/null
}

build_PI () {
    get_repo "p4lang" "PI" "master" "stable"
    pushd PI/master >/dev/null
    builddir=$topdir/PI/master/build
    if [ ! -d $builddir ]; then
        $(mkdir -p $builddir)
    fi
    if $reuse_asis && [ -x $installdir/lib/libpi.${LDLIB_EXT} ]; then
        echo "Reusing PI as is"
    else
        if $rerun_autoconf || [ ! -r $builddir/Makefile ]; then
            ./autogen.sh
            cd $builddir
            ../configure --with-proto --without-internal-rpc --without-cli --prefix=$installdir
        else
            cd $builddir
        fi
        if $PI_clean_before_rebuild; then
            make clean
        fi
        make && \
        make install && \
        $SUDO $LDCONFIG || \
        die "Failed to install PI"
    fi
    popd >/dev/null
    if [ $keep_files = false ]; then
        rm -rf PI/master
    fi
}

### Driver setup
build_driver () {
    get_repo "barefootnetworks" "bf-drivers" "master" "brig-stable"
    pushd bf-drivers/master >/dev/null
    builddir=$topdir/bf-drivers/master/build
    if [ ! -d $builddir ]; then
        $(mkdir -p $builddir)
    fi
    if $reuse_asis && [ -x $builddir/bf_switchd ]; then
        echo "Reusing bf-drivers as is"
    else
        if $rerun_autoconf || [ ! -r $builddir/Makefile ]; then
            git submodule update --init --recursive
            ./autogen.sh
            cd $builddir
            CFLAGS="-O0" CPPFLAGS="-I $installdir/include" ../configure --enable-thrift --with-avago --without-kdrv --with-build-model --enable-pi --enable-grpc --prefix=$installdir
        else
            cd $builddir
        fi
        if $clean_before_rebuild; then
            make clean
        fi
        make || die "Failed to build bf-drivers"
        make install || die "Failed to install bf-drivers"
        $SUDO $LDCONFIG
    fi
    popd >/dev/null
    if [ $keep_files = false ]; then
        rm -rf bf-drivers/master
    fi
}

### Model setup
build_model () {
    get_repo "barefootnetworks" "model" "master" "brig-stable"
    pushd model/master >/dev/null
        builddir="."
        if [ -r opt/Makefile ]; then
            builddir=opt
        elif [ -r debug/Makefile ]; then
            builddir=debug
        else
            builddir=$topdir/model/master/build
            if [ ! -d $builddir ]; then
                $(mkdir -p $builddir)
            fi
        fi
        if $reuse_asis && [ -x $builddir/tests/simple_test_harness/simple_test_harness ]; then
            echo "Reusing built $PWD/$builddir/tests/simple_test_harness/simple_test_harness as is"
        else
            if [ $(uname -s) == 'Linux' ]; then
                config_args="--enable-runner --enable-model-tofinoB0 --enable-model-jbay --prefix=$installdir CXX=g++"
            else
                config_args="--enable-model-tofinoB0 --enable-model-jbay --prefix=$installdir"
            fi
            if $rerun_autoconf || [ ! -r $builddir/Makefile ]; then
                ./autogen.sh
                cd $builddir
                LDFLAGS="-L $installdir/lib" CPPFLAGS="-I $installdir/include" ../configure $config_args
                find . -name '*.gch' -o -name '*.d' | xargs rm -f
            else
                cd $builddir
            fi
            if $clean_before_rebuild; then
                make clean
            fi
            make || die "harlyn model build failed"
            # Running make install for model recursively has a circular dependency, hence using -j 1
            make -j 1 install || die "Failed to install model"
            $SUDO $LDCONFIG
        fi
    popd >/dev/null
    if [ $keep_files = false ]; then
        rm -rf model/master
    fi
}

# build the drivers and model only on Linux
if [ $(uname -s) == 'Linux' ]; then
    build_PI
    build_driver
    build_model
fi

# re-install ptf unconditionally
tmpdir=$(mktemp -d)
echo "Using $tmpdir for temporary build files"
pushd $tmpdir
gitclone https://github.com/p4lang/ptf.git ptf
cd ptf
$SUDO python setup.py install
cd ..
$SUDO rm -rf ptf
popd # tmpdir
echo "Removing $tmpdir"
rm -rf $tmpdir

if [ $(uname -s) == 'Linux' ]; then
    echo "Checking for huge pages"
    $SUDO $topdir/$curdir/scripts/ptf_hugepage_setup.sh
fi

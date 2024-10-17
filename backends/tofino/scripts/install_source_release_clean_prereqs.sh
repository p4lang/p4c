#!/bin/sh

# Install prerequisites for source_release_clean.py

SCRIPT_DIR=$(dirname $(readlink -f "$0"))

mkdir -p "$SCRIPT_DIR/_deps"

# Fetch the prerequeisites
cd "$SCRIPT_DIR/_deps"

CPPP_REPO="https://git.sr.ht/~breadbox/cppp"
if [ ! -d cpp ] ; then
    git clone ${CPPP_REPO}

    cd cppp
    make -j 4
    cd ..
fi


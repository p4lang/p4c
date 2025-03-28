#!/bin/sh

# Copyright (C) 2024 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.  You may obtain
# a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#
# SPDX-License-Identifier: Apache-2.0

WALLE=""
CHIP=""
OUT=""
objs=""
object_files=""
base_program=""
debug_info=false
tmpdir=""
pipe_args=""
READLINK_COMMAND=$(which greadlink || which readlink)
execdir=$(dirname $($READLINK_COMMAND -f $0))

if [ x"$BFAS_OPTIONS" = x"-g" ]; then
    debug_info=true
fi

tempfile() {
    file=$(basename $1 $2)
    orig=file
    ctr=1
    while [ -r $tmpdir/$file ]; do
        file=$ctr-$file
        ctr=$((ctr + 1))
    done
    echo $file
}

while [ $# -gt 0 ]; do
    case $1 in
    -b)
        base_program="$2"
        shift ; shift;;
    -g)
        debug_info=true
        shift;;
    -o)
        OUT="$2"
        shift ; shift ;;
    --walle|-w)
        WALLE="$2"
        shift ; shift ;;
    --target|-t)
        CHIP="$2"
        OUT="$2.bin"
        shift ; shift ;;
    --singlepipe|-s)
        pipe_args="--top memories.pipe --top regs.pipe"
        shift ;;
    --allpipes|-a)
        pipe_args=""
        shift ;;
    *.json.Z)
        if [ -z "$tmpdir" ]; then
            tmpdir=$(mktemp -d)
        fi
        file=$(tempfile $1 .Z)
        gunzip -c $1 >$tmpdir/$file
        objs="$objs $tmpdir/$file"
        object_files="$object_files $1"
        shift ;;
    *.json.gz)
        if [ -z "$tmpdir" ]; then
            tmpdir=$(mktemp -d)
        fi
        file=$(tempfile $1 .gz)
        gunzip -c $1 >$tmpdir/$file
        objs="$objs $tmpdir/$file"
        object_files="$object_files $1"
        shift ;;
    *.json.bz)
        if [ -z "$tmpdir" ]; then
            tmpdir=$(mktemp -d)
        fi
        file=$(tempfile $1 .bz)
        bzcat $1 >$tmpdir/$file
        objs="$objs $tmpdir/$file"
        object_files="$object_files $1"
        shift ;;
    *.json.bz2)
        if [ -z "$tmpdir" ]; then
            tmpdir=$(mktemp -d)
        fi
        file=$(tempfile $1 .bz2)
        bzcat $1 >$tmpdir/$file
        objs="$objs $tmpdir/$file"
        object_files="$object_files $1"
        shift ;;
    *.json)
        objs="$objs $1"
        object_files="$object_files $1"
        shift ;;
    *)
        echo >&2 "Unknown argument $1"
        shift ;;
    esac
done

if [ ! -x "$WALLE" ]; then
    if [ -f $execdir/walle -a -x $execdir/walle ]; then
        WALLE=$execdir/walle
    elif [ -x $execdir/walle.py ]; then
        WALLE=$execdir/walle.py
    elif [ -x $execdir/walle/walle.py ]; then
        WALLE=$execdir/walle/walle.py
    elif [ -e "$WALLE" ]; then
        echo "$WALLE must be executable"
        exit 1
    else
        echo "4: $WALLE"
        echo >&2 "Can't find walle"
        exit 1
    fi
fi

if [ -z "$CHIP" ]; then
    for jf in $objs; do
        if [ $(basename $jf) = regs.top.cfg.json ]; then
            CHIP=$(grep '"_type"' $jf | sed -e 's/.*"regs\.//' -e 's/[_"].*//')
            break
        fi
    done
    if [ -z "$CHIP" ]; then
        echo >&2 "Can't find target, assuming tofino"
        CHIP=tofino
    fi
    if [ -z "$OUT" ]; then
        OUT=$CHIP.bin
    fi
fi

schema_arg=""
if [ -r $CHIP/chip.schema ]; then
    schema_arg="--schema $CHIP/chip.schema"
elif [ -r $execdir/$CHIP/chip.schema ]; then
    schema_arg="--schema $execdir/$CHIP/chip.schema"
fi

#echo "$WALLE --target $CHIP $schema_arg -o $OUT $objs $pipe_args"
$WALLE --target $CHIP $schema_arg -o $OUT $objs $pipe_args
rc=$?

# cleanup
output_dir=$(dirname $OUT)
if [ -z "$output_dir" ]; then
    output_dir="./"
fi
if ! $debug_info; then
    rm -f $object_files
fi
if [ ! -z "$base_program" ] ; then
    pp=$output_dir/${base_program}.p4i
    if ! $debug_info && test -e $pp ; then rm -f $pp; fi
fi
if ! $debug_info && test -e $output_dir/bfas.config.log ; then
    rm -f $output_dir/bfas.config.log
fi
# if we uncompressed, remove the directory
if [ -d "$tmpdir" ]; then
    rm -rf $tmpdir
fi

# exit with a return code if walle failed
exit $rc

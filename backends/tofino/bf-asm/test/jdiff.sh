#!/bin/bash

curdir=`pwd`
json_diff_file="json_diff.txt"

TRY_BINDIRS="
    $PWD
    $PWD/..
    $PWD/../*
    $TESTDIR/..
    $TESTDIR/../*
"

function findbin () {
    found=""
    if [ -x "$BUILDDIR/$1" ]; then
        echo $BUILDDIR/$1
        return
    fi
    for d in $TRY_BINDIRS; do
        if [ -x "$d/$1" ]; then
            if [ -z "$found" -o "$d/$1" -nt "$found" ]; then
                found="$(cd $d; pwd -P)/$1"
            fi
        fi
    done
    if [ -z "$found" ]; then
        echo >&2 "Can't find $1 executable"
        echo false
        exit
    else
        echo $found
    fi
}

if [ ! -x "$JSON_DIFF" ]; then
    JSON_DIFF=$(findbin json_diff)
fi

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Please specify 2 dirs to diff"
    exit 1
fi

dir1=$(readlink -f "$1")
dir2=$(readlink -f "$2")
echo "Comparing dir1 vs dir2"
echo "dir1="$dir1
echo "dir2="$dir2

> $json_diff_file
for f1 in $dir1/*.cfg.json*; do
    f=$(basename $f1)
    f2="$dir2/${f1##*/}"
    if [ ! -r $f2 ]; then
        ext="${f2##*.}"
        if [ "$ext" = "gz" ]; then
            f2=${f2%.*}
        else
            f2="$f2.gz"
        fi
    fi
    if [ ! -r $f2 ]; then
        echo "Cannot read file $f2"
        continue
    fi
    echo "Comparing file $(basename $f1) vs $(basename $f2)"
    if zcmp -s $f1 $f2; then
        continue
    elif [ "$f1" = "$dir1/regs.pipe.cfg.json" ]; then
        $JSON_DIFF -i mau $f1 $f2
        if [ $? -gt 128 ]; then
            echo "***json_diff crashed"
        fi
        continue
    fi
    $JSON_DIFF $f1 $f2 >> $json_diff_file
done

if [ -r $dir1/context.json ]; then
    { $JSON_DIFF $CTXT_DIFFARGS $dir1/context.json $dir2/context.json;
      if [ $? -gt 128 ]; then echo "***json_diff crashed"; fi; } >> \
          $json_diff_file
fi

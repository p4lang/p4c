#!/usr/bin/env bash

if [ "$GCOV" != "" ]; then
   cd src/bm_sim
   $GCOV -p -r -o .libs/ *.cpp
   rm *third_party*
   cd -
   bash <(curl -s https://codecov.io/bash) -x $GCOV -p src/bm_sim
fi

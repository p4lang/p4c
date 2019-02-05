#!/usr/bin/env bash

# See https://github.com/codecov/example-cpp11-cmake/blob/master/.travis.yml

if [ "$GCOV" != "" ]; then
   lcov --gcov-tool $GCOV --capture --directory src/bm_sim/.libs/ --output-file coverage.info
   lcov --remove coverage.info '/usr/*' --output-file coverage.info
   lcov --remove coverage.info 'third_party/*' --output-file coverage.info
   bash <(curl -s https://codecov.io/bash) -f coverage.info
fi

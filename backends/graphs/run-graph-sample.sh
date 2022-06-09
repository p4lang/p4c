#!/usr/bin/env bash

shift # drop path to p4c source dir
exec ./p4c-graphs "$@" # exec, therefore no need to propagate errors

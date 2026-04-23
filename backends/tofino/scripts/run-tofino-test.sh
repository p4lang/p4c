#!/bin/bash

repo="$1"; shift

exec ./p4c --target tofino --std 14 "$@"

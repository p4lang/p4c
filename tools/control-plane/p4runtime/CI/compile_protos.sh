#!/usr/bin/env bash

# This script is meant for CI testing, to ensure that the P4Runtime Protobuf
# files are correct and compile with the protoc compiler (CPP, gRPC and Python).

if [ $# -ne 1 ]; then
    echo "Usage: compile_protos.sh <BUILD_DIR>"
    exit 1
fi

BUILD_DIR=$1
PROTOC=$(which protoc)
if [ $? -ne 0 ]; then
    echo "Could not find protoc"
    exit 2
fi

echo "Using $PROTOC"

GRPC_CPP_PLUGIN=$(which grpc_cpp_plugin)
if [ $? -ne 0 ]; then
    echo "Could not find CPP protoc plugin"
    exit 2
fi
GRPC_PY_PLUGIN=$(which grpc_python_plugin)
if [ $? -ne 0 ]; then
    echo "Could not find Python protoc plugin"
    exit 2
fi

set -e

THIS_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
PROTO_DIR=$THIS_DIR/../proto

tmpdir=$(mktemp -d)
pushd $tmpdir > /dev/null
git clone --depth 1 https://github.com/googleapis/googleapis.git
popd > /dev/null
GOOGLE_PROTO_DIR=$tmpdir/googleapis

PROTOS="\
$PROTO_DIR/p4/v1/p4data.proto \
$PROTO_DIR/p4/v1/p4runtime.proto \
$PROTO_DIR/p4/config/v1/p4info.proto \
$PROTO_DIR/p4/config/v1/p4types.proto \
$GOOGLE_PROTO_DIR/google/rpc/status.proto \
$GOOGLE_PROTO_DIR/google/rpc/code.proto"

PROTOFLAGS="-I$GOOGLE_PROTO_DIR -I$PROTO_DIR"

mkdir -p $BUILD_DIR/cpp_out
mkdir -p $BUILD_DIR/grpc_out
mkdir -p $BUILD_DIR/py_out

set -o xtrace
$PROTOC $PROTOS --cpp_out $BUILD_DIR/cpp_out $PROTOFLAGS
$PROTOC $PROTOS --grpc_out $BUILD_DIR/grpc_out --plugin=protoc-gen-grpc=$GRPC_CPP_PLUGIN $PROTOFLAGS
# With the Python plugin, it seems that I need to use a single command for proto
# + grpc and that the output directory needs to be the same (because the grpc
# plugin inserts code into the proto-generated files). But maybe I am just using
# an old version of the Python plugin.
$PROTOC $PROTOS --python_out $BUILD_DIR/py_out $PROTOFLAGS --grpc_out $BUILD_DIR/py_out --plugin=protoc-gen-grpc=$GRPC_PY_PLUGIN
set +o xtrace

rm -rf $tmpdir

echo "SUCCESS"

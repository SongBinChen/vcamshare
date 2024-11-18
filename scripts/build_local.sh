#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
BUILD_DIR=$SCRIPT_DIR/../build/local/

# rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

cmake ../..
make VERBOSE=1

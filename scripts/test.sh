#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
cd $SCRIPT_DIR

./build_local.sh

if [ $? -eq 0 ]
then
    cd $SCRIPT_DIR/../build/local
    ./test --run_test=MuxerTest
    # UtilsTest AudioTest MuxerTest
fi

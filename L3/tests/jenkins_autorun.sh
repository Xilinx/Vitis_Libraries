#/bin/bash

TEST_DIR=./out_test

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
fi

source set_env.sh

PYTHON=python
PYTEST=./python/run_test.py

$PYTHON $PYTEST --operator gemm gemv --shell vcu1525_dynamic_5_1


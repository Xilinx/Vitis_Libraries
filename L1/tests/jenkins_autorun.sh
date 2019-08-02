#/bin/bash

TEST_DIR=./out_test

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
fi

PYTHON=python3
PYTEST=./sw/python/run_test.py

$PYTHON $PYTEST --operator amax amin asum axpy dot copy nrm2 scal swap 
$PYTHON $PYTEST --operator gemv gbmv sbmvUp sbmvLo tbmvUp tbmvLo

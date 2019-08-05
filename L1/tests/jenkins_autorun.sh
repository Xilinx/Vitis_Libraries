#/bin/bash

TEST_DIR=./out_test
STAT=./statistics.rpt

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
fi

if [ -f $STAT ]; then
  rm $STAT -rf
fi

source set_env.sh

PYTHON=python3
PYTEST=./sw/python/run_test.py

$PYTHON $PYTEST --operator amax amin asum axpy dot copy nrm2 scal swap 
$PYTHON $PYTEST --operator gemv gbmv sbmvUp sbmvLo tbmvUp tbmvLo

if [ -f $STAT ]; then
  cat $STAT
fi

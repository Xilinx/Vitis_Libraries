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
$PAR=8

$PYTHON $PYTEST --operator amax amin asum axpy copy dot nrm2 scal swap --csim
$PYTHON $PYTEST --operator gemv gbmv sbmvLo sbmvUp symvLo symvUp tbmvLo tbmvUp tbmvLo tbmvUp tpmvLo tpmvUp trmvLo trmvUp --csim

if [ -f $STAT ]; then
  cat $STAT
fi

#/bin/bash

TEST_DIR=./out_test
STAT=./statistics.rpt

if [ -d $TEST_DIR ]; then
  rm $TEST_DIR -rf
  mkdir $TEST_DIR
fi

if [ -f $STAT ]; then
  rm $STAT -rf
fi

source /group/xsjfarm/lsf/conf/profile.lsf
source set_env.sh

PYTHON=python3
PYTEST=./sw/python/run_test.py
PYCHECK=./sw/python/check_process.py
PAR=4

SUBMIT="bsub -cwd `pwd` -q medium -R \"select[(os== lin && type == X86_64 && (osdistro == rhel || osdistro == centos) && (osver== ws7))] rusage[mem=16000]\""
eval "$SUBMIT $PYTHON $PYTEST --operator amax amin asum axpy copy dot nrm2 scal swap --parallel $PAR --id 0 &"
eval "$SUBMIT $PYTHON $PYTEST --operator gemv gbmv sbmvLo sbmvUp tbmvLo tbmvUp trmvLo trmvUp --parallel $PAR --id 1 &"
eval "$SUBMIT $PYTHON $PYTEST --operator symvLo symvUp spmvUp spmvLo tpmvLo tpmvUp --parallel $PAR --csim --id 2 &"
$PYTHON $PYCHECK --number 3

if [ -f $STAT ]; then
  cat $STAT
fi

#!/usr/bin/env bash

JSONDIR="${1}"

function run {
  for tst in ${JSONDIR}/*.json; do
    echo -n "testing $(basename $tst)... "
    ${TESTBIN} <$tst
    res=$?
    if [[ $res -ne 0 ]] ; then
      echo "error code $res"
      exit 1
    else
      echo "OK"
    fi
  done
}

# test using apply(rule, variant_vector) 
#      with apply(rule, data) as fallback
TESTBIN="../build/tests/testeval"
run

# test using always apply(rule, data)
TESTBIN="../build/tests/testeval -s"
run

echo "All tests passed."


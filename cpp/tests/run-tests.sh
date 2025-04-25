#!/usr/bin/env bash

function run {
  for tst in *.json; do
    echo "testing ${TESTBIN} <$tst"
    ${TESTBIN} <$tst
    res=$?
    if [[ $res -ne 0 ]] ; then
      echo "$res"
      exit 1
    fi
  done
}

# test using apply(rule, variant_vector) 
#      with apply(rule, data) as fallback
TESTBIN="../examples/testeval.bin"
run

# test using always apply(rule, data)
TESTBIN="../examples/testeval.bin -s"
run

echo "qed."


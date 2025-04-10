#!/usr/bin/env bash

TESTBIN=../examples/testeval.bin

for tst in *.json; do
  echo "testing $TESTBIN <$tst"
  $TESTBIN <$tst
  res=$?
  if [[ $res -ne 0 ]] ; then
    echo "$res"
    exit 1
  fi
done

echo "qed."


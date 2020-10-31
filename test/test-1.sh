#!/bin/sh
set -eu

# TEST 1

TEST=$( readlink --canonicalize $( dirname $0    ) )
TOP=$(  readlink --canonicalize $( dirname $TEST ) )
PATH=$TOP/bin:$PATH
NAME=$( basename $0 )
cd $TEST/work

stash save t1 -a
STATUS=$?

if [ $STATUS = 0 ]
then
  echo "$NAME: success."
else
  echo "$NAME: FAILED"
  exit 1
fi


#!/bin/sh
set -eu

TEST=$( readlink --canonicalize $( dirname $0    ) )
TOP=$(  readlink --canonicalize $( dirname $TEST ) )
cd $TEST

if ! [ -d SVN ]
then
  svnadmin create SVN
fi

svn co file:///$TEST/SVN work

cp t1 work
cd work
svn add t1
svn commit --message "Add t1" t1

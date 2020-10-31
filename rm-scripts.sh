#!/bin/sh
set -e

if ! rm -v compile depcomp missing install-sh
then
  echo "FAILED"
  exit 1
fi

echo "OK"

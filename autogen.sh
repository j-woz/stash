#!/bin/sh
set -eu

aclocal
automake --add-missing
autoconf

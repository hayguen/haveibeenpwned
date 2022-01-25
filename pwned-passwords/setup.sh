#!/bin/bash

S="$(readlink -nf "$BASH_SOURCE")"
PREFIX="$1"
if [ -z "$PREFIX" ]; then
  echo "usage: $S [<prefix>]"
  echo "prefix argument is missing. using default prefix: '/usr/local'"
  PREFIX="/usr/local"
fi

install -d "$PREFIX/bin"
install -d "$PREFIX/share/haveibeenpwned"
install haveibeenpwned "$PREFIX/bin/"
install pwd-full.srds  "$PREFIX/share/haveibeenpwned/"

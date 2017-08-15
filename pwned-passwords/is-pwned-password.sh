#!/bin/bash

# set default database file and (s)grep command - modify these if necessary
DB="pwd-full.srds"
GBIN=$( which srdsgrep )
G="${GBIN} -m 1 -x"


# set defaults: "verbosity", "print count" and "time duration"
V=0
C=0
T=0

if [ -z "${GBIN}" ]; then
  echo "error: srdsgrep is NOT in PATH!"
  exit 10
fi
if [ ! -f "${GBIN}" ]; then
  echo "error: srdsgrep is not installed at '${GBIN}'!"
  exit 10
fi
if [ ! -x "${GBIN}" ]; then
  echo "error: srdsgrep '${GBIN}' is not executable!"
  exit 10
fi

if [ ! -f "${DB}" ]; then
  echo "error: database file '${DB}' does not exist!"
  echo "  try executing './get-pwned-passwords.sh'"
  exit 10
fi


if [ -z "$1" ]; then
  echo "usage: $0 <password> [-v] [-c] [-t] [<database>]"
  echo "  checks if given password is in the database of exposed/pawned passwords"
  echo "  -v : print verbose output"
  echo "  -c : print 0 / 1 only"
  echo "  -t : time duration of grep"
  #echo "  -g : use 'grep -m 1' searching full file until 1st match"
  #echo "       default is srdsgrep in \$HOME/bin/"
  echo "  database must be last argument!"
  exit 10
fi

# unenrypted unhashed clear password!
P="$1"
shift


while [ ! -z "$1" ]; do
  if [ "$1" == "-v" ]; then
    V=1
    shift
  elif [ "$1" == "-c" ]; then
    C=1
    shift
  elif [ "$1" == "-t" ]; then
    T=1
    shift
#  elif [ "$1" == "-g" ]; then
#    G="grep -m 1"
#    shift
  else
    break
  fi
done

if [ ! -z "$1" ]; then
  DB="$1"
  shift
fi

if [ ! -z "$1" ]; then
  echo "warning: ignoring additional argument '$1' and following!"
fi


# get sha1sum - without trailing zero of echo - and without filename
SHA="$( echo -n "$P" |sha1sum - |awk '{ print $1; }' )"

if [ $V -gt 0 ]; then
  echo "checking password '$P'"
  echo "sha1sum of password is '${SHA}'"
  echo "using sha1 password database '${DB}'"
  echo "using grep command '$G'"
fi

if [ $T -gt 0 ]; then
  # the grep may take some time! time it for comparison:
  R=$( time ${G} -c ${SHA} ${DB} )
  if [ $V -gt 0 ]; then
    echo "result of grep is: '$R'"
  fi
else
  # grep without timing
  R=$( ${G} -c ${SHA} ${DB} )
fi

if [ $C -gt 0 ]; then
  echo $R
else
  if [ $R -gt 0 ]; then
    echo "found password's sha1sum in database: password is pawned!"
  else
    echo "password not in database: password is OK"
  fi
fi

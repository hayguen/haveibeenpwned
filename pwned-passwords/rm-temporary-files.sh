#!/bin/bash

if [ -f pwd-full.srds ]; then

  S="pwned-passwords-sha1-ordered-by-hash-v4.7z"
  if [ -f "$S" ]; then
    TS="$(date -r "$S" +%s)"
    TD="$(date -r pwd-full.srds +%s)"
    if [ $TS -gt $TD ]; then
      echo "error: archive $S is newer than pwd-full.srds!"
      exit 10
    fi
    echo "removing archive $S"
    rm "$S"
  fi

  echo "removing all temporary files - created by get-pwned-passwords.sh"
  rm pwd-1.0.srds
  rm pwd-update1.srds
  rm pwd-update2.srds
  rm pwd-updates.srds
  rm pwned-passwords-1.0.txt.7z
  rm pwned-passwords-update-1.txt.7z
  rm pwned-passwords-update-2.txt.7z

else
  echo "not removing temporary files, cause pwd-full.srds does NOT exist!"
fi


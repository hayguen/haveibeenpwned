#!/bin/bash

if [ -f pwd-full.srds ]; then

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


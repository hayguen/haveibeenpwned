#!/bin/bash

DBVER="Version 8"
HASH_N="pwned-passwords-sha1-ordered-by-hash-v8"
ARCH_SHA="3499a3f82bb94f62cbd9bc782d6d20324e7cde8e"

echo "continue/download compressed 7zip archive ${DBVER} - ordered by hash"
wget -c "https://downloads.pwnedpasswords.com/passwords/${HASH_N}.7z"

echo ""
echo "verify donwloaded files' integrity via SHA1 hash values against ${ARCH_SHA}"
echo "  https://haveibeenpwned.com/Passwords"
sha1sum ${HASH_N}.7z

echo ""
echo "listing contents of 7z archive"
7z l ${HASH_N}.7z
#exit 0

echo ""
echo "check first 50 lines .. whilst decompressing full password file?"
7z x -so ${HASH_N}.7z ${HASH_N}.txt | head -n 50 >first50.txt

echo ""
echo "decompress ${HASH_N}.7z     and convert to binary .."
7z x -so ${HASH_N}.7z ${HASH_N}.txt | hex2rds -B 4096 -o pwd-full.srds
echo "${HASH_N}.7z" >pwd-full.srds.source.txt
# if database would be compressed with gz or bz2 we could curl directly pipe into gunzip / bunzip2


if /bin/false ; then
# updates/merging not necessary with full single-download
echo "merging updates 1 & 2 .."
srdsmerge -vv -l 20 -o pwd-updates.srds  pwd-update1.srds pwd-update2.srds
echo "merging 1.0 database with updates .. this takes some time .."
srdsmerge -vv -l 20 -o pwd-full.srds     pwd-1.0.srds     pwd-updates.srds

#od -A n -t x1 -w20 -v pwd-updates.srds | sed 's/ //g' | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | sort -c
#od -A n -t x1 -w20 -v pwd-full.srds    | head -n 20
fi

echo ""
echo "test sort order of result file: pwd-full.srds .. this takes some time .."
# od -A n -t x1 -w20 -v pwd-full.srds | sort -c  # this is really slow!
srdscheck -v -l 20 pwd-full.srds

echo ""
echo "test fulfilled - all ok, if there was no output"

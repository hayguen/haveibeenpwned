#!/bin/bash

echo "continue/download compressed 7zip archive v4 ordered by hash"
wget -c https://downloads.pwnedpasswords.com/passwords/pwned-passwords-sha1-ordered-by-hash-v4.7z

echo "verify donwloaded files' integrity via SHA1 hash values against d81c649cda9cddb398f2b93c629718e14b7f2686"
echo "  https://haveibeenpwned.com/Passwords"
sha1sum pwned*.7z

echo "listing contents of 7z archive"
7z l pwned-passwords-sha1-ordered-by-hash-v4.7z
#exit 0

echo "check first 50 lines .. whilst decompressing full password file?"
7z x -so pwned-passwords-sha1-ordered-by-hash-v4.7z pwned-passwords-sha1-ordered-by-hash-v4.txt | head -n 50 >first50.txt


echo "decompress pwned-passwords-1.0.txt.7z       and convert to binary .."
7z x -so pwned-passwords-sha1-ordered-by-hash-v4.7z pwned-passwords-sha1-ordered-by-hash-v4.txt | hex2rds -B 4096 -o pwd-full.srds
echo "pwned-passwords-sha1-ordered-by-hash-v4.7z" >pwd-full.srds.source.txt
# if database would be compressed with gz or bz2 we could curl directly pipe into gunzip / bunzip2


if /bin/false ; then
# updates/merging not necessary with full single-download of v4
echo "merging updates 1 & 2 .."
srdsmerge -vv -l 20 -o pwd-updates.srds  pwd-update1.srds pwd-update2.srds
echo "merging 1.0 database with updates .. this takes some time .."
srdsmerge -vv -l 20 -o pwd-full.srds     pwd-1.0.srds     pwd-updates.srds

#od -A n -t x1 -w20 -v pwd-updates.srds | sed 's/ //g' | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | sort -c
#od -A n -t x1 -w20 -v pwd-full.srds    | head -n 20
fi

echo "test sort oder of result file: pwd-full.srds .. this takes some time .."
od -A n -t x1 -w20 -v pwd-full.srds | sort -c

echo "test fulfilled - all ok, if there was no output"

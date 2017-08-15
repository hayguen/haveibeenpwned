#!/bin/bash

echo "continue/download compressed 7zip archives .."
wget -c https://downloads.pwnedpasswords.com/passwords/pwned-passwords-1.0.txt.7z
wget -c https://downloads.pwnedpasswords.com/passwords/pwned-passwords-update-1.txt.7z
wget -c https://downloads.pwnedpasswords.com/passwords/pwned-passwords-update-2.txt.7z

echo "verify donwloaded files' integrity via SHA1 hash values against"
echo "  https://haveibeenpwned.com/Passwords"
sha1sum pwned*.7z

echo "decompress pwned-passwords-1.0.txt.7z       and convert to binary .."
7z x -so pwned-passwords-1.0.txt.7z      pwned-passwords-1.0.txt      | hex2rds -o pwd-1.0.srds
echo "decompress pwned-passwords-update-1.txt.7z  and convert to binary .."
7z x -so pwned-passwords-update-1.txt.7z pwned-passwords-update-1.txt | hex2rds -o pwd-update1.srds
echo "decompress pwned-passwords-update-2.txt.7z  and convert to binary .."
7z x -so pwned-passwords-update-2.txt.7z pwned-passwords-update-2.txt | hex2rds -o pwd-update2.srds

# if database would be compressed with gz or bz2 we could curl directly pipe into gunzip / bunzip2

echo "merging updates 1 & 2 .."
srdsmerge -vv -l 20 -o pwd-updates.srds  pwd-update1.srds pwd-update2.srds
echo "merging 1.0 database with updates .. this takes some time .."
srdsmerge -vv -l 20 -o pwd-full.srds     pwd-1.0.srds     pwd-updates.srds

#od -A n -t x1 -w20 -v pwd-updates.srds | sed 's/ //g' | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | head -n 20
#od -A n -t x1 -w20 -v pwd-updates.srds | sort -c
#od -A n -t x1 -w20 -v pwd-full.srds    | head -n 20

echo "test sort oder of result file: pwd-full.srds .. this takes some time .."
od -A n -t x1 -w20 -v pwd-full.srds | sort -c

echo "test fulfilled - all ok, if there was no output"

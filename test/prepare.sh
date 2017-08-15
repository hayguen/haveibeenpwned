#!/bin/bash

# each raw data set with 7 bytes
# the key start at offset 3 and has length 3 (bytes)
echo -n -e "xyz001\n"  >1.srds
echo -n -e "abc002\n" >>1.srds
echo -n -e "cde004\n" >>1.srds
echo -n -e "cde005\n" >>1.srds
echo -n -e "zyx005\n" >>1.srds
echo -n -e "efg006\n" >>1.srds

echo -n -e "xyz000\n"  >2.srds
echo -n -e "abc003\n" >>2.srds
echo -n -e "efg007\n" >>2.srds
echo -n -e "efg008\n" >>2.srds

cp ../build/hex2rds   ~/bin/
cp ../build/srdsgrep  ~/bin/
cp ../build/srdsmerge ~/bin/

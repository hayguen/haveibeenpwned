#!/bin/bash

source prepare.sh

#OPTS="-vv -b 3 -l 7"
OPTS="-b 3 -l 7"

echo -e "\n\ntest 1: expected result: no match for 003!"
srdsgrep -c ${OPTS} "003" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "003" 1.srds


echo -e "\n\ntest 2: expected result: 1 match for 002"
srdsgrep -c ${OPTS} "002" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "002" 1.srds

echo -e "\n\ntest 3: expected result: 2 matches for 005"
srdsgrep -c ${OPTS} "005" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "005" 1.srds

echo -e "\n\ntest 4: expected result: 2 matches for 005 - but only 1 output"
srdsgrep -c -m 1 ${OPTS} "005" 1.srds
echo -e "\ncontents:"
srdsgrep -m 1 ${OPTS} "005" 1.srds

echo -e "\n\ntest 5: as test3 with hexadecimal key for 005"
srdsgrep -c ${OPTS} -x "30 30 35" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} -x "30 30 35" 1.srds


echo -e "\n\ntest 6: expected result: 1 match for 001 (first entry)"
srdsgrep -c ${OPTS} "001" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "001" 1.srds

echo -e "\n\ntest 7: expected result: 1 match for 006 (last entry)"
srdsgrep -c ${OPTS} "006" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "006" 1.srds


echo -e "\n\ntest 8: expected result: no match for 000 (before first entry)"
srdsgrep -c ${OPTS} "000" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "000" 1.srds

echo -e "\n\ntest 9: expected result: no match for 007 (after last entry)"
srdsgrep -c ${OPTS} "007" 1.srds
echo -e "\ncontents:"
srdsgrep ${OPTS} "007" 1.srds

echo -e "\n\ntest 10: expected result: 2 matches for 005 with input from cat pipe"
cat 1.srds |srdsgrep -c ${OPTS} "005"

echo -e "\n\ntest 11: expected result: 2 matches for 005 with input from pipe"
srdsgrep -c ${OPTS} "005" <1.srds

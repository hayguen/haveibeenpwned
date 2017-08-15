#!/bin/bash

source prepare.sh

OPTS="-l 7 -b 3 -e 5"

srdsmerge ${OPTS} -o 12.srds 1.srds 2.srds
cat 12.srds

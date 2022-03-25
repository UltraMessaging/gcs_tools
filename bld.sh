#!/bin/sh
# bld.sh - build the programs on Linux.

# User must have environment set up:
# LBM - path for your LBM platform. Eg: "$HOME/UMP_6.14/Linux-glibc-2.17-x86_64"
# LD_LIBRARY_PATH - path for your LBM libraries. Eg: "$LBM/lib"
# LBM_LICENSE_INFO - license key. Eg: "Product=LBM:Organization=Your org:Expiration-Date=never:License-Key=..."

# For Linux
LIBS="-l pthread -l m -l rt"

rm -rf linux64_bin linux64_bin.tz
mkdir linux64_bin

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o linux64_bin/lbmmrcv verifymsg.c lbmmrcv.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o linux64_bin/lbmmsrc verifymsg.c lbmmsrc.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o linux64_bin/lbmrcv verifymsg.c lbmrcv.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o linux64_bin/lbmsrc verifymsg.c lbmsrc.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o linux64_bin/lbmssrc verifymsg.c lbmssrc.c

tar czf linux64_bin.tz linux64_bin

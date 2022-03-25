#!/bin/sh
# bld.sh - build the programs on Linux.

# User must have environment set up:
# LBM - path for your LBM platform. Eg: "$HOME/UMP_6.14/Linux-glibc-2.17-x86_64"
# LD_LIBRARY_PATH - path for your LBM libraries. Eg: "$LBM/lib"
# LBM_LICENSE_INFO - license key. Eg: "Product=LBM:Organization=Your org:Expiration-Date=never:License-Key=..."

# For Linux
LIBS="-l pthread -l m -l rt"

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o lbmmrcv verifymsg.c lbmmrcv.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o lbmmsrc verifymsg.c lbmmsrc.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o lbmrcv verifymsg.c lbmrcv.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o lbmsrc verifymsg.c lbmsrc.c

gcc -Wall -g -I $LBM/include -I $LBM/include/lbm -L $LBM/lib -l lbm $LIBS \
    -o lbmssrc verifymsg.c lbmssrc.c

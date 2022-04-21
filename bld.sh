#!/bin/sh
# bld.sh - build the programs on Linux.

source ../lbm.sh
LD_LIBRARY_PATH=$LBM671/lib; export LD_LIBRARY_PATH

# For Linux
LIBS="-l pthread -l m -l rt"

rm -rf linux64_bin linux64_bin.tz
mkdir linux64_bin

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsmrcv verifymsg.c gcsmrcv.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsmsrc verifymsg.c gcsmsrc.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsrcv verifymsg.c gcsrcv.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcssrc verifymsg.c gcssrc.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsurcv verifymsg.c gcsurcv.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsusrc verifymsg.c gcsusrc.c

tar czf linux64_bin.tz linux64_bin

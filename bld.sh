#!/bin/sh
# bld.sh - build the programs on Linux.

for F in *.md; do :
  if egrep "<!-- mdtoc-start -->" $F >/dev/null; then :
    # Update doc table of contents (see https://github.com/fordsfords/mdtoc).
    if which mdtoc.pl >/dev/null; then mdtoc.pl -b "" $F;
    elif [ -x ../mdtoc/mdtoc.pl ]; then ../mdtoc/mdtoc.pl -b "" $F;
    else echo "FYI: mdtoc.pl not found; see https://github.com/fordsfords/mdtoc"; exit 1
    fi
  fi
done

source ../lbm.sh
LD_LIBRARY_PATH=$LBM671/lib; export LD_LIBRARY_PATH

# For Linux
LIBS="-l pthread -l m -l rt"

rm -rf linux64_bin linux64_bin.zip
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

gcc -Wall -g \
    -o linux64_bin/gcsmdump gcsmdump.c

gcc -Wall -g \
    -o linux64_bin/gcsmsend gcsmsend.c

gcc -Wall -g -l m \
    -o linux64_bin/gcsmpong gcsmpong.c

gcc -Wall -g -I $LBM671/include -I $LBM671/include/lbm -L $LBM671/lib -l lbm $LIBS \
    -o linux64_bin/gcsdumpxml gcsdumpxml.c

zip -qr linux64_bin.zip linux64_bin

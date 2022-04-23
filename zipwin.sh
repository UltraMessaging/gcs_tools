#!/bin/sh
# zipwin.sh - zip up the windows binaries (for use in Cygwin on host Winbuild).

rm -rf win64_bin
mkdir win64_bin
cp ../Documents/Visual\ Studio\ 2012/Projects/gcs_tools/x64/Debug/*.exe win64_bin/

zip -qr win64_bin.zip win64_bin

cd ../Documents/Visual\ Studio\ 2012/Projects/
tar -czf ../../../gcs_tools/gcs_tools-solution-backup.tz gcs_tools

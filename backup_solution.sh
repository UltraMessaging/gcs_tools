#!/bin/sh
# backup_solution.sh - tar up the solution and project files for gcs_tools

cd ../Documents/Visual\ Studio\ 2012/Projects/
rm -rf `find gcs_tools -name 'Debug' -print`
tar -czf ../../../gcs_tools/gcs_tools-solution-backup.tz gcs_tools

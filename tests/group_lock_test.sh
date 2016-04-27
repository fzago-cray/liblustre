#!/bin/sh

# This test crashes Lustre 2.5 client. Enable it only for 2.7+
LMAJOR=$(head -1 /proc/fs/lustre/version | sed -e "s/lustre: \([0-9]\+\)\.[0-9]\+\.[0-9]\+/\1/")
LMINOR=$(head -1 /proc/fs/lustre/version | sed -e "s/lustre: [0-9]\+\.\([0-9]\+\)\.[0-9]\+/\1/")
[[ $LMAJOR < 2 ]] && exit 77
[[ $LMINOR < 7 ]] && exit 77

./group_lock_test -d ${MOUNT:-/mnt/${FSNAME:-lustre}}

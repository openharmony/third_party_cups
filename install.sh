#!/bin/bash
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation version 2.1
# of the License.
#
# Copyright(c) 2023 Huawei Device Co., Ltd.

set -e
cd $1
if [ -d "cups-2.4.0" ];then
    rm -rf cups-2.4.0
fi
tar xvf cups-2.4.0-source.tar.gz
cd $1/cups-2.4.0
patch -p1 < $1/backport-CVE-2022-26691.patch --fuzz=0 --no-backup-if-mismatch
patch -p1 < $1/backport-CVE-2023-32324.patch --fuzz=0 --no-backup-if-mismatch
patch -p1 < $1/backport-CVE-2023-34241.patch --fuzz=0 --no-backup-if-mismatch
patch -p1 < $1/cups_single_file.patch --fuzz=0 --no-backup-if-mismatch
patch -p1 < $1/pthread_cancel.patch --fuzz=0 --no-backup-if-mismatch
exit 0
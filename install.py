#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import tarfile
import argparse
import os
import subprocess
import sys
import shutil


def untar_file(tar_file_path, extract_path):
    try:
        tar_cmd = ['tar', '-zxf', tar_file_path, '-C', extract_path]
        subprocess.run(tar_cmd, check=True)
    except Exception as e:
        print("tar error!")
        return

def copy_file(dir):
    src_name = '/mime.convs.in'
    dest_name = '/mime.convs'
    src_file = dir + src_name
    dest_file = dir + dest_name
    print(f'copy from %s to %s', src_file, dest_file)
    shutil.copy2(src_file, dest_file)

def move_file(src_path, dst_path):
    files = [
        "ohos_ip_conflict.patch",
        "backport-CVE-2022-26691.patch",
        "backport-CVE-2023-32324.patch",
        "backport-CVE-2023-34241.patch",
        "ohos-multi-file-print.patch",
        "ohos-modify-pthread.patch",
        "ohos-add-openssl.patch",
        "backport-CVE-2023-4504.patch",
        "backport-CVE-2024-35235.patch",
        "ohos-usb-manager.patch",
        "ohos-usb-print.patch",
        "ohos-ppdfile-not-generated.patch",
        "ohos-hilog-print.patch",
        "ohos-uni-print-driver-path.patch",
        "config.h"
    ]
    for file in files:
        src_file = os.path.join(src_path, file)
        dst_file = os.path.join(dst_path, file)
        shutil.copy(src_file, dst_file)


def apply_patch(patch_file, target_dir):
    try:
        if not os.path.exists(target_dir):
            return
        patch_cmd = ['patch', '-p1', "--fuzz=0", "--no-backup-if-mismatch", '-i', patch_file, '-d', target_dir]
        subprocess.run(patch_cmd, check=True)
    except Exception as e:
        print(f"apply_patch error! patch: {patch_file}")
        return


def do_patch(target_dir):
    patch_file = [
        "backport-CVE-2022-26691.patch",
        "backport-CVE-2023-32324.patch",
        "backport-CVE-2023-34241.patch",
        "ohos-multi-file-print.patch",
        "ohos-modify-pthread.patch",
        "ohos-add-openssl.patch",
        "backport-CVE-2023-4504.patch",
        "backport-CVE-2024-35235.patch",
        "ohos_ip_conflict.patch",
        "ohos-usb-manager.patch",
        "ohos-usb-print.patch",
        "ohos-ppdfile-not-generated.patch",
        "ohos-hilog-print.patch",
        "ohos-uni-print-driver-path.patch"
    ]

    for patch in patch_file:
        apply_patch(patch, target_dir)


def main():
    cups_path = argparse.ArgumentParser()
    cups_path.add_argument('--gen-dir', help='generate path of log', required=True)
    cups_path.add_argument('--source-dir', help='generate path of log', required=True)
    args = cups_path.parse_args()
    tar_file_path = os.path.join(args.source_dir, "cups-2.4.0-source.tar.gz")
    target_dir = os.path.join(args.gen_dir, "cups-2.4.0")
    convs_dir = os.path.join(target_dir, "conf")

    untar_file(tar_file_path, args.gen_dir)
    move_file(args.source_dir, target_dir)
    do_patch(target_dir)
    copy_file(convs_dir)
    return 0

if __name__ == '__main__':
    sys.exit(main())

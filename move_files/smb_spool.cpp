/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unistd.h>
#include <fcntl.h>
#include "securec.h"
#include "smb_log.h"
#include "smb_spool.h"

static constexpr int32_t FILE_NAME_SIZE = 256;
static constexpr int32_t FILE_BUFFER_NAME_SIZE = 65536;
static constexpr int32_t SMB_CONNECT_TIMEOUT_MS = 2000;

SMBSpool::SMBSpool() : smb2_(nullptr), connected_(false)
{
    smb2_ = smb2_init_context();
    if (!smb2_) {
        OHHILOG("DEBUG: SmbBackend Failed to initialize libsmb2 context\n");
    }
}

SMBSpool::~SMBSpool() 
{
    Disconnect();
    if (smb2_) {
        smb2_destroy_context(smb2_);
        smb2_ = nullptr;
    }
}

bool SMBSpool::ConnectToShare(const ParsedURI& uri, const char* password)
{
    if (!smb2_) {
        OHHILOG("DEBUG: SmbBackend smb2_ is null\n");
        return false;
    }

    if (!uri.username.empty()) {
        smb2_set_user(smb2_, uri.username.c_str());
    }
    if (password) {
        smb2_set_password(smb2_, password);
    }
    smb2_set_timeout(smb2_, SMB_CONNECT_TIMEOUT_MS);
    int32_t result = smb2_connect_share(smb2_, uri.server.c_str(), uri.share.c_str(), 
        uri.username.empty() ? nullptr : uri.username.c_str());
    
    if (result != 0) {
        OHHILOG("DEBUG: SmbBackend smb2_connect_share fail, ret = %d, reason = %s\n",
            result, smb2_get_error(smb2_));
        return false;
    }

    connected_ = true;
    return true;
}

void SMBSpool::Disconnect()
{
    if (connected_ && smb2_) {
        smb2_disconnect_share(smb2_);
        connected_ = false;
    }
}

bool SMBSpool::PrintFile(FILE* file)
{
    if (!smb2_) {
        OHHILOG("DEBUG: SmbBackend smb2_ is null\n");
        return false;
    }
    if (!connected_) {
        OHHILOG("DEBUG: SmbBackend not connected\n");
        return false;
    }
    if (!SendPrintData(file)) {
        OHHILOG("DEBUG: SendPrintData fail\n");
        return false;
    }
    
    return true;
}

bool SMBSpool::SendPrintData(FILE* file)
{
    if (!file) {
        OHHILOG("DEBUG: SendPrintData file is null\n");
        return false;
    }
    char remotePath[FILE_NAME_SIZE] = {'\0'};
    if (snprintf_s(remotePath ,sizeof(remotePath), sizeof(remotePath) - 1, "smb-job-%ld",
        static_cast<long>(time(nullptr))) < 0) {
        OHHILOG("DEBUG: SendPrintData set file name fail\n");
        return false;
    }
    struct smb2fh* fileHandle = smb2_open(smb2_, remotePath, O_CREAT | O_WRONLY | O_TRUNC);
    if (!fileHandle) {
        OHHILOG("DEBUG: SmbBackend Failed to open remote spool file, reason: %s\n", smb2_get_error(smb2_));
        return false;
    }
    uint8_t buffer[FILE_BUFFER_NAME_SIZE] = {'\0'};
    uint32_t bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        int32_t bytesWritten = smb2_write(smb2_, fileHandle, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            OHHILOG("DEBUG: SmbBackend Failed to write to printer\n");
            smb2_close(smb2_, fileHandle);
            return false;
        }
    }
    smb2_close(smb2_, fileHandle);
    return true;
}
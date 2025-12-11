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
#ifndef SMB_SPOOL_H
#define SMB_SPOOL_H

#include <string>
#include "uri_processor.h"
#include "smb2/smb2.h"
#include "smb2/libsmb2.h"
#include "smb2/smb2-errors.h"
#include "smb2/libsmb2-raw.h"

class SMBSpool {
public:
    SMBSpool();
    ~SMBSpool();
    bool ConnectToShare(const ParsedURI& uri, const char* password);
    bool PrintFile(FILE* file);

private:
    bool SendPrintData(FILE* file);
    void Disconnect();
    struct smb2_context* smb2_;
    bool connected_;

};

#endif  // SMB_SPOOL_H
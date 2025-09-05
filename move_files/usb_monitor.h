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

#ifndef USB_MONITOR_H
#define USB_MONITOR_H

#include <stdint.h>
#include <cups/ipp.h>

#define PRINTER_STATE_REASONS_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ipp_pstate_t printerState;
    char printerStateReasons[PRINTER_STATE_REASONS_SIZE];
} PrinterStatus;

typedef void (MonitorPrinterCallback)(PrinterStatus* jobData);

bool StartMonitorIppPrinter(MonitorPrinterCallback callback, const char* uri);

bool IsSupportIppOverUsb(const char* uri);

void SetTerminalSingal(void);

void ComparePrinterStateReasons(const char* oldReasons, const char* newReasons,
    char** addedReasons, char** deletedReasons);

void FreeCompareStringsResult(char** addedReasons, char** deletedReasons);
#ifdef __cplusplus
}
#endif

#endif  // USB_MONITOR_H
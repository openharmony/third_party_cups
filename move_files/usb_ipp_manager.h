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

#ifndef USB_IPP_MANAGER_H
#define USB_IPP_MANAGER_H

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include "usb_srv_client.h"
#include "v1_0/iusb_interface.h"
#include "usb_monitor.h"

namespace OHOS::CUPS {
using namespace OHOS::USB;
constexpr int32_t INVAILD_VALUE = -1;
constexpr uint8_t USB_CLASS_PRINTER = 0x07;
constexpr uint8_t USB_PRINTER_SUBCLASS_IPP = 0x01;
constexpr uint8_t USB_PRINTER_PROTOCOL_IPP = 0x04;
constexpr int32_t WRITE_RETRY_MAX_TIMES = 20;
constexpr int32_t USB_WRITE_INTERVAL = 50;
constexpr int32_t EORROR_HDF_DEV_ERR_TIME_OUT = -7;
constexpr int32_t EORROR_HDF_DEV_ERR_NO_DEVICE = -202;
constexpr int32_t USB_DEVICE_CLASS_PRINT = 7;
constexpr int32_t USB_DEVICE_SUBCLASS_PRINT = 1;
constexpr int32_t USB_BULKTRANSFER_READ_TIMEOUT = 500;
constexpr int32_t USB_BULKTRANSFER_WRITE_TIMEOUT = 500;
constexpr int32_t USB_DEVICE_PROTOCOL_PRINT = 4;
constexpr int32_t USB_READ_INTERVAL = 50;
constexpr int32_t INDEX_0 = 0;
constexpr int32_t INDEX_1 = 1;
constexpr int32_t CLAIM_INTERFACE_RETRY_MAX_TIMES = 5;
constexpr int32_t EORROR_HDF_DEV_ERR_IO_FAILURE = -1;

struct PrinterTranIndex {
    int32_t configIndex = INVAILD_VALUE;
    int32_t interfaceIndex = INVAILD_VALUE;
};

struct BufferContext {
    uint8_t* data = nullptr;
    size_t size = INDEX_0;
    size_t pos = INDEX_0;
};

struct UsbPrinter {
    USBDevicePipe ippPipe;
    PrinterTranIndex tranIndex; // allocated ipp interface
    std::vector<PrinterTranIndex> indexVec;
    UsbDevice device;
    bool isOpened = false;
    bool isSupportIpp = false;
};

class IppUsbManager {
public:
    static IppUsbManager& GetInstance();
    IppUsbManager(const IppUsbManager&) = delete;
    IppUsbManager& operator=(const IppUsbManager&) = delete;
    IppUsbManager(IppUsbManager&&) = delete;
    IppUsbManager& operator=(IppUsbManager&&) = delete;

    bool IsSupportIppOverUsb(const std::string& uri);
    bool ConnectUsbPinter(const std::string& uri);
    bool DisConnectUsbPinter(const std::string& uri);
    bool ProcessMonitorPrinter(const std::string& uri, MonitorPrinterCallback callback);
    void SetTerminalSingal();
private:
    IppUsbManager();
    ~IppUsbManager();

    bool FindUsbPrinter(UsbDevice& device, const std::string& uri);
    bool ProcessDataFromDevice(const std::string& uri, PrinterStatus& printerStatus);
    int32_t BulkTransferRead(const std::string& uri, std::vector<uint8_t>& readTempBUffer);
    int32_t BulkTransferWrite(const std::string& uri, std::vector<uint8_t>& vectorRequestBuffer);
    bool AllocateInterface(const std::string &uri, UsbDevice& usbdevice,
        USBDevicePipe &usbDevicePipe);
    bool ParseIppResponse(std::vector<uint8_t>& responseData, PrinterStatus& printerStatus);
    std::vector<uint8_t> BuildIppRequest();
    static ssize_t ReadFromBuffer(void* ctx, void* data, size_t len);
    static ssize_t WriteToBuffer(void* ctx, const void* data, size_t len);
    bool IsPrintJobAbortNormally(PrinterStatus& printerStatus);
    bool IsContainsHttpHeader(const std::vector<uint8_t>& data);

    std::map<std::string, UsbPrinter> ippPrinterMap_;
    std::atomic<bool> isTerminated_;
    std::mutex lock_;
};
} // namespace OHOS::CUPS
#endif // USB_IPP_MANAGER_H
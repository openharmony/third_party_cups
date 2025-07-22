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

#include <thread>
#include <cups/ipp.h>
#include <cups/cups-private.h>
#include <securec.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include "usb_manager.h"
#include "usb_errors.h"
#include "usb_ipp_manager.h"

namespace OHOS::CUPS {
constexpr int32_t errorReasonCount = 19;
static constexpr std::array<const char*, errorReasonCount> ippPrinterErrorReason = {
    "other",
    "cover-open",
    "input-tray-missing",
    "marker-supply-empty",
    "marker-supply-low",
    "marker-waste-almost-full",
    "marker-waste-full",
    "media-empty",
    "media-jam",
    "media-low",
    "media-needed",
    "moving-to-paused",
    "paused",
    "spool-area-full",
    "toner-empty",
    "toner-low",
    "offline",
    "marker-ink-almost-empty",
    "door-open"
};
static auto &usbSrvClient = UsbSrvClient::GetInstance();
IppUsbManager::IppUsbManager()
{
    isTerminated_.store(false);
}

IppUsbManager::~IppUsbManager()
{
    std::lock_guard<std::mutex> autoLock(lock_);
    for (auto [uri, ippPrinter] : ippPrinterMap_) {
        if (ippPrinter.isOpened) {
            usbSrvClient.Close(ippPrinter.ippPipe);
        }
    }
    ippPrinterMap_.clear();
}

IppUsbManager& IppUsbManager::GetInstance()
{
    static IppUsbManager instance;
    return instance;
}

bool IppUsbManager::FindUsbPrinter(UsbDevice& device, const std::string& uri)
{
    std::vector<UsbDevice> devlist;
    auto getDevRet = usbSrvClient.GetDevices(devlist);
    if (getDevRet != OHOS::ERR_OK) {
        fprintf(stderr, "DEBUG: USB_MONITOR GetDevices fail, ret = %d\n", getDevRet);
        return false;
    }
    if (devlist.empty()) {
        fprintf(stderr, "DEBUG: USB_MONITOR No usb devices\n");
        return false;
    }
    for (size_t i = 0; i < devlist.size(); i++) {
        ohusb_pipe pipe;
        pipe.busNum = devlist[i].GetBusNum();
        pipe.devAddr = devlist[i].GetDevAddr();
        constexpr int32_t MAX_SN_LENGTH = 256;
        char tempsern[MAX_SN_LENGTH] = {};
        int32_t length = OH_GetStringDescriptor(&pipe, devlist[i].GetiSerialNumber(),
            reinterpret_cast<unsigned char *>(tempsern), sizeof(tempsern) - 1);
        std::string sn;
        if (length <= 0) {
            fprintf(stderr, "DEBUG: USB_MONITOR Not find sn\n");
            continue;
        }
        sn = std::string(tempsern);
        if (uri.find(sn) == std::string::npos) {
            continue;
        }
        device = devlist[i];
        fprintf(stderr, "DEBUG: USB_MONITOR find printer\n");
        return true;
    }
    fprintf(stderr, "DEBUG: USB_MONITOR not find sn\n");
    return false;
}

bool IppUsbManager::IsSupportIppOverUsb(const std::string& uri)
{
    {
        std::lock_guard<std::mutex> autoLock(lock_);
        auto it = ippPrinterMap_.find(uri);
        if (it != ippPrinterMap_.end() && it->second.isSupportIpp) {
            fprintf(stderr, "DEBUG: USB_MONITOR Support has been checked\n");
            return true;
        }
    }
    UsbDevice usbDevice;
    if (!FindUsbPrinter(usbDevice, uri)) {
        fprintf(stderr, "DEBUG: USB_MONITOR FindUsbPrinter fail\n");
        return false;
    }
    int32_t configCount = usbDevice.GetConfigCount();
    std::vector<PrinterTranIndex> indexVec;
    for (int32_t configIndex = 0; configIndex < configCount; configIndex++) {
        int32_t interfaceCount = static_cast<int32_t>(usbDevice.GetConfigs()[configIndex].GetInterfaceCount());
        for (int32_t interfaceIndex = 0; interfaceIndex < interfaceCount; interfaceIndex++) {
            UsbInterface usbInterface = usbDevice.GetConfigs()[configIndex].GetInterfaces()[interfaceIndex];
            bool isSupportIpp = (usbInterface.GetClass() == USB_DEVICE_CLASS_PRINT &&
                usbInterface.GetSubClass() == USB_DEVICE_SUBCLASS_PRINT &&
                usbInterface.GetProtocol() == USB_DEVICE_PROTOCOL_PRINT);
            if (isSupportIpp) {
                PrinterTranIndex indexPair;
                indexPair.configIndex = configIndex;
                indexPair.interfaceIndex = interfaceIndex;
                indexVec.push_back(indexPair);
            }
        }
    }
    constexpr int32_t USB_INTERFACE_MIN_COUNT = 1;
    if (indexVec.size() >= USB_INTERFACE_MIN_COUNT) {
        std::lock_guard<std::mutex> autoLock(lock_);
        auto& printer =  ippPrinterMap_[uri];
        printer.device = usbDevice;
        printer.isSupportIpp = true;
        printer.indexVec = std::move(indexVec);
        fprintf(stderr, "DEBUG: USB_MONITOR printer support ipp-over-usb\n");
        return true;
    }
    fprintf(stderr, "DEBUG: USB_MONITOR printer is not support ipp-over-usb\n");
    return false;
}

bool IppUsbManager::ConnectUsbPinter(const std::string &uri)
{
    if (uri.empty()) {
        fprintf(stderr, "DEBUG: USB_MONITOR uri is empty\n");
        return false;
    }
    UsbDevice usbdevice;
    {
        std::lock_guard<std::mutex> autoLock(lock_);
        auto it = ippPrinterMap_.find(uri);
        if (it == ippPrinterMap_.end()) {
            fprintf(stderr, "DEBUG: USB_MONITOR not find uri in ippPrinterMap_\n");
            return false;
        }
        if (it->second.isOpened) {
            fprintf(stderr, "DEBUG: USB_MONITOR printer is opened\n");
            return true;
        } else {
            usbdevice = it->second.device;
        }
    }
    USBDevicePipe usbDevicePipe;
    int32_t openDeviceRet = usbSrvClient.OpenDevice(usbdevice, usbDevicePipe);
    if (openDeviceRet == UEC_OK) {
        return AllocateInterface(uri, usbdevice, usbDevicePipe);
    }
    fprintf(stderr, "DEBUG: USB_MONITOR OpenDevice fail, ret = %d\n", openDeviceRet);
    return false;
}

bool IppUsbManager::DisConnectUsbPinter(const std::string& uri)
{
    if (uri.empty()) {
        fprintf(stderr, "DEBUG: USB_MONITOR uri is empty\n");
        return false;
    }
    std::lock_guard<std::mutex> autoLock(lock_);
    auto it = ippPrinterMap_.find(uri);
    if (it == ippPrinterMap_.end()) {
        fprintf(stderr, "DEBUG: USB_MONITOR uri is not in ippPrinterMap_\n");
        return false;
    }
    usbSrvClient.Close(it->second.ippPipe);
    ippPrinterMap_.erase(uri);
    return true;
}

void IppUsbManager::SetPrinterStateReasons(PrinterStatus& printerStatus)
{
    std::vector<int> errorReasonIndex;
    for (size_t i = 0; i < ippPrinterErrorReason.size(); i++) {
        if (strstr(printerStatus.printerStateReasons, ippPrinterErrorReason[i]) != nullptr) {
            errorReasonIndex.push_back(i);
        }
    }

    std::ostringstream errorReasonStream;
    bool first = true;
    for (const auto index : errorReasonIndex) {
        if (!first) {
            errorReasonStream << ",";
        }
        errorReasonStream << ippPrinterErrorReason[index] << "-error";
        first = false;
    }

    const std::string errorReason = errorReasonStream.str();
    if (!errorReason.empty()) {
        int ret = snprintf_s(
            printerStatus.printerStateReasons,
            sizeof(printerStatus.printerStateReasons),
            sizeof(printerStatus.printerStateReasons) - 1,
            "%s",
            errorReason.c_str());
        if (ret < 0) {
            fprintf(stderr, "DEBUG: USB_MONITOR snprintf_s printerStateReasons error\n");
        }
    }
}

void IppUsbManager::ReportPrinterState(bool& isPrinterStarted, PrinterStatus& printerStatus,
    MonitorPrinterCallback callback)
{
    if (callback == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR callback is nullptr\n");
        return;
    }
    SetPrinterStateReasons(printerStatus);
    ipp_pstate_t printerState = printerStatus.printerState;
    if (!isPrinterStarted && printerState == IPP_PSTATE_IDLE &&
        strcmp(printerStatus.printerStateReasons, "none") != 0) {
        callback(&printerStatus); // report faults before the printer is started
        return;
    }
    if (!isPrinterStarted && printerState != IPP_PSTATE_IDLE) {
        isPrinterStarted = true;
    }
    constexpr int32_t bufferSize = 1024;
    if (isPrinterStarted && printerState == IPP_PSTATE_IDLE &&
        strcmp(printerStatus.printerStateReasons, "none") != 0 &&
        memset_s(printerStatus.printerStateReasons, bufferSize, 0, bufferSize) == 0 &&
        sprintf_s(printerStatus.printerStateReasons, bufferSize, "none") < 0) {
            fprintf(stderr, "DEBUG: USB_MONITOR memset_s printerStateReasons fail\n");
    }
    if (isPrinterStarted) {
        callback(&printerStatus); // report processing or stopped state
    }
}

bool IppUsbManager::ProcessMonitorPrinter(const std::string& uri, MonitorPrinterCallback callback)
{
    int32_t ret = 0;
    constexpr uint32_t MAX_LOOP_TIME = 60 * 60 * 24 * 30; // 30 days
    bool isPrinterStarted = false;
    for (uint32_t loopCount = 0; loopCount < MAX_LOOP_TIME && !isTerminated_.load(); loopCount++) {
        std::this_thread::sleep_for(std::chrono::seconds(INDEX_1));
        int32_t writeDataRetryCount = 0;
        do {
            auto ippdata = BuildIppRequest();
            ret = BulkTransferWrite(uri, ippdata);
            if (ret == EORROR_HDF_DEV_ERR_TIME_OUT) {
                std::this_thread::sleep_for(std::chrono::milliseconds(USB_WRITE_INTERVAL));
                writeDataRetryCount++;
                fprintf(stderr, "DEBUG: USB_MONITOR retrwriteDataRetryCounty = %d fail\n", writeDataRetryCount);
            }
        } while (ret == EORROR_HDF_DEV_ERR_TIME_OUT && writeDataRetryCount < WRITE_RETRY_MAX_TIMES);
        if (ret != UEC_OK) {
            fprintf(stderr, "DEBUG: USB_MONITOR BulkTransferWrite fail, ret = %d\n", ret);
            break;
        }
        PrinterStatus printerStatus;
        if (!ProcessDataFromDevice(uri, printerStatus)) {
            fprintf(stderr, "DEBUG: USB_MONITOR ProcessDataFromDevice false\n");
            break;
        }
        ReportPrinterState(isPrinterStarted, printerStatus, callback);
        if (isPrinterStarted && printerStatus.printerState == IPP_PSTATE_IDLE) {
            fprintf(stderr, "DEBUG: USB_MONITOR ProcessMonitorPrinter job is completed\n");
            return true;
        }
    }
    fprintf(stderr, "DEBUG: USB_MONITOR endtWriteDataToPrinterLooper\n");
    return false;
}

void IppUsbManager::SetTerminalSingal()
{
    isTerminated_.store(true);
}

void IppUsbManager::RemoveHttpHeader(std::vector<uint8_t>& readTempBuffer)
{
    auto it = std::search(
        readTempBuffer.begin(), readTempBuffer.end(),
        reinterpret_cast<const uint8_t*>("\r\n\r\n"),
        reinterpret_cast<const uint8_t*>("\r\n\r\n") + 4
    );
    if (it != readTempBuffer.end()) {
        readTempBuffer.erase(readTempBuffer.begin(), it + INDEX_4);
    }
}

bool IppUsbManager::ProcessDataFromDevice(const std::string& uri, PrinterStatus& printerStatus)
{
    constexpr int32_t MAX_TIME = 50;
    for (int32_t readCount = 0; readCount < MAX_TIME; readCount++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(USB_WRITE_INTERVAL));
        std::vector<uint8_t> readTempBuffer;
        int32_t readFromUsbRes = BulkTransferRead(uri, readTempBuffer);
        if (readFromUsbRes != UEC_OK && readFromUsbRes != EORROR_HDF_DEV_ERR_TIME_OUT) {
            fprintf(stderr, "DEBUG: USB_MONITOR BulkTransferRead fail, ret = %d\n", readFromUsbRes);
            break;
        }
        RemoveHttpHeader(readTempBuffer);
        if (readTempBuffer.empty()) {
            continue;
        }
        if (ParseIppResponse(readTempBuffer, printerStatus)) {
            fprintf(stderr, "DEBUG: USB_MONITOR ProcessDataFromDevice success\n");
            return true;
        }
    }
    fprintf(stderr, "DEBUG: USB_MONITOR ProcessDataFromDevice fail\n");
    return false;
}

int32_t IppUsbManager::BulkTransferRead(const std::string& uri, std::vector<uint8_t>& readTempBuffer)
{
    std::lock_guard<std::mutex> autoLock(lock_);
    auto it = ippPrinterMap_.find(uri);
    if (it == ippPrinterMap_.end()) {
        fprintf(stderr, "DEBUG: USB_MONITOR not found uri in ippPrinterMap_\n");
        return INVAILD_VALUE;
    }
    UsbDevice usbdevice = it->second.device;
    PrinterTranIndex tranIndex = it->second.tranIndex;
    int32_t currentConfigIndex = tranIndex.configIndex;
    int32_t currentInterfaceIndex = tranIndex.interfaceIndex;
    UsbInterface useInterface = usbdevice.GetConfigs()[currentConfigIndex].GetInterfaces()[currentInterfaceIndex];
    USBEndpoint pointRead;
    std::vector<USBEndpoint>& endPoints = useInterface.GetEndpoints();
    for (auto& point : endPoints) {
        if (point.GetDirection() != 0) {
            pointRead = point;
            break;
        }
    }
    USBDevicePipe usbDevicePipe = it->second.ippPipe;
    int32_t readFromUsbRes = 0;
    int32_t claimRetryCount = 0;
    do {
        readFromUsbRes = usbSrvClient.BulkTransfer(usbDevicePipe, pointRead, readTempBuffer,
            USB_BULKTRANSFER_READ_TIMEOUT);
        if (readFromUsbRes == EORROR_HDF_DEV_ERR_IO_FAILURE) {
            int claimRes = usbSrvClient.ClaimInterface(usbDevicePipe, useInterface, true);
            if (claimRes < 0 || ++claimRetryCount >= CLAIM_INTERFACE_RETRY_MAX_TIMES) {
                break;
            }
        }
    } while (readFromUsbRes == EORROR_HDF_DEV_ERR_IO_FAILURE);
    return readFromUsbRes;
}

int32_t IppUsbManager::BulkTransferWrite(const std::string& uri, std::vector<uint8_t>& vectorRequestBuffer)
{
    std::lock_guard<std::mutex> autoLock(lock_);
    auto it = ippPrinterMap_.find(uri);
    if (it == ippPrinterMap_.end()) {
        fprintf(stderr, "DEBUG: USB_MONITOR not found uri in ippPrinterMap_\n");
        return INVAILD_VALUE;
    }
    UsbDevice& usbdevice = it->second.device;
    PrinterTranIndex& tranIndex = it->second.tranIndex;
    int32_t currentConfigIndex = tranIndex.configIndex;
    int32_t currentInterfaceIndex = tranIndex.interfaceIndex;
    UsbInterface useInterface = usbdevice.GetConfigs()[currentConfigIndex].GetInterfaces()[currentInterfaceIndex];
    USBEndpoint pointWrite;
    std::vector<USBEndpoint>& endPoints = useInterface.GetEndpoints();
    for (auto& point : endPoints) {
        if (point.GetDirection() == 0) {
            pointWrite = point;
            break;
        }
    }
    USBDevicePipe usbDevicePipe = it->second.ippPipe;
    int32_t writeRet = 0;
    int32_t claimRetryCount = 0;
    do {
        writeRet = usbSrvClient.BulkTransfer(usbDevicePipe, pointWrite, vectorRequestBuffer,
            USB_BULKTRANSFER_WRITE_TIMEOUT);
        if (writeRet == EORROR_HDF_DEV_ERR_IO_FAILURE) {
            int claimRes = usbSrvClient.ClaimInterface(usbDevicePipe, useInterface, true);
            if (claimRes < 0 || ++claimRetryCount >= CLAIM_INTERFACE_RETRY_MAX_TIMES) {
                break;
            }
        }
    } while (writeRet == EORROR_HDF_DEV_ERR_IO_FAILURE);
    return writeRet;
}

bool IppUsbManager::AllocateInterface(const std::string &uri, UsbDevice& usbdevice,
    USBDevicePipe &usbDevicePipe)
{
    PrinterTranIndex tranIndex;
    std::lock_guard<std::mutex> autoLock(lock_);
    auto it = ippPrinterMap_.find(uri);
    if (it == ippPrinterMap_.end()) {
        fprintf(stderr, "DEBUG: USB_MONITOR AllocateInterface, cannot find uri in ippPrinterMap_\n");
        usbSrvClient.Close(usbDevicePipe);
        return false;
    }
    std::vector<PrinterTranIndex>& indexVec = it->second.indexVec;
    for (auto indexVecIt = indexVec.rbegin(); indexVecIt != indexVec.rend(); it++) {
        int32_t configIndex = indexVecIt->configIndex;
        int32_t interfaceIndex = indexVecIt->interfaceIndex;
        UsbInterface ippInterface =
            usbdevice.GetConfigs()[configIndex].GetInterfaces()[interfaceIndex];
        int32_t ret = usbSrvClient.ClaimInterface(usbDevicePipe, ippInterface, true);
        if (ret != UEC_OK) {
            fprintf(stderr, "DEBUG: USB_MONITOR ClaimInterface fail, ret = %d\n", ret);
            continue;
        }
        if (ippInterface.GetAlternateSetting() != 0) {
            ret = usbSrvClient.SetInterface(usbDevicePipe, ippInterface);
            if (ret != UEC_OK) {
                fprintf(stderr, "DEBUG: USB_MONITOR SetInterface fail, ret = %d\n", ret);
                continue;
            }
        }
        tranIndex.configIndex = configIndex;
        tranIndex.interfaceIndex = interfaceIndex;
        break;
    }
    if (tranIndex.configIndex == INVAILD_VALUE) {
        fprintf(stderr, "DEBUG: USB_MONITOR connect usb fail");
        usbSrvClient.Close(usbDevicePipe);
        return false;
    }
    auto& printer = it->second;
    printer.isOpened = true;
    printer.ippPipe = usbDevicePipe;
    printer.tranIndex = tranIndex;
    return true;
}

std::vector<uint8_t> IppUsbManager::BuildIppRequest()
{
    ipp_t* request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
    if (request == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR request is a nullptr.\n");
        return {};
    }
    static const char * const jattrs[] = {
        "printer-state",
        "printer-state-reasons"
    };
    static const std::string DEFAULT_USER = "default";
    static const std::string LOCAL_URI = "ipp://127.0.0.1:60000/ipp/print";
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", nullptr, LOCAL_URI.c_str());
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", nullptr, DEFAULT_USER.c_str());
    ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes",
        sizeof(jattrs) / sizeof(jattrs[0]), nullptr, jattrs);
    
    std::vector<uint8_t> ippdata;
    ippWriteIO(&ippdata, (ipp_iocb_t)IppUsbManager::WriteToBuffer, true, nullptr, request);
    ippDelete(request);
    if (ippdata.empty()) {
        fprintf(stderr, "DEBUG: USB_MONITOR ippWriteIO fail.\n");
        return {};
    }
    std::string httpHeader = "POST /ipp/print HTTP/1.1\r\n"
                            "Host: localhost:60000\r\n"
                            "Content-Type: application/ipp\r\n"
                            "Content-Length: " + std::to_string(ippdata.size()) + "\r\n\r\n";

    std::vector<uint8_t> fullData(ippdata.size() + httpHeader.size());
    if (memcpy_s(fullData.data(), fullData.size(), httpHeader.data(), httpHeader.size()) != 0) {
        fprintf(stderr, "DEBUG: USB_MONITOR memcpy_s httpHeader fail.\n");
        return {};
    }
    if (memcpy_s(fullData.data() + httpHeader.size(), fullData.size(), ippdata.data(), ippdata.size()) != 0) {
        fprintf(stderr, "DEBUG: USB_MONITOR memcpy_s ippdata fail.\n");
        return {};
    }
    return fullData;
}

ssize_t IppUsbManager::WriteToBuffer(void* ctx, const void* data, size_t len)
{
    auto *buffer = static_cast<std::vector<uint8_t> *>(ctx);
    if (buffer == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR buffer is a nullptr.\n");
        return IPP_STATE_ERROR;
    }
    size_t oldSize = buffer->size();
    buffer->resize(oldSize + len);
    if (memcpy_s(buffer->data() + oldSize, buffer->size(), data, len) != 0) {
        fprintf(stderr, "DEBUG: USB_MONITOR WriteToBuffer memcpy_s fail.\n");
        return IPP_STATE_ERROR;
    }
    return static_cast<ssize_t>(len);
}

ssize_t IppUsbManager::ReadFromBuffer(void *ctx, void *data, size_t len)
{
    if (ctx == nullptr || data == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR ctx or data is a nullptr.\n");
        return IPP_STATE_ERROR;
    }
    auto* context = static_cast<BufferContext*>(ctx);
    size_t remainSize = context->size - context->pos;
    if (remainSize == INDEX_0) {
        fprintf(stderr, "DEBUG: USB_MONITOR Read completed.\n");
        return INDEX_0;
    }
    size_t toRead = std::min(len, remainSize);
    if (memcpy_s(data, len, context->data + context->pos, toRead) != 0) {
        fprintf(stderr, "DEBUG: USB_MONITOR ReadFromBuffer memcpy_s fail.\n");
        return IPP_STATE_ERROR;
    }
    context->pos += toRead;
    return static_cast<ssize_t>(toRead);
}

bool IppUsbManager::ParseIppResponse(std::vector<uint8_t>& responseData, PrinterStatus& printerStatus)
{
    ipp_t *response = ippNew();
    if (response == nullptr) {
        fprintf(stderr, "DEBUG: USB_MONITOR response is nullptr.\n");
        return false;
    }
    BufferContext context { responseData.data(), responseData.size(), 0};
    ipp_state_t state = ippReadIO(&context, (ipp_iocb_t)IppUsbManager::ReadFromBuffer, true, nullptr, response);
    if (state != IPP_STATE_DATA) {
        fprintf(stderr, "DEBUG: USB_MONITOR Failed to parse IPP response.\n");
        ippDelete(response);
        return false;
    }
    ipp_attribute_t *attr = nullptr;
    constexpr int32_t bufferSize = 1024;
    printerStatus.printerState = IPP_PSTATE_IDLE;
    if (memset_s(printerStatus.printerStateReasons, bufferSize, 0, bufferSize) != 0) {
        fprintf(stderr, "DEBUG: USB_MONITOR memset_s printerStateReasons fail\n");
        ippDelete(response);
        return false;
    }
    if ((attr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) != nullptr) {
        printerStatus.printerState = (ipp_pstate_e)ippGetInteger(attr, 0);
    }
    if ((attr = ippFindAttribute(response, "printer-state-reasons", IPP_TAG_KEYWORD)) != nullptr) {
        ippAttributeString(attr, printerStatus.printerStateReasons, sizeof(printerStatus.printerStateReasons));
    }
    fprintf(stderr, "DEBUG: USB_MONITOR printerStateReasons = %s, printerState = %d\n",
        printerStatus.printerStateReasons, static_cast<int32_t>(printerStatus.printerState));
    ippDelete(response);
    return true;
}
}  // namespace OHOS::CUPS
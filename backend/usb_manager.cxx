/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/*
 * OH USB interface code for CUPS.
 */

/*
 * Include necessary headers.
 */

#include "usb_manager.h"
#include "usb_srv_client.h"
#include "v1_0/iusb_interface.h"

#include <map>
#include <regex>
#include <string>
#include <thread>
#include <securec.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::USB;

#define SAFE_DELETE(ptr)    \
    if ((ptr) != nullptr) { \
        delete (ptr);       \
        (ptr) = nullptr;    \
    }

#define SAFE_DELETE_ARRAY(ptr) \
    if ((ptr) != nullptr) {    \
        delete[] (ptr);        \
        (ptr) = nullptr;       \
    }

int32_t BulkTransferWriteData(
    USBDevicePipe &usbDevicePipe, USBEndpoint &endpoint, const std::string &sendDataStr, int32_t timeout);
int32_t ParseDeviceDescriptor(UsbDevice &usbDevice, ohusb_device_descriptor *devDesc);
int32_t ParseConfigDescriptor(USBConfig &usbConfig, ohusb_config_descriptor *confDesc);
int32_t ParseInterface(std::vector<UsbInterface> &usbIfaces, ohusb_interface *usbInterface, int32_t ifaceIndex);
int32_t ParseInterfaceDescriptor(UsbInterface &usbInterface, ohusb_interface_descriptor *ifaceDesc);
int32_t ParseEndpointDescriptor(USBEndpoint &usbEndpoint, ohusb_endpoint_descriptor *epDesc);
void ReleaseDeviceDescriptor(ohusb_device_descriptor *devDesc);
void ReleaseConfigDescriptor(ohusb_config_descriptor *confDesc);
void ReleaseInterface(ohusb_interface *iface);
void ReleaseInterfaceDescriptor(ohusb_interface_descriptor *ifaceDesc);
void DumpDeviceDescriptor(ohusb_device_descriptor *devdesc);
void DumpConfigDescriptor(ohusb_config_descriptor *confDesc);
void DumpInterfaceDescriptor(ohusb_interface_descriptor *ifaceDesc);
void DumpEndpointDescriptor(ohusb_endpoint_descriptor *eptDesc);
char *CopyString(const std::string &source);

int32_t OH_GetDevices(ohusb_device_descriptor** list, ssize_t *numdevs)
{
    fprintf(stderr, "DEBUG: OH_GetDevices enter\n");
    vector<UsbDevice> devlist;
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    auto getDevRet = usbSrvClient.GetDevices(devlist);
    fprintf(stderr, "DEBUG: OH_GetDevices getDevRet = %d\n", getDevRet);
    if (getDevRet != OHOS::ERR_OK) {
        return OHUSB_ERROR_IO;
    }
    if (devlist.empty()) {
        return OHUSB_SUCCESS;
    }
    ssize_t devCount = devlist.size();
    *numdevs = 0;
    fprintf(stderr, "DEBUG: OH_GetDevices device nums = %zd\n", devCount);

    *list = (ohusb_device_descriptor *)calloc((size_t)devCount, sizeof(ohusb_device_descriptor));
    if (*list == nullptr) {
        fprintf(stderr, "DEBUG: OH_GetDevices list is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    for (ssize_t i = 0; i < devCount; i++) {
        fprintf(stderr, "DEBUG: OH_GetDevices devlist[%zd] devAddr = %d, busNum = %d\n",
            i, devlist[i].GetDevAddr(), devlist[i].GetBusNum());
        if (ParseDeviceDescriptor(devlist[i], *list + i) != OHUSB_SUCCESS) {
            return OHUSB_ERROR_IO;
        }
        (*numdevs)++;
    }
    return OHUSB_SUCCESS;
}

int32_t OH_OpenDevice(ohusb_device_descriptor *devDesc, ohusb_pipe **pipe)
{
    fprintf(stderr, "DEBUG: OH_OpenDevice enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    UsbDevice usbDevice;
    usbDevice.SetBusNum(devDesc->busNum);
    usbDevice.SetDevAddr(devDesc->devAddr);
    USBDevicePipe usbDevicePipe;
    auto openDevRet = usbSrvClient.OpenDevice(usbDevice, usbDevicePipe);
    fprintf(stderr, "DEBUG: OH_OpenDevice getDevRet = %d\n", openDevRet);
    if (openDevRet != OHOS::ERR_OK) {
        return OHUSB_ERROR_IO;
    }
    *pipe = (ohusb_pipe *)calloc(1, sizeof(ohusb_pipe));
    if (*pipe == nullptr) {
        fprintf(stderr, "DEBUG: OH_OpenDevice pipe is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    (*pipe)->busNum = usbDevicePipe.GetBusNum();
    (*pipe)->devAddr = usbDevicePipe.GetDevAddr();
    fprintf(stderr, "DEBUG: OH_OpenDevice busNum=%d, devAddr=%d\n", (*pipe)->busNum, (*pipe)->devAddr);
    fprintf(stderr, "DEBUG: OH_OpenDevice out\n");
    return OHUSB_SUCCESS;
}

int32_t OH_CloseDevice(ohusb_pipe *pipe)
{
    fprintf(stderr, "DEBUG: OH_CloseDevice enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    int32_t ret = usbSrvClient.Close(usbDevicePipe);
    if (ret != ERR_OK) {
        fprintf(stderr, "DEBUG: OH_CloseDevice fail with ret = %d\n", ret);
        return OHUSB_ERROR_BUSY;
    }
    fprintf(stderr, "DEBUG: OH_CloseDevice out\n");
    return OHUSB_SUCCESS;
}

int32_t OH_ClaimInterface(ohusb_pipe *pipe, int interfaceId, bool force)
{
    fprintf(stderr, "DEBUG: OH_ClaimInterface enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    UsbInterface usbInterface;
    usbInterface.SetId(interfaceId);
    int32_t ret = usbSrvClient.ClaimInterface(usbDevicePipe, usbInterface, force);
    if (ret != ERR_OK) {
        fprintf(stderr, "DEBUG: OH_ClaimInterface fail with ret = %d\n", ret);
        return OHUSB_ERROR_BUSY;
    }
    fprintf(stderr, "DEBUG: OH_ClaimInterface out\n");
    return OHUSB_SUCCESS;
}

int32_t OH_ReleaseInterface(ohusb_pipe *pipe, int interfaceId)
{
    fprintf(stderr, "DEBUG: OH_ReleaseInterface enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    UsbInterface usbInterface;
    usbInterface.SetId(interfaceId);
    int32_t ret = usbSrvClient.ReleaseInterface(usbDevicePipe, usbInterface);
    if (ret != ERR_OK) {
        fprintf(stderr, "DEBUG: OH_ReleaseInterface fail with ret = %d\n", ret);
        return OHUSB_ERROR_BUSY;
    }
    fprintf(stderr, "DEBUG: OH_ReleaseInterface out\n");
    return OHUSB_SUCCESS;
}

int32_t OH_BulkTransferRead(
    ohusb_pipe *pipe, ohusb_transfer_pipe *tpipe, unsigned char *data, int length, int *transferred)
{
    fprintf(stderr, "DEBUG: OH_BulkTransferRead enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    USBEndpoint endpoint;
    endpoint.SetInterfaceId(tpipe->bInterfaceId);
    endpoint.SetAddr(tpipe->bEndpointAddress);
    std::vector<uint8_t> readTempBuffer;
    size_t readSize = 0;
    int32_t ret = usbSrvClient.BulkTransfer(usbDevicePipe, endpoint, readTempBuffer, OHUSB_BULKTRANSFER_READ_TIMEOUT);
    fprintf(stderr, "DEBUG: OH_BulkTransferRead ret = %d\n", ret);
    if (ret == HDF_DEV_ERR_NO_DEVICE) {
        fprintf(stderr, "DEBUG: OH_BulkTransferRead HDF_DEV_ERR_NO_DEVICE, The device module has no device\n");
        return OHUSB_ERROR_NO_DEVICE;
    }
    readSize = readTempBuffer.size();
    fprintf(stderr, "DEBUG: OH_BulkTransferRead readSize = %zu\n", readSize);
    if (ret == OHUSB_SUCCESS) {
        if (readSize < 512) {
            std::copy(readTempBuffer.begin(), readTempBuffer.begin() + readSize, data);
            *transferred = readSize;
        }
        return OHUSB_SUCCESS;
    }

    fprintf(stderr, "DEBUG: OH_BulkTransferRead read data time out\n");
    return OHUSB_ERROR_TIMEOUT;
}

int32_t OH_BulkTransferWrite(
    ohusb_pipe *pipe, ohusb_transfer_pipe *tpipe, unsigned char *data, int length, int *transferred, int32_t timeout)
{
    fprintf(stderr, "DEBUG: OH_BulkTransferRead enter\n");
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    USBEndpoint endpoint;
    endpoint.SetInterfaceId(tpipe->bInterfaceId);
    endpoint.SetAddr(tpipe->bEndpointAddress);
    int32_t ret = 0;
    int leftDataLen = length;
    while (leftDataLen > 0) {
        fprintf(stderr, "DEBUG: OH_BulkTransferWrite leftDataLen waiting for transfer = %d\n", leftDataLen);
        size_t len = leftDataLen > OHUSB_ENDPOINT_MAX_LENGTH ? OHUSB_ENDPOINT_MAX_LENGTH : leftDataLen;
        std::string sendDataStr(reinterpret_cast<char *>(data), len);
        ret = BulkTransferWriteData(usbDevicePipe, endpoint, sendDataStr, timeout);
        if (ret == OHUSB_ERROR_IO) {
            fprintf(stderr, "DEBUG: OH_BulkTransferWrite OHUSB_ERROR_IO\n");
            int32_t claimRet = OH_ClaimInterface(pipe, tpipe->bClaimedInterfaceId, true);
            if (claimRet >= 0) {
                fprintf(stderr, "DEBUG: OH_BulkTransferWrite OH_ClaimInterface success ret = %d\n", claimRet);
                continue;
            }
        }
        if (ret != 0) {
            return ret;
        }
        fprintf(stderr, "DEBUG: OH_BulkTransferWrite transferred data len = %zu\n", len);
        leftDataLen -= len;
        data += len;
    }
    *transferred = length - leftDataLen;
    fprintf(stderr, "DEBUG: transferred data len = %d\n", *transferred);
    fprintf(stderr, "DEBUG: OH_BulkTransferWrite out\n");
    return OHUSB_SUCCESS;
}

int32_t BulkTransferWriteData(
    USBDevicePipe &usbDevicePipe, USBEndpoint &endpoint, const std::string &sendDataStr, int32_t timeout)
{
    std::vector<uint8_t> vectorRequestBuffer;
    vectorRequestBuffer.assign(sendDataStr.begin(), sendDataStr.end());
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    uint32_t retryNum = 0;
    int32_t ret = 0;
    do {
        fprintf(stderr, "DEBUG: BulkTransferWriteData write data retryCout: %d\n", retryNum);
        ret = usbSrvClient.BulkTransfer(usbDevicePipe, endpoint, vectorRequestBuffer, timeout);
        fprintf(stderr, "DEBUG: BulkTransferWrite ret: %d\n", ret);
        if (ret == HDF_DEV_ERR_NO_DEVICE) {
            fprintf(stderr, "DEBUG: BulkTransferWriteData no device\n");
            return OHUSB_ERROR_NO_DEVICE;
        }
        if (ret == OHUSB_ERROR_IO) {
            fprintf(stderr, "DEBUG: BulkTransferWriteData OHUSB_ERROR_IO\n");
            return OHUSB_ERROR_IO;
        }
        if (ret == OHUSB_ERROR_TIMEOUT) {
            std::this_thread::sleep_for(std::chrono::milliseconds(OHUSB_BULKTRANSFER_WRITE_SLEEP));
        }
        retryNum++;
    } while (ret != OHUSB_SUCCESS && retryNum < OHUSB_WRITE_RETRY_MAX_TIMES);
    if (ret != 0) {
        fprintf(stderr, "DEBUG: Write data fail\n");
        return ret;
    }
    return ret;
}

int32_t OH_ControlTransferRead(
    ohusb_pipe *pipe, ohusb_control_transfer_parameter *ctrlParam, unsigned char *data, uint16_t length)
{
    fprintf(stderr, "DEBUG: OH_ControlTransferRead enter\n");
    if (pipe == nullptr || ctrlParam == nullptr) {
        fprintf(stderr, "DEBUG: pipe or ctrlParam is nullptr\n");
        return OHUSB_ERROR_IO;
    }
    fprintf(stderr, "DEBUG: OH_ControlTransferRead busNum=%d, devAddr=%d\n", pipe->busNum, pipe->devAddr);
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    int32_t requestType = ctrlParam->requestType;
    int32_t request = ctrlParam->request;
    int32_t value = ctrlParam->value;
    int32_t index = ctrlParam->index;
    int32_t timeOut = ctrlParam->timeout;
    const HDI::Usb::V1_0::UsbCtrlTransfer tctrl = {requestType, request, value, index, timeOut};
    std::vector<uint8_t> bufferData(length, 0);
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    uint32_t retryNum = 0;
    int32_t ret = OHUSB_SUCCESS;
    do {
        ret = usbSrvClient.ControlTransfer(usbDevicePipe, tctrl, bufferData);
        retryNum++;
    } while ((ret != OHUSB_SUCCESS || bufferData.size() == 0) && retryNum < OHUSB_CONTROLTRANSFER_READ_RETRY_MAX_TIMES);
    fprintf(stderr, "DEBUG: OH_ControlTransferRead ret = %d, retryCount: %d\n", ret, retryNum);
    if (ret != 0) {
        fprintf(stderr, "DEBUG: OH_ControlTransferRead fail\n");
        return OHUSB_ERROR_IO;
    }
    fprintf(stderr, "DEBUG: OH_ControlTransferRead bufferData size = %zu\n", bufferData.size());
    
    if (bufferData.size() == 0) {
        return 0;
    }

    if ((value >> 8) == OHUSB_DT_STRING) {
        int bufferLen = bufferData.size();
        uint16_t *wbuf = new (std::nothrow) uint16_t[bufferLen + 1]();
        if (wbuf == nullptr) {
            fprintf(stderr, "DEBUG: OH_ControlTransferRead wbuf is nullptr.\n");
            return OHUSB_ERROR_NO_MEM;
        }
        for (uint32_t i = 0; i < bufferLen - 2; ++i) {
            wbuf[i] = bufferData[i + 2];
        }
        std::wstring wstr(reinterpret_cast<wchar_t *>(wbuf), (bufferLen - 2) / 2);
        std::string buffer(wstr.begin(), wstr.end());
        fprintf(stderr, "DEBUG: OH_ControlTransferRead buffer = %s\n", buffer.c_str());
        if (memcpy_s(data, length, buffer.c_str(), buffer.length()) != 0) {
            fprintf(stderr, "DEBUG: OH_ControlTransferRead memcpy_s fail.\n");
            return OHUSB_ERROR_NO_MEM;
        }
    } else if (memcpy_s(data, length, bufferData.data(), bufferData.size()) != 0) {
        fprintf(stderr, "DEBUG: OH_ControlTransferRead memcpy_s fail.\n");
        return OHUSB_ERROR_NO_MEM;
    }

    fprintf(stderr, "DEBUG: OH_ControlTransferRead out\n");
    return bufferData.size();
}

int32_t OH_ControlTransferWrite(
    ohusb_pipe *pipe, ohusb_control_transfer_parameter *ctrlParam, unsigned char *data, uint16_t length)
{
    fprintf(stderr, "DEBUG: OH_ControlTransferWrite enter\n");
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    int32_t requestType = ctrlParam->requestType;
    int32_t request = ctrlParam->request;
    int32_t value = ctrlParam->value;
    int32_t index = ctrlParam->index;
    int32_t timeOut = ctrlParam->timeout;
    const HDI::Usb::V1_0::UsbCtrlTransfer tctrl = {requestType, request, value, index, timeOut};
    std::vector<uint8_t> bufferData(length, 0);
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    int32_t ret = usbSrvClient.ControlTransfer(usbDevicePipe, tctrl, bufferData);
    if (ret != 0) {
        fprintf(stderr, "DEBUG: OH_ControlTransferWrite fail with ret = %d\n", ret);
        return OHUSB_ERROR_IO;
    }

    fprintf(stderr, "DEBUG: OH_ControlTransferWrite out\n");
    return length;
}

int32_t OH_GetStringDescriptor(ohusb_pipe *pipe, int descId, unsigned char *descriptor, int length)
{
    fprintf(stderr, "DEBUG: OH_GetStringDescriptor enter\n");
    if (descId == 0) {
		return OHUSB_ERROR_INVALID_PARAM;
    }
    ohusb_control_transfer_parameter ctrlParam = {
        OHUSB_ENDPOINT_IN,
        OHUSB_REQUEST_GET_DESCRIPTOR,
        (uint16_t)((OHUSB_DT_STRING << 8) | descId),
        OHUSB_LANGUAGE_ID_ENGLISH,
        OHUSB_GET_STRING_DESCRIPTOR_TIMEOUT
    };
    auto ret = OH_ControlTransferRead(pipe, &ctrlParam, descriptor, length);
    if (ret <= 0) {
        fprintf(stderr, "DEBUG: OH_GetStringDescriptor OH_ControlTransferRead failed\n");
        return ret;
    }

    fprintf(stderr, "DEBUG: OH_GetStringDescriptor out\n");
    return ret;
}

int32_t ParseDeviceDescriptor(UsbDevice &usbDevice, ohusb_device_descriptor *devDesc)
{
    fprintf(stderr, "DEBUG: ParseDeviceDescriptor enter\n");
    if (devDesc == nullptr) {
        fprintf(stderr, "DEBUG: ParseDeviceDescriptor devDesc is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }

    devDesc->devAddr = usbDevice.GetDevAddr();
    devDesc->busNum = usbDevice.GetBusNum();
    devDesc->bcdUSB = usbDevice.GetbcdUSB();
    devDesc->bDeviceClass = usbDevice.GetClass();
    devDesc->bDeviceSubClass = usbDevice.GetSubclass();
    devDesc->bDeviceProtocol = usbDevice.GetProtocol();
    devDesc->bMaxPacketSize0 = usbDevice.GetbMaxPacketSize0();
    devDesc->idVendor = usbDevice.GetVendorId();
    devDesc->idProduct = usbDevice.GetProductId();
    devDesc->iManufacturer = usbDevice.GetiManufacturer();
    devDesc->iProduct = usbDevice.GetiProduct();
    devDesc->iSerialNumber = usbDevice.GetiSerialNumber();
    devDesc->bNumConfigurations = 0;
    ohusb_config_descriptor *config =
        (ohusb_config_descriptor *)calloc((size_t)usbDevice.GetConfigCount(), sizeof(ohusb_config_descriptor));
    if (config == nullptr) {
        fprintf(stderr, "DEBUG: ParseDeviceDescriptor config is null.\n");
        return OHUSB_ERROR_NO_MEM;
    }
    devDesc->config = config;

    for (int i = 0; i < usbDevice.GetConfigCount(); i++) {
        USBConfig usbConfig;
        usbDevice.GetConfig(i, usbConfig);
        if (ParseConfigDescriptor(usbConfig, config + i) != OHUSB_SUCCESS) {
            return OHUSB_ERROR_IO;
        }
        (devDesc->bNumConfigurations)++;
    }
    DumpDeviceDescriptor(devDesc);
    fprintf(stderr, "DEBUG: ParseDeviceDescriptor out\n");
    return OHUSB_SUCCESS;
}

int32_t ParseConfigDescriptor(USBConfig &usbConfig, ohusb_config_descriptor *confDesc)
{
    fprintf(stderr, "DEBUG: ParseConfigDescriptor enter\n");
    if (confDesc == nullptr) {
        fprintf(stderr, "DEBUG: ParseConfigDescriptor confDesc is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    confDesc->iConfiguration = usbConfig.GetId();
    confDesc->bmAttributes = usbConfig.GetAttributes();
    confDesc->MaxPower = usbConfig.GetMaxPower();

    std::vector<UsbInterface> usbIfaces = usbConfig.GetInterfaces();
    int32_t ifaceCount = usbIfaces.size() > 0 ? usbIfaces[usbIfaces.size() - 1].GetId() + 1 : 0;

    confDesc->bNumInterfaces = 0;
    ohusb_interface *usbInterface =
        (ohusb_interface *)calloc((size_t)ifaceCount, sizeof(ohusb_interface));
    if (usbInterface == nullptr) {
        fprintf(stderr, "DEBUG: ParseConfigDescriptor usbInterface is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    confDesc->interface = usbInterface;
    for (int32_t ifaceIndex = 0; ifaceIndex < ifaceCount; ifaceIndex++) {
        if (ParseInterface(usbIfaces, usbInterface + ifaceIndex, ifaceIndex) != OHUSB_SUCCESS) {
            return OHUSB_ERROR_IO;
        }
        (confDesc->bNumInterfaces)++;
    }
    DumpConfigDescriptor(confDesc);
    fprintf(stderr, "DEBUG: ParseConfigDescriptor out\n");
    return OHUSB_SUCCESS;
}

int32_t ParseInterface(std::vector<UsbInterface> &usbIfaces, ohusb_interface *usbInterface, int32_t ifaceIndex)
{
    fprintf(stderr, "DEBUG: ParseInterface ifaceIndex=%d\n", ifaceIndex);
    for (int32_t usbIfacesIndex = 0; usbIfacesIndex < usbIfaces.size(); usbIfacesIndex++) {
        if (usbIfaces[usbIfacesIndex].GetId() == ifaceIndex) {
            ohusb_interface_descriptor *altsetting;
            altsetting = (ohusb_interface_descriptor *)
                calloc((size_t)(usbInterface->num_altsetting + 1), sizeof(*altsetting));
            if (altsetting == nullptr) {
                fprintf(stderr, "DEBUG: ParseInterface altsetting is nullptr\n");
                return OHUSB_ERROR_NO_MEM;
            }
            if (usbInterface->num_altsetting != 0 &&
                memcpy_s(altsetting, sizeof(*altsetting) * (size_t)(usbInterface->num_altsetting),
                usbInterface->altsetting, sizeof(*altsetting) * (size_t)(usbInterface->num_altsetting)) != 0) {
                fprintf(stderr, "DEBUG: ParseInterface memcpy_s fail.\n");
                return OHUSB_ERROR_NO_MEM;
            }
            usbInterface->altsetting = altsetting;
            if (ParseInterfaceDescriptor(usbIfaces[usbIfacesIndex], altsetting + (usbInterface->num_altsetting))
                != OHUSB_SUCCESS) {
                return OHUSB_ERROR_IO;
            }
            usbInterface->num_altsetting++;
        }
    }
    fprintf(stderr, "DEBUG: ParseInterface num_altsetting=%d\n", usbInterface->num_altsetting);
    return OHUSB_SUCCESS;
}

int32_t ParseInterfaceDescriptor(UsbInterface &usbInterface, ohusb_interface_descriptor *ifaceDesc)
{
    fprintf(stderr, "DEBUG: ParseInterfaceDescriptor enter\n");
    if (ifaceDesc == nullptr) {
        fprintf(stderr, "DEBUG: ParseInterfaceDescriptor ifaceDesc is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    ifaceDesc->bInterfaceNumber = usbInterface.GetId();
    ifaceDesc->bAlternateSetting = usbInterface.GetAlternateSetting();
    ifaceDesc->bInterfaceClass = usbInterface.GetClass();
    ifaceDesc->bInterfaceSubClass = usbInterface.GetSubClass();
    ifaceDesc->bInterfaceProtocol = usbInterface.GetProtocol();
    ifaceDesc->bNumEndpoints = 0;
    fprintf(stderr, "DEBUG: ParseInterfaceDescriptor class=%x, subclass=%x, protocol=%x.\n",
        usbInterface.GetClass(), usbInterface.GetSubClass(), usbInterface.GetProtocol());
	ohusb_endpoint_descriptor *endpoint;
    endpoint = (ohusb_endpoint_descriptor *)
        calloc((size_t)usbInterface.GetEndpointCount(), sizeof(ohusb_endpoint_descriptor));
    if (endpoint == nullptr) {
        fprintf(stderr, "DEBUG: endpoint is null.\n");
        return OHUSB_ERROR_NO_MEM;
    }
    ifaceDesc->endpoint = endpoint;
    std::vector<USBEndpoint> endpoints = usbInterface.GetEndpoints();
    for (int i = 0; i < usbInterface.GetEndpointCount(); i++) {
        if (ParseEndpointDescriptor(endpoints[i], endpoint + i) != OHUSB_SUCCESS) {
            return OHUSB_ERROR_IO;
        }
        (ifaceDesc->bNumEndpoints)++;
    }
    DumpInterfaceDescriptor(ifaceDesc);
    fprintf(stderr, "DEBUG: ParseInterfaceDescriptor out\n");
	return OHUSB_SUCCESS;
}

int32_t ParseEndpointDescriptor(USBEndpoint &usbEndpoint, ohusb_endpoint_descriptor *epDesc)
{
    fprintf(stderr, "DEBUG: ParseEndpointDescriptor enter\n");
    if (epDesc == nullptr) {
        fprintf(stderr, "DEBUG: ParseEndpointDescriptor epDesc is nullptr\n");
        return OHUSB_ERROR_NO_MEM;
    }
    epDesc->bDescriptorType = usbEndpoint.GetType();
    epDesc->bEndpointAddress = usbEndpoint.GetAddress();
    epDesc->bmAttributes = usbEndpoint.GetAttributes();
    epDesc->wMaxPacketSize = usbEndpoint.GetMaxPacketSize();
    epDesc->bInterval = usbEndpoint.GetInterval();
    epDesc->bInterfaceId = usbEndpoint.GetInterfaceId();
    epDesc->direction = usbEndpoint.GetDirection();
    DumpEndpointDescriptor(epDesc);
    fprintf(stderr, "DEBUG: ParseEndpointDescriptor out\n");
	return OHUSB_SUCCESS;
}

int32_t OH_SetConfiguration(ohusb_pipe *pipe, int configIndex)
{
    fprintf(stderr, "DEBUG: OH_SetConfiguration enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    USBConfig usbConfig;
    usbConfig.SetId(configIndex);
    int32_t ret = usbSrvClient.SetConfiguration(usbDevicePipe, usbConfig);
    if (ret != ERR_OK) {
        fprintf(stderr, "DEBUG: OH_SetConfiguration fail with ret = %d\n", ret);
        return OHUSB_ERROR_BUSY;
    }
    fprintf(stderr, "DEBUG: OH_SetConfiguration out\n");
    return OHUSB_SUCCESS;
}

int32_t OH_SetInterface(ohusb_pipe *pipe, int interfaceId, int altIndex)
{
    fprintf(stderr, "DEBUG: OH_SetInterface enter\n");
    auto &usbSrvClient = OHOS::USB::UsbSrvClient::GetInstance();
    USBDevicePipe usbDevicePipe = {pipe->busNum, pipe->devAddr};
    UsbInterface usbInterface;
    usbInterface.SetId(interfaceId);
    usbInterface.SetAlternateSetting(altIndex);
    int32_t ret = usbSrvClient.SetInterface(usbDevicePipe, usbInterface);
    if (ret != ERR_OK) {
        fprintf(stderr, "DEBUG: OH_SetInterface fail with ret = %d\n", ret);
        return OHUSB_ERROR_BUSY;
    }
    fprintf(stderr, "DEBUG: OH_SetInterface out\n");
    return OHUSB_SUCCESS;
}

void ReleaseInterfaceDescriptor(ohusb_interface_descriptor *ifaceDesc)
{
    if (ifaceDesc != nullptr) {
        SAFE_DELETE_ARRAY(ifaceDesc->endpoint);
        ifaceDesc->bNumEndpoints = 0;
        SAFE_DELETE_ARRAY(ifaceDesc);
    }
}

void ReleaseInterface(ohusb_interface *iface)
{
    if (iface != nullptr) {
        for (uint8_t i = 0; i < iface->num_altsetting; i++) {
            ReleaseInterfaceDescriptor((iface->altsetting) + i);
        }
        iface->num_altsetting = 0;
        SAFE_DELETE_ARRAY(iface);
    }
}

void ReleaseConfigDescriptor(ohusb_config_descriptor *confDesc)
{
    if (confDesc != nullptr) {
        for (uint8_t i = 0; i < confDesc->bNumInterfaces; i++) {
            ReleaseInterface((confDesc->interface) + i);
        }
        confDesc->bNumInterfaces = 0;
        SAFE_DELETE_ARRAY(confDesc);
    }
}

void ReleaseDeviceDescriptor(ohusb_device_descriptor *devDesc)
{
    if (devDesc != nullptr) {
        for (uint8_t i = 0; i < devDesc->bNumConfigurations; i++) {
            ReleaseConfigDescriptor((devDesc->config) + i);
        }
        devDesc->bNumConfigurations = 0;
        SAFE_DELETE_ARRAY(devDesc);
    }
}

void DumpDeviceDescriptor(ohusb_device_descriptor *devDesc)
{
    fprintf(stderr, "DEBUG: ------------------begin DeviceDescriptor------------------\n");
    fprintf(stderr, "DEBUG: busNum = %x\n", devDesc->busNum);
    fprintf(stderr, "DEBUG: devAddr = %x\n", devDesc->devAddr);
    fprintf(stderr, "DEBUG: bcdUSB = %d\n", devDesc->bcdUSB);
    fprintf(stderr, "DEBUG: bDeviceClass = %x\n", devDesc->bDeviceClass);
    fprintf(stderr, "DEBUG: bDeviceSubClass = %x\n", devDesc->bDeviceSubClass);
    fprintf(stderr, "DEBUG: bDeviceProtocol = %x\n", devDesc->bDeviceProtocol);
    fprintf(stderr, "DEBUG: bMaxPacketSize0 = %x\n", devDesc->bMaxPacketSize0);
    fprintf(stderr, "DEBUG: idVendor = %d\n", devDesc->idVendor);
    fprintf(stderr, "DEBUG: idProduct = %d\n", devDesc->idProduct);
    fprintf(stderr, "DEBUG: bcdDevice = %d\n", devDesc->bcdDevice);
    fprintf(stderr, "DEBUG: iManufacturer = %x\n", devDesc->iManufacturer);
    fprintf(stderr, "DEBUG: iProduct = %x\n", devDesc->iProduct);
    fprintf(stderr, "DEBUG: iSerialNumber = %x\n", devDesc->iSerialNumber);
    fprintf(stderr, "DEBUG: bNumConfigurations = %x\n", devDesc->bNumConfigurations);
    fprintf(stderr, "DEBUG: ------------------end DeviceDescriptor------------------\n");
    return;
}

void DumpConfigDescriptor(ohusb_config_descriptor *confDesc)
{
    fprintf(stderr, "DEBUG: ------------------begin ConfigDescriptor------------------\n");
    fprintf(stderr, "DEBUG: iConfiguration = %x\n", confDesc->iConfiguration);
    fprintf(stderr, "DEBUG: bmAttributes = %x\n", confDesc->bmAttributes);
    fprintf(stderr, "DEBUG: MaxPower = %x\n", confDesc->MaxPower);
    fprintf(stderr, "DEBUG: bNumInterfaces = %x\n", confDesc->bNumInterfaces);
    fprintf(stderr, "DEBUG: ------------------end ConfigDescriptor------------------\n");
    return;
}

void DumpInterfaceDescriptor(ohusb_interface_descriptor *ifaceDesc)
{
    fprintf(stderr, "DEBUG: ------------------begin InterfaceDescriptor------------------\n");
    fprintf(stderr, "DEBUG: bInterfaceNumber = %x\n", ifaceDesc->bInterfaceNumber);
    fprintf(stderr, "DEBUG: bAlternateSetting = %x\n", ifaceDesc->bAlternateSetting);
    fprintf(stderr, "DEBUG: bInterfaceClass = %x\n", ifaceDesc->bInterfaceClass);
    fprintf(stderr, "DEBUG: bInterfaceSubClass = %x\n", ifaceDesc->bInterfaceSubClass);
    fprintf(stderr, "DEBUG: bInterfaceProtocol = %x\n", ifaceDesc->bInterfaceProtocol);
    fprintf(stderr, "DEBUG: bNumEndpoints = %x\n", ifaceDesc->bNumEndpoints);
    fprintf(stderr, "DEBUG: ------------------end InterfaceDescriptor------------------\n");
    return;
}

void DumpEndpointDescriptor(ohusb_endpoint_descriptor *eptDesc)
{
    fprintf(stderr, "DEBUG: ------------------begin EndpointDescriptor------------------\n");
    fprintf(stderr, "DEBUG: bDescriptorType = %x\n", eptDesc->bDescriptorType);
    fprintf(stderr, "DEBUG: bEndpointAddress = %x\n", eptDesc->bEndpointAddress);
    fprintf(stderr, "DEBUG: bmAttributes = %x\n", eptDesc->bmAttributes);
    fprintf(stderr, "DEBUG: wMaxPacketSize = %d\n", eptDesc->wMaxPacketSize);
    fprintf(stderr, "DEBUG: bInterval = %x\n", eptDesc->bInterval);
    fprintf(stderr, "DEBUG: bInterfaceId = %x\n", eptDesc->bInterfaceId);
    fprintf(stderr, "DEBUG: direction = %d\n", eptDesc->direction);
    fprintf(stderr, "DEBUG: ------------------end EndpointDescriptor------------------\n");
    return;
}

char *CopyString(const std::string &source)
{
    auto len = source.length();
    char *dest = new (std::nothrow) char[len + 1];
    if (dest == nullptr) {
        fprintf(stderr, "DEBUG: CopyString allocate failed\n");
        return nullptr;
    }
    if (strcpy_s(dest, len + 1, source.c_str()) != 0) {
        fprintf(stderr, "DEBUG: CopyString strcpy_s failed\n");
    }
    dest[len] = '\0';
    return dest;
}

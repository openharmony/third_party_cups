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

#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <cups/string-private.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#define OHUSB_TRANSFER_TYPE_MASK        0x03    /* in bmAttributes */
#define OHUSB_ENDPOINT_DIR_MASK		    0x80

const int OHUSB_CLASS_PRINTER = 7;
const uint16_t OHUSB_LANGUAGE_ID_ENGLISH = 0x409;
const int32_t OHUSB_GET_STRING_DESCRIPTOR_TIMEOUT = 1000;
const int32_t OHUSB_STRING_DESCRIPTOR_LENGTH_INDEX = 0;

const int32_t OHUSB_ENDPOINT_MAX_LENGTH = 512;
const int32_t OHUSB_CONTROLTRANSFER_READ_SLEEP = 100;
const int32_t OHUSB_CONTROLTRANSFER_READ_RETRY_MAX_TIMES = 20;
const int32_t OHUSB_BULKTRANSFER_WRITE_SLEEP = 1000;
const int32_t OHUSB_WRITE_RETRY_MAX_TIMES = 60;
const int32_t OHUSB_BULKTRANSFER_READ_TIMEOUT = 5000;

/*
 * Transfer type
 */
typedef enum {
	/** Control transfer */
    OHUSB_TRANSFER_TYPE_CONTROL = 0U,

    /** Isochronous transfer */
    OHUSB_TRANSFER_TYPE_ISOCHRONOUS = 1U,

    /** Bulk transfer */
    OHUSB_TRANSFER_TYPE_BULK = 2U,

    /** Interrupt transfer */
    OHUSB_TRANSFER_TYPE_INTERRUPT = 3U,

    /** Bulk stream transfer */
    OHUSB_TRANSFER_TYPE_BULK_STREAM = 4U
} ohusb_transfer_type;

/*
 * Request type bits.
 */
typedef enum {
	/** Standard */
    OHUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

    /** Class */
    OHUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

    /** Vendor */
    OHUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

    /** Reserved */
    OHUSB_REQUEST_TYPE_RESERVED = (0x03 << 5)
} ohusb_request_type;

/*
 * Endpoint direction.
 */
typedef enum {
	/** Out: host-to-device */
    OHUSB_ENDPOINT_OUT = 0x00,

    /** In: device-to-host */
    OHUSB_ENDPOINT_IN = 0x80
} ohusb_endpoint_direction;

/*
 * Recipient bits of the requestType.
 */
typedef enum {
	/** Device */
    OHUSB_RECIPIENT_DEVICE = 0x00,

    /** Interface */
    OHUSB_RECIPIENT_INTERFACE = 0x01,

    /** Endpoint */
    OHUSB_RECIPIENT_ENDPOINT = 0x02,

    /** Other */
    OHUSB_RECIPIENT_OTHER = 0x03
} ohusb_request_recipient;

/*
 * Standard requests
 */
typedef enum {
	/** Request status of the specific recipient */
    OHUSB_REQUEST_GET_STATUS = 0x00,
    
    /** Clear or disable a specific feature */
    OHUSB_REQUEST_CLEAR_FEATURE = 0x01,
    
    /* 0x02 is reserved */
    
    /** Set or enable a specific feature */
    OHUSB_REQUEST_SET_FEATURE = 0x03,
    
    /* 0x04 is reserved */
    
    /** Set device address for all future accesses */
    OHUSB_REQUEST_SET_ADDRESS = 0x05,
    
    /** Get the specified descriptor */
    OHUSB_REQUEST_GET_DESCRIPTOR = 0x06,
    
    /** Used to update existing descriptors or add new descriptors */
    OHUSB_REQUEST_SET_DESCRIPTOR = 0x07,
    
    /** Get the current device configuration value */
    OHUSB_REQUEST_GET_CONFIGURATION = 0x08,
    
    /** Set device configuration */
    OHUSB_REQUEST_SET_CONFIGURATION = 0x09,
    
    /** Return the selected alternate setting for the specified interface */
    OHUSB_REQUEST_GET_INTERFACE = 0x0a,

    /** Select an alternate interface for the specified interface */
    OHUSB_REQUEST_SET_INTERFACE = 0x0b,

    /** Set then report an endpoint's synchronization frame */
    OHUSB_REQUEST_SYNCH_FRAME = 0x0c,

    /** Sets both the U1 and U2 Exit Latency */
    OHUSB_REQUEST_SET_SEL = 0x30,

    /** Delay from the time a host transmits a packet to the time it is
      * received by the device. */
    OHUSB_SET_ISOCH_DELAY = 0x31
} ohusb_standard_request;

/*
 * Descriptor types as defined by the USB specification.
 */
typedef enum {
	/** Device descriptor */
	OHUSB_DT_DEVICE = 0x01,

	/** Configuration descriptor */
	OHUSB_DT_CONFIG = 0x02,

	/** String descriptor */
    OHUSB_DT_STRING = 0x03,

    /** Interface descriptor */
    OHUSB_DT_INTERFACE = 0x04,

    /** Endpoint descriptor */
    OHUSB_DT_ENDPOINT = 0x05,

    /** Interface Association Descriptor */
    OHUSB_DT_INTERFACE_ASSOCIATION = 0x0b,

    /** BOS descriptor */
    OHUSB_DT_BOS = 0x0f,

    /** Device Capability descriptor */
    OHUSB_DT_DEVICE_CAPABILITY = 0x10,

    /** HID descriptor */
    OHUSB_DT_HID = 0x21,

    /** HID report descriptor */
    OHUSB_DT_REPORT = 0x22,

    /** Physical descriptor */
    OHUSB_DT_PHYSICAL = 0x23,

    /** Hub descriptor */
    OHUSB_DT_HUB = 0x29,

    /** SuperSpeed Hub descriptor */
    OHUSB_DT_SUPERSPEED_HUB = 0x2a,

    /** SuperSpeed Endpoint Companion descriptor */
    OHUSB_DT_SS_ENDPOINT_COMPANION = 0x30
} ohusb_descriptor_type;

/**
 * Error codes.
 */
typedef enum {
	/** Success (no error) */
	OHUSB_SUCCESS = 0,

	/** Input/output error */
	OHUSB_ERROR_IO = -1,

	/** Invalid parameter */
	OHUSB_ERROR_INVALID_PARAM = -2,

	/** Access denied (insufficient permissions) */
	OHUSB_ERROR_ACCESS = -3,

	/** No such device (it may have been disconnected) */
	OHUSB_ERROR_NO_DEVICE = -4,

	/** Entity not found */
	OHUSB_ERROR_NOT_FOUND = -5,

	/** Resource busy */
	OHUSB_ERROR_BUSY = -6,

	/** Operation timed out */
	OHUSB_ERROR_TIMEOUT = -7,

	/** Overflow */
	OHUSB_ERROR_OVERFLOW = -8,

	/** Pipe error */
	OHUSB_ERROR_PIPE = -9,

	/** System call interrupted (perhaps due to signal) */
	OHUSB_ERROR_INTERRUPTED = -10,

	/** Insufficient memory */
	OHUSB_ERROR_NO_MEM = -11,

	/** Operation not supported or unimplemented on this platform */
	OHUSB_ERROR_NOT_SUPPORTED = -12,

	/** Other error */
	OHUSB_ERROR_OTHER = -99
} ohusb_error;

/**
 * A structure representing the parameter for usb control transfer.
 */
typedef struct {
    /** Request type */
    int32_t requestType;

    /** Request */
    int32_t request;

    /** Value. Varies according to request */
    int32_t value;

    /** Index. Varies according to request, typically used to pass an index
	 * or offset */
    int32_t index;

    /** timeout */
    int32_t timeout;
} ohusb_control_transfer_parameter;

/**
 * A structure representing the standard USB endpoint descriptor.
 */
typedef struct {
    /** Descriptor type */
    uint8_t  bDescriptorType;

    /** The address of the endpoint described by this descriptor. Bits 0:3 are
	 * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction. */
    uint8_t  bEndpointAddress;

    /** Attributes which apply to the endpoint when it is configured using
	 * the bConfigurationValue. Bits 0:1 determine the transfer type. Bits 2:3 are
     * only used for isochronous endpoints. Bits 4:5 are also only used for
     * isochronous endpoints . Bits 6:7 are reserved. */
    uint8_t  bmAttributes;

    /** Maximum packet size this endpoint is capable of sending/receiving. */
    uint16_t wMaxPacketSize;

    /** Interval for polling endpoint for data transfers. */
    uint8_t  bInterval;

    /** The interface id the endpoint belongs to. */
    uint8_t  bInterfaceId;

    /** The direction of the endpoint. */
    uint32_t direction;
} ohusb_endpoint_descriptor;

/**
 * A structure representing the standard USB interface descriptor.
 */
typedef struct {
    /** Number of this interface */
    uint8_t  bInterfaceNumber;

    /** Value used to select this alternate setting for this interface */
    uint8_t  bAlternateSetting;

    /** USB-IF class code for this interface. */
    uint8_t  bInterfaceClass;

    /** USB-IF subclass code for this interface, qualified by the
	 * bInterfaceClass value */
    uint8_t  bInterfaceSubClass;

    /** USB-IF protocol code for this interface, qualified by the
	 * bInterfaceClass and bInterfaceSubClass values */
    uint8_t  bInterfaceProtocol;

    /** Array of endpoint descriptors. This length of this array is determined
	 * by the bNumEndpoints field. */
    ohusb_endpoint_descriptor *endpoint;

    /** Number of endpoints used by this interface (excluding the control
	 * endpoint). */
    uint8_t  bNumEndpoints;
} ohusb_interface_descriptor;

/**
 * A collection of alternate settings for a particular USB interface.
 */
typedef struct {
    /** Array of interface descriptors. The length of this array is determined
	 * by the num_altsetting field. */
    ohusb_interface_descriptor *altsetting;

    /** The number of alternate settings that belong to this interface.
	 * Must be non-negative. */
    int num_altsetting;
} ohusb_interface;

/**
 * A structure representing the standard USB configuration descriptor.
 */
typedef struct {
    /** Identifier value for this configuration */
    uint8_t iConfiguration;

    /** Configuration characteristics */
    uint8_t bmAttributes;

    /** Maximum power consumption of the USB device from this bus in this
	 * configuration when the device is fully operation. Expressed in units
	 * of 2 mA when the device is operating in high-speed mode and in units
	 * of 8 mA when the device is operating in super-speed mode. */
    uint8_t MaxPower;

    /** Array of interfaces supported by this configuration. The length of
	 * this array is determined by the bNumInterfaces field. */
    ohusb_interface *interface;

    /** Number of interfaces supported by this configuration */
    uint8_t bNumInterfaces;
} ohusb_config_descriptor;

/**
 * A structure representing the standard USB device descriptor.
 */
typedef struct {
    /** The bus num of the usb device. */
    uint8_t busNum;

    /** The device address of the usb device. */
    uint8_t devAddr;

    /** USB specification release number in binary-coded decimal. A value of
	 * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
    uint16_t bcdUSB;

    /** USB-IF class code for the device. */
    uint8_t bDeviceClass;

    /** USB-IF subclass code for the device, qualified by the bDeviceClass
	 * value */
    uint8_t bDeviceSubClass;

    /** USB-IF protocol code for the device, qualified by the bDeviceClass and
	 * bDeviceSubClass values */
    uint8_t bDeviceProtocol;

    /** Maximum packet size for endpoint 0 */
    uint8_t bMaxPacketSize0;

    /** USB-IF vendor ID */
    uint16_t idVendor;

    /** USB-IF product ID */
    uint16_t idProduct;

    /** Device release number in binary-coded decimal */
    uint16_t bcdDevice;

    /** Index of string descriptor describing manufacturer */
    uint8_t iManufacturer;

    /** Index of string descriptor describing product */
    uint8_t iProduct;

    /** Index of string descriptor containing device serial number */
    uint8_t iSerialNumber;

    /** Array of configs supported by this device. The length of
	 * this array is determined by the bNumConfigurations field. */
    ohusb_config_descriptor *config;

    /** Number of configs supported by this device */
    uint8_t bNumConfigurations;
} ohusb_device_descriptor;

/**
 * A structure representing the pipe information to transfer data.
 */
typedef struct {
    /** The bus num of the usb device. */
    uint8_t busNum;

    /** The device address of the usb device. */
    uint8_t devAddr;
} ohusb_pipe;

/**
 * A structure representing the endpoint information to transfer data.
 */
typedef struct {
    /** The interface id to which the endpoint to transmit data belongs. */
    uint8_t bInterfaceId;

    /** The endpoint address to transfer data. */
    uint8_t bEndpointAddress;

    /** The interface id to be claimed. */
    int32_t bClaimedInterfaceId;
} ohusb_transfer_pipe;

extern int32_t OH_GetDevices(ohusb_device_descriptor** list, ssize_t *numdevs);
extern int32_t OH_OpenDevice(ohusb_device_descriptor *devDesc, ohusb_pipe **pipe);
extern int32_t OH_CloseDevice(ohusb_pipe *pipe);
extern int32_t OH_ClaimInterface(ohusb_pipe *pipe, int interfaceId, bool force);
extern int32_t OH_ReleaseInterface(ohusb_pipe *pipe, int interfaceId);
extern int32_t OH_BulkTransferRead(
    ohusb_pipe *pipe, ohusb_transfer_pipe *tpipe, unsigned char *data, int length, int *transferred);
extern int32_t OH_BulkTransferWrite(
    ohusb_pipe *pipe, ohusb_transfer_pipe *tpipe, unsigned char *data, int length, int *transferred, int32_t timeout);
extern int32_t OH_ControlTransferRead(
    ohusb_pipe *pipe, ohusb_control_transfer_parameter *ctrlParam, unsigned char *data, uint16_t length);
extern int32_t OH_ControlTransferWrite(
    ohusb_pipe *pipe, ohusb_control_transfer_parameter *ctrlParam, unsigned char *data, uint16_t length);
extern int32_t OH_GetStringDescriptor(ohusb_pipe *pipe, int descId, unsigned char *descriptor, int length);
extern int32_t OH_SetConfiguration(ohusb_pipe *pipe, int configIndex);
extern int32_t OH_SetInterface(ohusb_pipe *pipe, int interfaceId, int altIndex);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* USB_MANAGER_H */

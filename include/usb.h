#ifndef SOLIX_USB_H
#define SOLIX_USB_H

#include "types.h"
#include "kernel.h"

/**
 * SolixOS USB Driver Framework
 * Modern USB host controller support with UHCI, OHCI, EHCI, and xHCI
 * Version: 2.0
 */

// USB specification constants
#define USB_VERSION_1_1  0x0110
#define USB_VERSION_2_0  0x0200
#define USB_VERSION_3_0  0x0300

#define USB_MAX_DEVICES  128
#define USB_MAX_ENDPOINTS 32
#define USB_MAX_CONFIGURATIONS 8
#define USB_MAX_INTERFACES 32

// USB speeds
#define USB_SPEED_LOW    0  // 1.5 Mbps
#define USB_SPEED_FULL   1  // 12 Mbps
#define USB_SPEED_HIGH   2  // 480 Mbps
#define USB_SPEED_SUPER  3  // 5 Gbps
#define USB_SPEED_UNKNOWN 0xFF

// USB endpoint types
#define USB_ENDPOINT_CONTROL 0
#define USB_ENDPOINT_ISOC    1
#define USB_ENDPOINT_BULK    2
#define USB_ENDPOINT_INT     3

// USB transfer directions
#define USB_DIR_OUT 0
#define USB_DIR_IN  0x80

// USB standard requests
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE     0x0A
#define USB_REQ_SET_INTERFACE     0x0B
#define USB_REQ_SYNCH_FRAME       0x0C

// USB descriptor types
#define USB_DESC_DEVICE           0x01
#define USB_DESC_CONFIG           0x02
#define USB_DESC_STRING           0x03
#define USB_DESC_INTERFACE        0x04
#define USB_DESC_ENDPOINT         0x05
#define USB_DESC_DEVICE_QUALIFIER 0x06
#define USB_DESC_OTHER_SPEED      0x07
#define USB_DESC_INTERFACE_POWER  0x08

// USB device classes
#define USB_CLASS_PER_INTERFACE   0x00
#define USB_CLASS_AUDIO           0x01
#define USB_CLASS_COMM            0x02
#define USB_CLASS_HID             0x03
#define USB_CLASS_PHYSICAL        0x05
#define USB_CLASS_IMAGE           0x06
#define USB_CLASS_PRINTER         0x07
#define USB_CLASS_MASS_STORAGE    0x08
#define USB_CLASS_HUB             0x09
#define USB_CLASS_CDC_DATA        0x0A
#define USB_CLASS_CSCID           0x0B
#define USB_CLASS_CONTENT_SEC     0x0D
#define USB_CLASS_VIDEO           0x0E
#define USB_CLASS_PERSONAL_HEALTH 0x0F
#define USB_CLASS_AUDIO_VIDEO     0x10
#define USB_CLASS_BILLBOARD       0x11
#define USB_CLASS_TYPE_C_BRIDGE   0x12
#define USB_CLASS_DIAGNOSTIC      0xDC
#define USB_CLASS_WIRELESS        0xE0
#define USB_CLASS_MISCELLANEOUS   0xEF
#define USB_CLASS_VENDOR_SPEC     0xFF

// USB endpoint descriptor
typedef struct usb_endpoint_desc {
    uint8_t  length;
    uint8_t  type;
    uint8_t  endpoint_address;
    uint8_t  attributes;
    uint16_t max_packet_size;
    uint8_t  interval;
} __packed usb_endpoint_desc_t;

// USB interface descriptor
typedef struct usb_interface_desc {
    uint8_t  length;
    uint8_t  type;
    uint8_t  interface_number;
    uint8_t  alternate_setting;
    uint8_t  num_endpoints;
    uint8_t  interface_class;
    uint8_t  interface_subclass;
    uint8_t  interface_protocol;
    uint8_t  interface;
} __packed usb_interface_desc_t;

// USB configuration descriptor
typedef struct usb_config_desc {
    uint8_t  length;
    uint8_t  type;
    uint16_t total_length;
    uint8_t  num_interfaces;
    uint8_t  configuration_value;
    uint8_t  configuration;
    uint8_t  attributes;
    uint8_t  max_power;
} __packed usb_config_desc_t;

// USB device descriptor
typedef struct usb_device_desc {
    uint8_t  length;
    uint8_t  type;
    uint16_t usb_version;
    uint8_t  device_class;
    uint8_t  device_subclass;
    uint8_t  device_protocol;
    uint8_t  max_packet_size0;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_version;
    uint8_t  manufacturer;
    uint8_t  product;
    uint8_t  serial_number;
    uint8_t  num_configurations;
} __packed usb_device_desc_t;

// USB device structure
typedef struct usb_device {
    uint32_t id;
    uint8_t  speed;
    uint8_t  address;
    uint8_t  configuration;
    uint8_t  num_configurations;
    uint8_t  num_interfaces;
    
    usb_device_desc_t device_desc;
    usb_config_desc_t* config_descs;
    usb_interface_desc_t* interface_descs;
    usb_endpoint_desc_t* endpoint_descs;
    
    void* driver_data;
    struct usb_device* next;
} usb_device_t;

// USB endpoint structure
typedef struct usb_endpoint {
    uint8_t  address;
    uint8_t  attributes;
    uint16_t max_packet_size;
    uint8_t  interval;
    uint8_t  direction;
    uint8_t  type;
    
    void* data_toggle;
    void* schedule_data;
} usb_endpoint_t;

// USB host controller operations
struct usb_hcd_ops {
    int (*start)(struct usb_hcd* hcd);
    void (*stop)(struct usb_hcd* hcd);
    int (*urb_enqueue)(struct usb_hcd* hcd, struct urb* urb);
    int (*urb_dequeue)(struct usb_hcd* hcd, struct urb* urb);
    void (*hub_control)(struct usb_hcd* hcd, uint16_t type_req, uint16_t value, 
                       uint16_t index, uint8_t* buf, uint16_t length);
};

// USB host controller
typedef struct usb_hcd {
    uint32_t id;
    uint32_t type;  // UHCI, OHCI, EHCI, xHCI
    void* regs;
    struct usb_hcd_ops* ops;
    
    uint32_t irq;
    uint8_t num_ports;
    uint8_t port_status[16];  // Max 16 ports per controller
    
    struct usb_hcd* next;
} usb_hcd_t;

// USB request block (URB)
typedef struct urb {
    usb_device_t* dev;
    usb_endpoint_t* ep;
    uint8_t* buffer;
    uint32_t buffer_length;
    uint32_t actual_length;
    uint32_t transfer_flags;
    uint32_t status;
    
    void (*complete)(struct urb* urb);
    void* context;
} urb_t;

// USB driver interface
typedef struct usb_driver {
    const char* name;
    uint32_t driver_type;
    
    int (*probe)(usb_device_t* dev);
    void (*disconnect)(usb_device_t* dev);
    
    uint32_t device_class;
    uint32_t device_subclass;
    uint32_t device_protocol;
    
    struct usb_driver* next;
} usb_driver_t;

// USB core functions
API int usb_init(void);
API int usb_register_hcd(usb_hcd_t* hcd);
API int usb_unregister_hcd(usb_hcd_t* hcd);
API int usb_register_driver(usb_driver_t* driver);
API int usb_unregister_driver(usb_driver_t* driver);

API usb_device_t* usb_alloc_device(void);
API void usb_free_device(usb_device_t* dev);
API int usb_set_address(usb_device_t* dev, uint8_t address);
API int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, 
                          void* buf, uint16_t length);

API urb_t* usb_alloc_urb(void);
API void usb_free_urb(urb_t* urb);
API int usb_submit_urb(urb_t* urb);
API int usb_unlink_urb(urb_t* urb);

// USB hub functions
API int usb_hub_init(void);
API void usb_hub_port_connect_change(usb_hcd_t* hcd, int port);
API void usb_hub_port_disconnect(usb_hcd_t* hcd, int port);

// USB device classes
API int usb_hid_init(void);
API int usb_mass_storage_init(void);
API int usb_cdc_init(void);

// Host controller specific functions
API int uhci_init(void);
API int ohci_init(void);
API int ehci_init(void);
API int xhci_init(void);

#endif

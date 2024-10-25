// See: https://stackoverflow.com/a/14798763/1141958

#include <stddef.h> // ssize_t

#define Pint_(v,fmt) fprintf(stderr, "  %25s = " fmt "\n", #v, (int) (v))
#define Pint(v) Pint_(v, "%d")
#define Pstr(v) fprintf(stderr, "  %25s = %s \n", #v, (char*) (v))
#define Pptr(v) fprintf(stderr, "  %25s = %p \n", #v, (void*) (v))

#define printf(fmt, ...) fprintf(stderr, fmt, ## __VA_ARGS__)

#define BULK_EP_OUT     0x82
#define BULK_EP_IN      0x08

static int alt_interface = 0, interface_number = 0;

static
int print_config_descriptor(struct libusb_device_handle *hDevice, struct libusb_config_descriptor *config)
{
    printf("\n\n  Number of Interfaces : %d",config->bNumInterfaces);
    printf("\n  Length : %d",config->bLength);
    printf("\n  Desc_Type : %d",config->bDescriptorType);
    printf("\n  Config_index : %d",config->iConfiguration);
    printf("\n  Total length : %lu", (unsigned long) config->wTotalLength);
    printf("\n  Configuration Value  : %d",config->bConfigurationValue);
    printf("\n  Configuration Attributes : %d",config->bmAttributes);
    printf("\n  MaxPower(mA) : %d\n",config->MaxPower);
    return 0;
}

static
const struct libusb_endpoint_descriptor* print_config(struct libusb_device *dev, struct libusb_device_handle *handle, int config_i)
{
    int usb_result = 0;
    const struct libusb_endpoint_descriptor *endpoint = 0;
    struct libusb_config_descriptor *config = 0;
    int altsetting_index, interface_index = 0;

    usb_result = libusb_get_config_descriptor(dev, config_i, &config);
    if ( usb_result ) return 0;
    usb_result = print_config_descriptor(handle, config);
    if ( usb_result ) return 0;

    for(interface_index=0;interface_index<config->bNumInterfaces;interface_index++)
    {
        const struct libusb_interface *iface = (void*) &config->interface[interface_index];
        for(altsetting_index=0;altsetting_index<iface->num_altsetting;altsetting_index++)
        {
            const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_index];

            int endpoint_index;
            for(endpoint_index=0;endpoint_index<altsetting->bNumEndpoints;endpoint_index++)
            {
                const struct libusb_endpoint_desriptor *ep = (void*) &altsetting->endpoint[endpoint_index];
                endpoint = (void*) ep;
                alt_interface = altsetting->bAlternateSetting;
                interface_number = altsetting->bInterfaceNumber;
            }

            printf("\n    Endpoint Address : 0x0%x",endpoint->bEndpointAddress);
            printf("\n    Size of EndPoint Descriptor : %d",endpoint->bLength);
            printf("\n    Type of Descriptor : %d",endpoint->bDescriptorType);
            printf("\n    Maximum Packet Size: %x",endpoint->wMaxPacketSize);
            printf("\n    Attributes applied to Endpoint: %d",endpoint->bmAttributes);
            printf("\n    Interval for Polling for data Transfer : %d\n",endpoint->bInterval);
        }
    }
    libusb_free_config_descriptor(config);
    return endpoint;
}

static
void print_descriptor(libusb_device_handle* handle, struct libusb_device_descriptor* descp) {
  struct libusb_device_descriptor desc = *descp;
  printf("\nVendor ID                  : %04x",desc.idVendor);
  printf("\nProduct ID                 : %04x",desc.idProduct);
  printf("\nSerial Number              : %x",desc.iSerialNumber);
  printf("\nSize of Device Descriptor  : %d",desc.bLength);
  printf("\nType of Descriptor         : %d",desc.bDescriptorType);
  printf("\nUSB Spec Release Number    : %d",desc.bcdUSB);
  printf("\nDevice Release Number      : %d",desc.bcdDevice);
  printf("\nDevice Class               : %d",desc.bDeviceClass);
  printf("\nDevice Sub-Class           : %d",desc.bDeviceSubClass);
  printf("\nDevice Protocol            : %d",desc.bDeviceProtocol);
  printf("\nMax. Packet Size           : %d",desc.bMaxPacketSize0);

  if ( handle ) {
    char str1[64], str2[64];
    memset(str1, 0, sizeof(str1));
    libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));
    memset(str2, 0, sizeof(str2));
    libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));
    printf("\nManufactured : %s",str1);
    printf("\nProduct : %s",str2);
  }
}

void luxeed_device_show(luxeed_device* dev) {
  Pptr(dev);
  Pptr(dev->u_dev);
  Pptr(dev->u_dev_hd);
  if ( dev->u_dev_hd ) {
    struct libusb_device_descriptor desc = dev->u_dev_desc;
    print_descriptor(dev->u_dev_hd, &desc);
    printf("\nNo. of Configurations : %d",desc.bNumConfigurations);
    for ( int config_i = 0; config_i < desc.bNumConfigurations; config_i ++ )
      print_config(dev->u_dev, dev->u_dev_hd, config_i);
    printf("\n");
  }
}

#undef printf

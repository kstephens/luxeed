#!/usr/bin/env python3.11

import usb.core
import usb.util
from icecream import ic

#######################################

LUXEED_USB_VENDOR = 0x534B
LUXEED_USB_PRODUCT = 0x0600
LUXEED_USB_INTERFACE = 1
LUXEED_USB_ENDPOINT_DATA = 0x02
LUXEED_USB_ENDPOINT_CONTROL = 0x82

#######################################

def list_all_devices():
  for dev in usb.core.find(find_all=True):
      print(dev)

def main():
  dev = usb.core.find(idVendor=LUXEED_USB_VENDOR, idProduct=LUXEED_USB_PRODUCT)
  #  print(dev)
  ic(dev)

  dev.set_configuration()

  cfg = dev.get_active_configuration()
  #  print(cfg)
  ic(type(cfg))
  ic(cfg)

  intf = cfg[(LUXEED_USB_INTERFACE,0)]
  ic(type(intf))
  ic(intf)
  # print(intf)

  ep = usb.util.find_descriptor(
      intf,
      # match the first OUT endpoint
      custom_match = \
      lambda e: \
          e.bEndpointAddress == LUXEED_USB_ENDPOINT_DATA and \
          usb.util.endpoint_direction(e.bEndpointAddress) == \
          usb.util.ENDPOINT_OUT)

  ic(type(ep))
  ic(ep)
  print(ep)

#######################################

'''
/* Parsed from ep01.txt as initialization, minus the leading 0x02.
   Chunks are sent 65 bytes long.
   Checksum appears at 0x14e.
*/
'''

#######################################

main()

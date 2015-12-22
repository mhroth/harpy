/* Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved. */

// $ clang midi2osc.c ./tinyosc/tinyosc.c -I/usr/local/include -L/usr/local/lib -lusb-1.0 -o midi2osc
// $ midi2osc 192.168.0.33 9000

#include <arpa/inet.h>
#include <libusb-1.0/libusb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for close

#include "tinyosc/tinyosc.h" // OSC support

#define KORG_NANOKONTROL2_VENDOR_ID 0x0944
#define KORG_NANOKONTROL2_PRODUCT_ID 0x0117
#define KORG_NANOKONTROL2_ENDPOINT 0x81

#define USB_BULK_TRANSFER_TIMEOUT_MS 1000

static void printInfoForNonSystemDevice(libusb_device *device);
const char *getOscAddressForControl(const unsigned char c);

static volatile bool _keepRunning = true;

static void sigintHandler(int x) {
  _keepRunning = false; // handle Ctrl+C
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: midi2osc <r:IP address> <r:port> <o:USB interface index>\n");
    return 0;
  }

  signal(SIGINT, &sigintHandler); // register the SIGINT handler

  // initialise the error return value
  int err = 0;

  // initialise send socket
  const int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(atoi(argv[1]));
  inet_pton(AF_INET, argv[0], &sin.sin_addr);
  err = connect(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
  if (err != 0) {
    printf("Failed to open OSC socket: %i\n", err);
    return -1;
  }

  libusb_context *usbctx = NULL;
  libusb_init(&usbctx);

  libusb_device **device_list = NULL;
  const ssize_t numDevices = libusb_get_device_list(usbctx, &device_list);
  for (int i = 0; i < numDevices; i++) {
    libusb_device *device = device_list[i];
    printInfoForNonSystemDevice(device_list[i]);
  }
  libusb_free_device_list(device_list, 1);

  // specifically open Korg Nanokontrol2
  libusb_device_handle *handle = libusb_open_device_with_vid_pid(usbctx,
      KORG_NANOKONTROL2_VENDOR_ID, KORG_NANOKONTROL2_PRODUCT_ID);
  if (handle == NULL) {
    printf("Could not open device with Vendor:Product Id %04X:%04X\n",
        KORG_NANOKONTROL2_VENDOR_ID, KORG_NANOKONTROL2_PRODUCT_ID);
  } else {
    libusb_device *device = libusb_get_device(handle);
    err = libusb_claim_interface(handle, 0);
    if (err != 0) {
      printf("Could not claim interface: %i\n", err);
    } else {
      const int maxPacketSize = libusb_get_max_packet_size(device, KORG_NANOKONTROL2_ENDPOINT);
      unsigned char usbBuffer[maxPacketSize];
      int usbLen = 0;
      while (_keepRunning) {
        while ((err = libusb_bulk_transfer(handle, KORG_NANOKONTROL2_ENDPOINT, usbBuffer, maxPacketSize, &usbLen, USB_BULK_TRANSFER_TIMEOUT_MS)) == 0) {
                    // printf("[%i] ", len);
                    // for (int j = 0; j < 4; j++) {
                    //   printf("%02X", usbBuffer[j]);
                    // }
                    // printf("\n");
          const char *address = getOscAddressForControl(usbBuffer[2]);
          if (address != NULL) {
            char oscBuffer[64]; // NOTE(mhroth): because we know that the OSC message will not be that long
            int oscLen = tosc_writeMessage(oscBuffer, sizeof(oscBuffer), address, "m", usbBuffer);
            send(fd, oscBuffer, oscLen, 0); // send the OSC message
          }
          break;
        }
      }

      libusb_release_interface(handle, 0);
    }
  }
  libusb_close(handle);

  close(fd); // close the send socket

  libusb_exit(usbctx); // exit the usb context
  return 0;
  //
  //   struct libusb_device_descriptor desc;
  //   libusb_get_device_descriptor(device, &desc);
  //
  //   libusb_device_handle *handle = NULL;
  //   libusb_open(device, &handle);
  //
  //   if (desc.idProduct == 0x117) {
  //     // libusb_reset_device(handle);
  //
  //     int config = 0;
  //     libusb_get_configuration(handle, &config);
  //     printf("current configuration: %i\n", config);
  //
  //     struct libusb_config_descriptor *configs = NULL;
  //     int err = libusb_get_active_config_descriptor(device, &configs);
  //     libusb_free_config_descriptor(configs);
  //
  //     err = libusb_claim_interface(handle, 0);
  //     // printf("libusb_claim_interface %i\n", err);
  //
  //     const int maxPacketSize = libusb_get_max_packet_size(device, 129 | LIBUSB_ENDPOINT_IN);
  //
  //     unsigned char data[maxPacketSize];
  //     int actual_length;
  //     while ((err = libusb_bulk_transfer(handle, (129 | LIBUSB_ENDPOINT_IN), data, maxPacketSize, &actual_length, 0)) == 0) {
  //       switch (err) {
  //         case 0: {
  //           printf("[%i] ", actual_length);
  //           for (int j = 0; j < 4; j++) {
  //             printf("%02X", data[j]);
  //           }
  //           printf("\n");
  //           break;
  //         }
  //         // http://libusb.sourceforge.net/api-1.0/group__misc.html
  //         case LIBUSB_ERROR_TIMEOUT: printf("LIBUSB_ERROR_TIMEOUT\n"); break;
  //         case LIBUSB_ERROR_PIPE: printf("LIBUSB_ERROR_PIPE\n"); break;
  //         case LIBUSB_ERROR_OVERFLOW: printf("LIBUSB_ERROR_OVERFLOW\n"); break;
  //         case LIBUSB_ERROR_NO_DEVICE: printf("LIBUSB_ERROR_NO_DEVICE\n"); break;
  //         case LIBUSB_ERROR_NOT_FOUND: printf("LIBUSB_ERROR_NOT_FOUND\n"); break;
  //         default: printf("unknown error: %i\n", err); break;
  //       }
  //     }
  //
  //     libusb_release_interface(handle, 0);
  //   }
  //
  //   libusb_close(handle);
  // }
  //
  // libusb_exit(usbctx);
  // return 0;
}

void printInfoForNonSystemDevice(libusb_device *device) {
  struct libusb_device_descriptor desc;
  libusb_get_device_descriptor(device, &desc);

  // NOTE(mhroth): skip over Apple devices
  if (desc.idVendor == 0x05AC || desc.idVendor == 0x0A5C) return;

  libusb_device_handle *handle = NULL;
  libusb_open(device, &handle);

  unsigned char productBuffer[64];
  libusb_get_string_descriptor_ascii(handle, desc.iProduct,
      productBuffer, sizeof(productBuffer));
  unsigned char manufBuffer[64];
  libusb_get_string_descriptor_ascii(handle, desc.iManufacturer,
      manufBuffer, sizeof(manufBuffer));
  printf("Vendor:Device (%s by %s) = 0x%04x:%04x\n",
      productBuffer, manufBuffer,
      desc.idVendor, desc.idProduct);

  for (int i = 0; i < desc.bNumConfigurations; i++) {
    struct libusb_config_descriptor *config = NULL;
    libusb_get_config_descriptor(device, i, &config);

    // TODO(mhroth): not sure about this part
    // unsigned char configBuffer[64];
    // libusb_get_string_descriptor_ascii(handle, config->iConfiguration,
    //     configBuffer, sizeof(configBuffer));
    printf("  * configuation %i\n", i);
    printf("    * bNumInterfaces: %i\n", config->bNumInterfaces);
    printf("    * MaxPower: %imA\n", config->MaxPower * 2);
    printf("    * interfaces: %i\n", config->bNumInterfaces);
    for (int j = 0; j < config->bNumInterfaces; j++) {
      printf("      * interface %i\n", j);
      struct libusb_interface interface = config->interface[j];
      printf("        * alt settings: %i\n", interface.num_altsetting);
      for (int k = 0; k < interface.num_altsetting; k++) {
        printf("          * alt setting: %i\n", k);
        struct libusb_interface_descriptor ifaceDesc = interface.altsetting[k];
        printf("            * endpoints: %i\n", ifaceDesc.bNumEndpoints);
        for (int m = 0; m < ifaceDesc.bNumEndpoints; m++) {
        printf("              * endpoint: %i\n", m);
        struct libusb_endpoint_descriptor endpoint = ifaceDesc.endpoint[m];
          printf("                * endpoint address: 0x%X %s\n",
              endpoint.bEndpointAddress,
              (endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) ? "in" : "out");
          printf("                * wMaxPacketSize: %i\n", endpoint.wMaxPacketSize);
        }
      }
    }

    libusb_free_config_descriptor(config);
  }

  libusb_close(handle);
}

const char *getOscAddressForControl(const unsigned char c) {
  switch (c) {
    case 0: return "/0/slider0";
    case 16: return "/0/knob0";
    case 7: return "/0/gain";
    case 23: return "/0/pan";
    default: return NULL;
  }
}
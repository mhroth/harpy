/* Copyright (c) 2015, Martin Roth (mhroth@gmail.com). All Rights Reserved. */

// clang midi2osc.c -I/usr/local/include -L/usr/local/lib -lusb-1.0 -o midi2osc

#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>

int main() {
  libusb_context *usbctx = NULL;
  libusb_init(&usbctx);

// #ifndef NDEBUG
//   libusb_set_debug(usbctx, 3); // max debugging level
// #endif

  libusb_device **device_list = NULL;
  const ssize_t numDevices = libusb_get_device_list(usbctx, &device_list);
  for (int i = 0; i < numDevices; i++) {
    libusb_device *device = device_list[i];

    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(device, &desc);

    libusb_device_handle *handle = NULL;
    libusb_open(device, &handle);

    unsigned char productBuffer[64];
    libusb_get_string_descriptor_ascii(handle, desc.iProduct, productBuffer, sizeof(productBuffer));
    unsigned char manufBuffer[64];
    libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, manufBuffer, sizeof(manufBuffer));
    printf("Vendor:Device (%s by %s) = %04x:%04x (%i:%i)\n",
        productBuffer, manufBuffer,
        desc.idVendor, desc.idProduct,
        desc.idVendor, desc.idProduct);

    if (desc.idProduct == 279) {
      libusb_reset_device(handle);

      int config = 0;
      libusb_get_configuration(handle, &config);
      printf("current configuration: %i\n", config);

      struct libusb_config_descriptor *configs = NULL;
      int err = libusb_get_active_config_descriptor(device, &configs);
      printf("libusb_get_active_config_descriptor %i\n", configs[0].interface[0].altsetting[0].endpoint[0].wMaxPacketSize);
      libusb_free_config_descriptor(configs);

      err = libusb_claim_interface(handle, 0);
      printf("libusb_claim_interface %i\n", err);

      int maxPacketSize = libusb_get_max_packet_size(device, 129 | LIBUSB_ENDPOINT_IN);
      printf("maxPacketSize: %i\n", maxPacketSize);

      unsigned char data[64];
      int actual_length;
      while ((err = libusb_bulk_transfer(handle, (129 | LIBUSB_ENDPOINT_IN), data, sizeof(data), &actual_length, 0)) == 0) {
        switch (err) {
          case 0: {
            printf("[%i] ", actual_length);
            for (int j = 0; j < actual_length; j++) {
              printf("%X", data[j]);
            }
            printf("\n");
            break;
          }
          // http://libusb.sourceforge.net/api-1.0/group__misc.html
          case LIBUSB_ERROR_TIMEOUT: printf("LIBUSB_ERROR_TIMEOUT\n"); break;
          case LIBUSB_ERROR_PIPE: printf("LIBUSB_ERROR_PIPE\n"); break;
          case LIBUSB_ERROR_OVERFLOW: printf("LIBUSB_ERROR_OVERFLOW\n"); break;
          case LIBUSB_ERROR_NO_DEVICE: printf("LIBUSB_ERROR_NO_DEVICE\n"); break;
          case LIBUSB_ERROR_NOT_FOUND: printf("LIBUSB_ERROR_NOT_FOUND\n"); break;
          default: printf("unknown error: %i\n", err); break;
        }
      }

      libusb_release_interface(handle, 0);
    }

    libusb_close(handle);
  }

  libusb_free_device_list(device_list, 1);
  libusb_exit(usbctx);
  return 0;
}

# Port layer: vendor-stack -> hardware adaptation.

if(MODULE_FATFS_ENABLE)
    add_subdirectory(fatfs_port)
endif()

if(MODULE_USB_DEVICE_ENABLE OR MODULE_USB_HOST_ENABLE)
    add_subdirectory(usb_port)
endif()

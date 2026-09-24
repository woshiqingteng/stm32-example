# Port layer: OS / filesystem / USB adaptation.

if(MODULE_FATFS_ENABLE)
    add_subdirectory(fatfs_port)
endif()

if(MODULE_USB_DEVICE_ENABLE OR MODULE_USB_HOST_ENABLE)
    add_subdirectory(usb_port)
endif()

if(MODULE_FREERTOS_ENABLE)
    add_subdirectory(freertos_port)
endif()

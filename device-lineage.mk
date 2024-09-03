#
# Copyright (C) 2024 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Audio
PRODUCT_VENDOR_PROPERTIES += \
    persist.vendor.aoc.firmware.disable_monitor_mode=true \
    ro.vendor.dolby.dax.version=DAX3_G_3.7.3.0_r1

# DRM
PRODUCT_VENDOR_PROPERTIES += \
    drm.service.enabled=true \
    media.mediadrmservice.enable=true

# Display
PRODUCT_PACKAGES += \
    libacryl_hdr_plugin

# GNSS
PRODUCT_PACKAGES += \
    android.hardware.location.gps.prebuilt.xml

PRODUCT_VENDOR_PROPERTIES += \
    persist.vendor.gps.hal.service.name=vendor

# Graphics
PRODUCT_VENDOR_PROPERTIES += \
    ro.surface_flinger.game_default_frame_rate_override=60

# HIDL
DEVICE_PRODUCT_COMPATIBILITY_MATRIX_FILE += \
    device/google/zumapro/device_framework_matrix_product_ext.xml

# RIL
PRODUCT_VENDOR_PROPERTIES += \
    keyguard.no_require_sim=true \
    persist.vendor.ril.ecc.use.xml=1 \
    persist.vendor.ril.support_nr_ds=1 \
    persist.vendor.ril.use_radio_hal=2.1 \
    ro.vendor.config.build_carrier=europen \
    vendor.rild.libpath=libsitril.so

# Sensors
PRODUCT_PACKAGES += \
    sensors.dynamic_sensor_hal

# USB
PRODUCT_VENDOR_PROPERTIES += \
    ro.usb.uvc.enabled=true

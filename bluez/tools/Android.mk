#
# Bluetooth tools for setup and debug
#			daniel_hk
LOCAL_PATH := external/bluetooth

# Retrieve BlueZ version from configure.ac file
BLUEZ_VERSION := $(shell grep ^AC_INIT $(LOCAL_PATH)/bluez/configure.ac | cpp -P -D'AC_INIT(_,v)=v')

# Specify pathmap for glib
#pathmap_INCL += glib:external/bluetooth/glib

# Specify common compiler flags
BLUEZ_COMMON_CFLAGS := -DVERSION=\"$(BLUEZ_VERSION)\" \
	-DSTORAGEDIR=$(ANDROID_STORAGEDIR) \

# Disable warnings enabled by Android but not enabled in autotools build
BLUEZ_COMMON_CFLAGS += -Wno-pointer-arith -Wno-missing-field-initializers

# @ daniel, extra tools for CSR
# hciconfig
#
ifneq ($(BOARD_HAVE_BLUETOOTH_CUSTOM_HCITOOL), true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bluez/lib/hci.c \
	bluez/lib/bluetooth.c \
	bluez/tools/csr.c \
	bluez/tools/csr_h4.c \
	bluez/tools/hciconfig.c

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_C_INCLUDES:=\
	$(LOCAL_PATH)/bluez \
	$(LOCAL_PATH)/bluez/tools \
	$(LOCAL_PATH)/bluez/lib \
	$(LOCAL_PATH)/bluez/src \
	$(LOCAL_PATH)/bluez/src/shared \

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=hciconfig

include $(BUILD_EXECUTABLE)


endif

ifeq ($(BOARD_HAVE_BLUETOOTH_CSR),true)

#
# bccmd
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bluez/lib/hci.c \
	bluez/lib/bluetooth.c \
	bluez/tools/bccmd.c \
	bluez/tools/csr.c \
	bluez/tools/csr_3wire.c \
	bluez/tools/csr_bcsp.c \
	bluez/tools/csr_hci.c \
	bluez/tools/csr_h4.c \
	bluez/tools/csr_usb.c \
	bluez/tools/ubcsp.c

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_C_INCLUDES:=\
	$(LOCAL_PATH)/bluez \
	$(LOCAL_PATH)/bluez/lib \
	$(LOCAL_PATH)/bluez/tools \
	$(LOCAL_PATH)/bluez/src \
	$(LOCAL_PATH)/bluez/src/shared \

LOCAL_MODULE:=bccmd

include $(BUILD_EXECUTABLE)

#
# sdptool
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bluez/lib/hci.c \
	bluez/lib/bluetooth.c \
	bluez/lib/sdp.c \
	bluez/tools/sdptool.c \
	bluez/src/sdp-xml.c

LOCAL_CFLAGS := $(BLUEZ_COMMON_CFLAGS)

LOCAL_C_INCLUDES:=\
	$(LOCAL_PATH)/bluez \
	$(LOCAL_PATH)/bluez/lib \
	$(LOCAL_PATH)/bluez/tools \
	$(LOCAL_PATH)/bluez/src \
	$(LOCAL_PATH)/bluez/src/shared \

LOCAL_C_INCLUDES += \
	$(call include-path-for, glib) \
	$(call include-path-for, glib)/glib \

LOCAL_SHARED_LIBRARIES := \
	libglib \

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=sdptool

include $(BUILD_EXECUTABLE)
endif

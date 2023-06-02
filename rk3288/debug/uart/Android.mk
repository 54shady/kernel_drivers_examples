LOCAL_PATH:= $(call my-dir)

# uart_loop
include $(CLEAR_VARS)
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_SRC_FILES := uart_loop_test.c
LOCAL_MODULE := uart_loop

include $(BUILD_EXECUTABLE)

# record read write
include $(CLEAR_VARS)
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := debug
LOCAL_SRC_FILES := uart_record_rw.c
LOCAL_MODULE := uart_record_rw

include $(BUILD_EXECUTABLE)
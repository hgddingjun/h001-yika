



LOCAL_PATH :=$(call my-dir)
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp

LOCAL_LDLIBS    := -llog
LOCAL_CFLAGS    := -Werror
LOCAL_MODULE  :=letcar
LOCAL_SRC_FILES:= \
bt_relayer.cpp \
bluetooth_bttest.cpp \
bt_em.c

include $(BUILD_SHARED_LIBRARY)

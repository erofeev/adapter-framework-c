##
#
#
#
##

APP_NAME            := simple_console_app
APP_CONFIG          := configs/project_config.h

COMMON_COMPONENTS   := adp_framework/adp_dispatcher
COMMON_COMPONENTS   += adp_framework/adp_console
COMMON_COMPONENTS   += adp_framework/adp_logging
COMMON_COMPONENTS   += adp_framework/adp_osal
COMMON_COMPONENTS   += adp_framework/adp_uart
COMMON_COMPONENTS   += FreeRTOS-Kernel

PLATFORM_TARGET     := linux
PLATFORM_COMPONENTS := adp_framework
PLATFORM_COMPONENTS += FreeRTOS-Kernel
PLATFORM_COMPONENTS += log_c

LDLIBS?=-lopenal
ifeq ($(LIBAUDIO),1)
CFLAGS+=-DLIBAUDIO
LDLIBS+=-laudio
else
LDLIBS+=-lalut
endif
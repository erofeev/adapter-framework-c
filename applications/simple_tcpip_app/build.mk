##
#
#
#
##

APP_NAME            := simple_tcpip_app
APP_CONFIG          := configs/project_config.h
PLATFORM_TARGET     := linux
PLATFORM_COMPONENTS := adp_framework
PLATFORM_COMPONENTS += FreeRTOS-Kernel
PLATFORM_COMPONENTS += FreeRTOS-Plus-TCP
PLATFORM_COMPONENTS += log_c

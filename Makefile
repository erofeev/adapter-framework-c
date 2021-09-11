##
#
#  Makefile
#
##
include applications/simple_mqtt_app/build.mk
#include applications/simple_tcpip_app/build.mk
#include applications/simple_console_app/build.mk


PRINT_COMPONENTS := $(addprefix \\n  -- [common] ,$(COMMON_COMPONENTS))
PRINT_COMPONENTS += $(addprefix \\n  -- [$(PLATFORM_TARGET)]  ,$(PLATFORM_COMPONENTS))

# Build system variables, rules and targets
CCACHE := 
CC     := gcc

# Final executable and config
APP                := bin/$(APP_NAME)
APP_INCLUDE_CONFIG := applications/$(APP_NAME)/$(APP_CONFIG)

# Directory naming conventions
SRC_DIR_NAME := applications/$(APP_NAME)
SRC_DIR_NAME += $(addprefix platforms/common/,$(COMMON_COMPONENTS))
SRC_DIR_NAME += $(addprefix platforms/$(PLATFORM_TARGET)/,$(PLATFORM_COMPONENTS))

# Default output dir
BUILD_DIR_NAME := build/$(APP_NAME)

# source files list
SRCS := $(shell find ./$(SRC_DIR_NAME) -name '*.c')
HDRS := $(shell find ./$(SRC_DIR_NAME) -name '*.h')
DEPS := $(shell find ./$(SRC_DIR_NAME) -type d )


# FLAGS
CFLAGS  := -Wall -Wshadow -Wundef -Winit-self -Wpointer-arith
CFLAGS  += -Wextra -Wno-unused-parameter -Wundef -Warray-bounds -Wno-missing-braces -Wmissing-field-initializers
# for GCC (not CLANG) we can also use -Wunused-but-set-variable   -Wunused-but-set-parameter
CFLAGS  += -fdata-sections -ffunction-sections -O2
LDFLAGS := -pthread -lpcap -Wl,-Map=$(BUILD_DIR_NAME)/$(APP_NAME).map,--cref -Wl,--gc-sections


# function to get obj file name from src file name
#OBJ_FROM_SRC = $(subst /$(SRC_DIR_NAME)/,/$(BUILD_DIR_NAME)/,$(1:%.c=%.o))
OBJ_FROM_SRC = $(addprefix $(BUILD_DIR_NAME)/,$(1:%.c=%.o))


.PHONY: build announce
.DEFAULT_GOAL := build

# rule template for obj
define obj_rule_by_src =
src := $(1)
obj := $$(call OBJ_FROM_SRC,$$(src))
OBJS := $$(OBJS) $$(obj)
$$(obj): $$(src) $(HDRS) $(APP_INCLUDE_CONFIG) 
# compile source code into objects
	@echo -n     + $$<
	@mkdir -p  $(dir $(OBJ_FROM_SRC))
	@$(CCACHE) $(CC) -include $(APP_INCLUDE_CONFIG) $(CFLAGS) -c $$< -o $$@ $(addprefix -I,$(DEPS))
	@echo 
endef

# generate rules for OBJS
$(foreach src,$(SRCS),$(eval $(call obj_rule_by_src,$(src))))

$(APP): $(OBJS)
# link objects
	@echo "\n--------- Linking ----------"
	@echo "Executable [$@]"
	@$(CC) $^ -o $@ $(LDFLAGS)


announce:
	@echo 
	@mkdir -p bin
	@mkdir -p $(BUILD_DIR_NAME)
	@echo "Application [$(APP_NAME)]\n\r"
	@echo "Project config file [$(APP_INCLUDE_CONFIG)] \n\r"
	@cp -rf $(APP_INCLUDE_CONFIG) $(BUILD_DIR_NAME)/
	@echo -n "List of included components: "
	@echo "${PRINT_COMPONENTS}"
	@echo ""
	@echo "Building..."

build: announce | $(OBJS) $(APP)
	@echo "ADP Info:"
	@size $(OBJS) | grep 'applications\|hex	filename'
	@echo "ADP Info:"
	@size $(OBJS) | grep 'adp\|hex	filename'
	@echo "Other Info:"
	@size $(OBJS) | grep -v 'applications\|adp\|hex	filename'
	@echo "Total Info:"
	@size $(APP)
	@echo "Done."
	@echo 
	@echo 

all: build

clean:
	@echo
	@echo -n Cleaning... 
	@rm -rf $(BUILD_DIR_NAME)
	@rm -rf $(APP)
	@echo "Done."
	@echo 

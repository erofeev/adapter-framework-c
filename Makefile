##
#
#
#
##

APP_NAME            := simple_console_app
APP_CONFIG          := app_config.h
PLATFORM_TARGET     := linux
PLATFORM_COMPONENTS := FreeRTOS-Kernel
PLATFORM_COMPONENTS += log_c


# Build system variables, rules and targets
CCACHE := ccache
CC     := gcc

# Final executable and config
APP                := bin/$(APP_NAME)
APP_INCLUDE_CONFIG := applications/$(APP_NAME)/$(APP_CONFIG)

# Directory naming conventions
SRC_DIR_NAME := applications/$(APP_NAME)
SRC_DIR_NAME += platforms/abstract
SRC_DIR_NAME += platforms/$(PLATFORM_TARGET) 

# Default output dir
BUILD_DIR_NAME := build

# source files list
SRCS := $(shell find ./$(SRC_DIR_NAME) -name '*.c')
DEPS := $(shell find ./$(SRC_DIR_NAME) -type d )


# FLAGS
CFLAGS  := -Wall -Wshadow -Wundef -Winit-self -Wpointer-arith
CFLAGS  += -fdata-sections -ffunction-sections -Os
LDFLAGS := -pthread -Wl,-Map=$(BUILD_DIR_NAME)/$(APP_NAME).map,--cref -Wl,--gc-sections


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
$$(obj): $$(src)
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
	@echo --------- Linking ----------
	@echo -n  "Executable [$@] - "
	@gcc $(LDFLAGS) $^ -o $@ 


announce:
	@echo 
	@mkdir -p bin
	@mkdir -p $(BUILD_DIR_NAME)
	@echo Project config file [$(APP_INCLUDE_CONFIG)]
	@cp -rf $(APP_INCLUDE_CONFIG) $(BUILD_DIR_NAME)/
	@echo Compilation started

build: announce | $(APP) 
	@echo "Done."
	@echo 

all: build

clean:
	@echo
	@echo -n Cleaning... 
	@rm -rf $(BUILD_DIR_NAME)/*
	@rm -rf $(APP)
	@echo "Done."
	@echo 



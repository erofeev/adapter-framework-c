# Directory naming conventions
SRC_DIR_NAME := src

# output dir
BUILD_DIR_NAME := build

# source files list
SRCS := $(shell find ./src -name '*.c')
DEPS := $(shell find ./src -type d )


# FLAGS
CLFAGS  := -Wall
LDFLAGS := -pthread


# function to get obj file name from src file name
OBJ_FROM_SRC = $(subst /$(SRC_DIR_NAME)/,/$(BUILD_DIR_NAME)/,$(1:%.c=%.o))

# Final executable
APP := bin/app

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
	@gcc $(CLFAGS) -c $$< -o $$@ $(addprefix -I,$(DEPS))
	@echo 
endef

# generate rules for OBJS
$(foreach src,$(SRCS),$(eval $(call obj_rule_by_src,$(src))))

$(APP): $(OBJS)
# link objects
	@echo --------- Linking ----------
	@echo -n  Build $@  -
	@gcc $(LDFLAGS) $^ -o $@ 


announce:
	@echo 
	@mkdir -p build/adapters/adp_dispatcher
	@mkdir -p build/adapters/adp_osal
	@mkdir -p build/adapters/adp_mqtt_client
	@mkdir -p build/adapters/adp_logging
	@mkdir -p build/libs/log_c/build
	@mkdir -p build/libs/FreeRTOS-Kernel/portable/MemMang
	@mkdir -p build/libs/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/utils
	@echo Compilation started

build: announce | $(APP) 
	@echo Done.
	@echo 

all: build

clean:
	@echo
	@echo -n Cleaning... 
	@rm -rf $(BUILD_DIR_NAME)/*
	@rm -rf $(APP)
	@echo Done
	@echo 



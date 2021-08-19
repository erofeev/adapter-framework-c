
APP_NAME := simple_example_app



# Final executable
APP := bin/$(APP_NAME)

# Directory naming conventions
SRC_DIR_NAME := src

# output dir
BUILD_DIR_NAME := build

# source files list
SRCS := $(shell find ./$(SRC_DIR_NAME) -name '*.c')
DEPS := $(shell find ./$(SRC_DIR_NAME) -type d )


# FLAGS
CLFAGS  := -Wall
LDFLAGS := -pthread -Wl,-Map=$(BUILD_DIR_NAME)/program.map,--cref


# function to get obj file name from src file name
OBJ_FROM_SRC = $(subst /$(SRC_DIR_NAME)/,/$(BUILD_DIR_NAME)/,$(1:%.c=%.o))


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



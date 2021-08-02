# Directory naming conventions
SRC_DIR_NAME := src
BUILD_DIR_NAME := build

# source files list
SRCS := $(shell find ./src -name '*.c')

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
	@gcc -c $$< -o $$@
	@echo 
endef

# generate rules for OBJS
$(foreach src,$(SRCS),$(eval $(call obj_rule_by_src,$(src))))


$(APP): $(OBJS)
# link objects
	@echo --------- Linking ----------
	@echo -n  Build $@  -
	@gcc $^ -o $@ 

announce:
	@echo 
	@echo Compilation started

build: announce | $(APP)
	@echo Done.
	@echo 

clean:
	@echo
	@echo -n Cleaning... 
	@rm -rf $(BUILD_DIR_NAME)/*
	@rm -rf $(APP)
	@echo Done
	@echo 



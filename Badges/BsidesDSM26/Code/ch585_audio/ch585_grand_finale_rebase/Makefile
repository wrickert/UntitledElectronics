all : flash

TARGET:=ch585_grand_finale
TARGET_MCU:=CH585
TARGET_MCU_PACKAGE:=CH585M

# ch32fun.mk does not track compiler-flag changes. Add a stamp file that
# changes whenever build flags change, and make the ELF depend on it.
BUILD_FLAGS_FILE := .buildflags
EXTRA_ELF_DEPENDENCIES += $(BUILD_FLAGS_FILE)

.PHONY: FORCE
FORCE:

$(BUILD_FLAGS_FILE): FORCE
	@set -e; \
	tmp="$$(mktemp)"; \
	printf '%s\n' \
		"EXTRA_CFLAGS=$(EXTRA_CFLAGS)" \
		"TARGET_MCU_PACKAGE=$(TARGET_MCU_PACKAGE)" \
		> "$$tmp"; \
	if [ -f "$@" ] && cmp -s "$@" "$$tmp"; then \
		rm -f "$$tmp"; \
	else \
		mv "$$tmp" "$@"; \
	fi

CH32FUN_MK ?= ../ch32fun/ch32fun/ch32fun.mk
ifeq ($(wildcard $(CH32FUN_MK)),)
CH32FUN_MK := ../../ch32fun/ch32fun/ch32fun.mk
endif

include $(CH32FUN_MK)

bootloader :
	@# Best-effort: if the device node isn't accessible, flashing via ROM bootloader still works.
	-./$(TARGET).py -b

flash : bootloader cv_flash
clean : cv_clean

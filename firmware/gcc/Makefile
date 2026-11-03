ROOT := ../..
OUT := $(ROOT)/build/firmware
CROSS_COMPILE ?= arm-none-eabi-
ifeq ($(strip $(CROSS_COMPILE)),)
CROSS_COMPILE := arm-none-eabi-
endif
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
NM := $(CROSS_COMPILE)nm
SPL := $(ROOT)/firmware/vendor/lib/src
KERNEL := $(ROOT)/firmware/vendor/freertos
SOURCES := $(wildcard $(ROOT)/core/*.c $(ROOT)/services/*.c $(ROOT)/firmware/board/*.c)
SOURCES += $(ROOT)/firmware/app/main.c $(ROOT)/firmware/app/faults.c
SOURCES += $(ROOT)/firmware/vendor/system_stm32f10x.c $(ROOT)/firmware/vendor/driver_core/core_cm3.c
SOURCES += $(addprefix $(SPL)/,misc.c stm32f10x_rcc.c stm32f10x_gpio.c stm32f10x_tim.c stm32f10x_adc.c stm32f10x_dma.c stm32f10x_usart.c stm32f10x_flash.c stm32f10x_iwdg.c stm32f10x_exti.c)
SOURCES += $(KERNEL)/tasks.c $(KERNEL)/queue.c $(KERNEL)/list.c port.c
OBJECTS := $(patsubst %.c,$(OUT)/%.o,$(notdir $(SOURCES))) $(OUT)/startup.o
VPATH := $(sort $(dir $(SOURCES)))
CPPFLAGS := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER
CPPFLAGS += $(addprefix -I,$(ROOT)/firmware/app . $(ROOT)/firmware/board $(ROOT)/core $(ROOT)/services $(ROOT)/firmware/vendor $(ROOT)/firmware/vendor/driver_core $(ROOT)/firmware/vendor/lib/inc $(KERNEL)/include)
CFLAGS := -mcpu=cortex-m3 -mthumb -std=c99 -Os -g3 -ffunction-sections -fdata-sections -fno-common -Wall -Wextra -Werror -MMD -MP
LDFLAGS := -mcpu=cortex-m3 -mthumb -nostartfiles --specs=nano.specs --specs=nosys.specs -Tlinker.ld -Wl,--gc-sections,-Map=$(OUT)/kitchen.map,--cref
.PHONY: all check
all: $(OUT)/kitchen.hex $(OUT)/kitchen.bin check
$(OUT):
	mkdir -p $@
$(OUT)/%.o: %.c | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OUT)/startup.o: startup.S | $(OUT)
	$(CC) -mcpu=cortex-m3 -mthumb -g3 -c $< -o $@
$(OUT)/kitchen.elf: $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
$(OUT)/kitchen.hex: $(OUT)/kitchen.elf
	$(OBJCOPY) -O ihex $< $@
$(OUT)/kitchen.bin: $(OUT)/kitchen.elf
	$(OBJCOPY) -O binary $< $@
check: $(OUT)/kitchen.elf $(OUT)/kitchen.hex $(OUT)/kitchen.bin
	$(SIZE) $<
	python3 $(ROOT)/tools/check_elf.py --prefix '$(CROSS_COMPILE)' $<
-include $(OBJECTS:.o=.d)

$(OBJECTS): Makefile $(ROOT)/firmware/app/FreeRTOSConfig.h

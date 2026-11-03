ENTRY(Reset_Handler)
MEMORY {
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 62K
  RETAIN (rw) : ORIGIN = 0x20000000, LENGTH = 256
  RAM (xrw) : ORIGIN = 0x20000100, LENGTH = 20224
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS {
  .retained (NOLOAD) : { KEEP(*(RETENTION)) } > RETAIN
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text : { *(.text*) *(.rodata*) KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4); } > FLASH
  .ARM.extab : { *(.ARM.extab*) } > FLASH
  .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH
  _sidata = LOADADDR(.data);
  .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
  .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
  .stack_reserve (NOLOAD) : { . = ALIGN(8); _stack_reserved = .; . += 1024; } > RAM
  /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}
ASSERT(_ebss <= _estack - 1024, "RAM overflow: preserve 1KiB interrupt stack")
ASSERT(LOADADDR(.data) + SIZEOF(.data) <= 0x0800F800, "Firmware overlaps configuration journal")

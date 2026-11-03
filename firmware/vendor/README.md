# Bundled dependencies

Copied from the user's original Kitchen_Project on 2026-11-02:

- STM32F10x Standard Peripheral Library (source headers identify V3.5.0).
- CMSIS Cortex-M3 core and STM32 startup/system sources.
- FreeRTOS V9.0.0, including original portable directories and license notices.

Original notices are retained. `freertos/include/FreeRTOSConfig.h` was renamed
`FreeRTOSConfig.original.h` so the active application configuration is selected
from `firmware/app/FreeRTOSConfig.h`. Only files explicitly listed in the Keil
project are compiled. Unused CPU ports are retained as upstream baseline files,
not active project features. No original resumes or interview documents are bundled.

GNU compatibility patch: core_cm3.c GNU STREXB/STREXH/STREXW outputs use early-clobber and memory constraints to prevent illegal input/output register overlap with GCC12. The ARMCC branches are unchanged. The active application now uses static task/queue allocation and no heap implementation.

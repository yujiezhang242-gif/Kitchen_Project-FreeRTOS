# GNU build support

`port.c` and `portmacro.h` adapt the bundled FreeRTOS V9 RVDS/ARM_CM3 port
from ARMCC assembly syntax/intrinsics to GNU inline assembly. Original license
notices and priority checks are retained. They are project-maintained files,
not a downloaded upstream GCC port. Keil still uses the unchanged RVDS port.

Tickless idle is disabled. Context switching requires board verification;
compiling and disassembling code alone does not prove it executes correctly.

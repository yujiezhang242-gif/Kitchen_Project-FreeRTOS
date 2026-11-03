#!/usr/bin/env python3
"""ARM C syntax/type check using clang. Does not assemble/link the ARMCC port.
Temporary libc declarations and port definitions permit type checking on macOS.
The shipping Keil project uses the real libc and original RVDS port unchanged.
"""
import pathlib, subprocess, tempfile, shutil
ROOT=pathlib.Path(__file__).resolve().parents[1]
PORT='''#ifndef PORTMACRO_H
#define PORTMACRO_H
#include <stdint.h>
typedef uint32_t StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY UINT32_MAX
#define portSTACK_GROWTH (-1)
#define portBYTE_ALIGNMENT 8
#define portTICK_PERIOD_MS 1
#define portCRITICAL_NESTING_IN_TCB 0
#define portTICK_TYPE_IS_ATOMIC 1
#define portDONT_DISCARD
#define portPRIVILEGE_BIT 0
#define portNOP() ((void)0)
#define portINLINE inline
void vPortEnterCritical(void);
void vPortExitCritical(void);
#define portENTER_CRITICAL() vPortEnterCritical()
#define portEXIT_CRITICAL() vPortExitCritical()
#define portDISABLE_INTERRUPTS() vPortEnterCritical()
#define portENABLE_INTERRUPTS() vPortExitCritical()
#define portYIELD() ((void)0)
#define portYIELD_FROM_ISR(x) ((void)(x))
#define portTASK_FUNCTION_PROTO(fn,arg) void fn(void *arg)
#define portTASK_FUNCTION(fn,arg) void fn(void *arg)
#endif
'''
def main():
    cc=shutil.which('clang')
    if not cc: raise SystemExit('clang required for optional ARM syntax check')
    with tempfile.TemporaryDirectory(prefix='kitchen-syntax-') as tmp:
        p=pathlib.Path(tmp)
        (p/'portmacro.h').write_text(PORT)
        (p/'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void *,const void *,size_t);\nvoid *memmove(void *,const void *,size_t);\nvoid *memset(void *,int,size_t);\nint memcmp(const void *,const void *,size_t);\nsize_t strlen(const char *);\n')
        (p/'stdio.h').write_text('#include <stddef.h>\ntypedef struct FILE FILE;\nint snprintf(char *,size_t,const char *,...);\n')
        (p/'stdlib.h').write_text('#include <stddef.h>\nvoid *malloc(size_t);\nvoid free(void *);\n')
        includes=[p,ROOT/'firmware/app',ROOT/'firmware/board',ROOT/'core',ROOT/'services',ROOT/'firmware/vendor',ROOT/'firmware/vendor/driver_core',ROOT/'firmware/vendor/lib/inc',ROOT/'firmware/vendor/freertos/include']
        files=list((ROOT/'core').glob('*.c'))+list((ROOT/'services').glob('*.c'))+list((ROOT/'firmware/board').glob('*.c'))+list((ROOT/'firmware/app').glob('*.c'))
        cmd=[cc,'--target=arm-none-eabi','-mcpu=cortex-m3','-mthumb','-std=c99','-ffreestanding','-fsyntax-only','-Wall','-Wextra','-Werror','-DSTM32F10X_MD','-DUSE_STDPERIPH_DRIVER']
        for inc in includes: cmd+=['-I',str(inc)]
        subprocess.run(cmd+[str(f) for f in files],check=True)
        print(f'PASS: ARM C syntax and types ({len(files)} units); ARMCC port/link/hardware unverified')
if __name__=='__main__': main()

#!/usr/bin/env python3
"""Verify linked target address ranges, vector table, static allocation and symbols."""
import argparse,json,pathlib,struct,subprocess,hashlib
from source_fingerprint import fingerprint

def main():
    parser=argparse.ArgumentParser(); parser.add_argument('--prefix',default='arm-none-eabi-'); parser.add_argument('elf'); args=parser.parse_args()
    path=pathlib.Path(args.elf); data=path.read_bytes()
    assert data[:7]==b'\x7fELF\x01\x01\x01','expected ELF32 little endian'
    header=struct.unpack_from('<16sHHIIIIIHHHHHH',data)
    assert header[2]==40,'expected ARM machine'
    entry,phoff,phentsize,phnum=header[4],header[5],header[9],header[10]
    flash_end=0x08000000; ram_end=0x20000000; vector=None
    for i in range(phnum):
        ptype,offset,vaddr,paddr,filesz,memsz,flags,align=struct.unpack_from('<8I',data,phoff+i*phentsize)
        if ptype!=1: continue
        if filesz and 0x08000000<=paddr<0x08010000:
            assert paddr+filesz<=0x0800f800,'load segment overwrites config journal'
            flash_end=max(flash_end,paddr+filesz)
        if 0x20000000<=vaddr<0x20005000:
            assert vaddr+memsz<=0x20005000,'RAM load segment exceeds 20KiB'
            ram_end=max(ram_end,vaddr+memsz)
        if vaddr==0x08000000: vector=data[offset:offset+filesz]
    assert vector and len(vector)>=236,'missing STM32F103 medium-density vectors'
    stack,reset=struct.unpack_from('<II',vector)
    assert stack==0x20005000 and reset&1 and entry==reset,'invalid startup stack/reset vector'
    symbols={}
    for line in subprocess.check_output([args.prefix+'nm','--defined-only',str(path)],text=True).splitlines():
        parts=line.split()
        if len(parts)==3: symbols[parts[2]]=int(parts[0],16)
    for name in ['SVC_Handler','PendSV_Handler','SysTick_Handler','DMA1_Channel5_IRQHandler','USART1_IRQHandler','EXTI9_5_IRQHandler','xTaskCreateStatic','xQueueGenericCreateStatic']:
        assert name in symbols,'missing '+name
    for index,name in {3:'HardFault_Handler',11:'SVC_Handler',14:'PendSV_Handler',15:'SysTick_Handler',31:'DMA1_Channel5_IRQHandler',39:'EXTI9_5_IRQHandler',53:'USART1_IRQHandler'}.items():
        assert struct.unpack_from('<I',vector,index*4)[0]==(symbols[name]|1),'vector does not reference '+name
        assert symbols[name]!=symbols.get('Default_Handler'),'required IRQ aliases Default_Handler: '+name
    for name in ['pvPortMalloc','vPortFree','malloc','calloc','realloc','free','_sbrk','printf','snprintf']:
        assert name not in symbols,'unexpected dynamic allocation/stdio symbol: '+name
    assert symbols['_ebss']<=stack-1024,'less than 1KiB interrupt stack headroom'
    report=dict(elf=str(path),flash_load_bytes=flash_end-0x08000000,flash_limit_bytes=62*1024,
                static_ram_bytes=symbols['_ebss']-0x20000000,interrupt_stack_reserved=1024,
                remaining_ram_after_stack=stack-symbols['_ebss']-1024,static_allocation=True,
                reset_vector=hex(reset),entry=hex(entry),hardware_validated=False)
    report['source_build_fingerprint']=fingerprint()
    report['artifact_sha256']={path.with_suffix(ext).name:hashlib.sha256(path.with_suffix(ext).read_bytes()).hexdigest() for ext in ('.elf','.hex','.bin','.map')}
    path.with_suffix('.resources.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__': main()

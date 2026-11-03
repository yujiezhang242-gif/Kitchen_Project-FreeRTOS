#!/usr/bin/env python3
"""Regenerate the MDK5/ARMCC5 project from the bundled baseline template."""
from pathlib import Path
import xml.etree.ElementTree as E
ROOT=Path(__file__).resolve().parents[1]
def main():
    root=E.parse(ROOT/'firmware/project.template.uvprojx')
    target=root.find('./Targets/Target')
    target.find('TargetName').text='Kitchen_RTOS_Pro'
    common=target.find('TargetOption/TargetCommonOption')
    common.find('OutputDirectory').text='.\\build\\'
    common.find('OutputName').text='kitchen_rtos'
    common.find('ListingPath').text='.\\build\\'
    common.find('CreateHexFile').text='1'
    common.find('Cpu').text='IRAM(0x20000000,0x5000) IROM(0x08000000,0xF800) CPUTYPE("Cortex-M3") CLOCK(8000000) ELITTLE'
    status=common.find('TargetStatus')
    for tag in ['Error','ExitCodeStop','NotGenerated','InvalidFlash']:
        status.find(tag).text='0'
    # Flash driver must target 64K, not use a 128K capacity assumption.
    common.find('FlashDriverDll').text='UL2CM3(-S0 -C0 -P0 -FD20000000 -FC1000 -FN1 -FF0STM32F10x_128 -FS08000000 -FL010000 -FP0($$Device:STM32F103C8$Flash\\STM32F10x_128.FLM))'
    common.find('PackID').text='Keil.STM32F1xx_DFP.1.0.5'
    c=target.find('TargetOption/TargetArmAds')
    c.find('ArmAdsMisc/useUlib').text='1'
    c.find('ArmAdsMisc/OnChipMemories/OCR_RVCT9/Size').text='0x5000'
    c.find('ArmAdsMisc/OnChipMemories/OCR_RVCT4/Size').text='0xF800'
    c.find('ArmAdsMisc/OnChipMemories/IROM/Size').text='0xF800'
    c.find('LDads/umfTarg').text='0'
    c.find('LDads/useFile').text='1'
    c.find('LDads/ScatterFile').text='.\\kitchen.sct'
    c.find('Cads/Optim').text='2'
    includes=['app','board','..\\core','..\\services','vendor','vendor\\driver_core','vendor\\lib\\inc','vendor\\freertos\\include','vendor\\freertos\\portable\\RVDS\\ARM_CM3']
    c.find('Cads/VariousControls/IncludePath').text=';'.join(includes)
    groups=target.find('Groups'); groups.clear()
    sources={
      'App':['app/main.c','app/faults.c','app/retarget.c'],
      'Core':['../core/kitchen.c'],
      'Services':['../services/wire.c','../services/config_store.c','../services/runtime.c','../services/health.c','../services/dht_decoder.c','../services/timing.c','../services/text.c','../services/crash.c'],
      'Board':['board/board.c','board/oled.c'],
      'CMSIS':['vendor/system_stm32f10x.c','vendor/driver_core/core_cm3.c','vendor/driver_core/startup_stm32f10x_md.s'],
      'STM32 SPL':['vendor/lib/src/'+n+'.c' for n in ['misc','stm32f10x_rcc','stm32f10x_exti','stm32f10x_gpio','stm32f10x_tim','stm32f10x_adc','stm32f10x_dma','stm32f10x_usart','stm32f10x_flash','stm32f10x_iwdg']],
      'FreeRTOS':['vendor/freertos/'+n+'.c' for n in ['tasks','queue','list']]+['vendor/freertos/portable/RVDS/ARM_CM3/port.c']}
    for name,paths in sources.items():
        g=E.SubElement(groups,'Group'); E.SubElement(g,'GroupName').text=name; fs=E.SubElement(g,'Files')
        for path in paths:
            assert (ROOT/'firmware'/path).is_file(),path
            f=E.SubElement(fs,'File'); E.SubElement(f,'FileName').text=Path(path).name
            E.SubElement(f,'FileType').text='2' if path.endswith('.s') else '1'
            E.SubElement(f,'FilePath').text=path.replace('/','\\')
    E.indent(root,space='  ')
    root.write(ROOT/'firmware/Kitchen_RTOS_Pro.uvprojx',encoding='utf-8',xml_declaration=True)
    print('Generated firmware/Kitchen_RTOS_Pro.uvprojx; 62KB app + 2 independent config pages')
if __name__=='__main__': main()

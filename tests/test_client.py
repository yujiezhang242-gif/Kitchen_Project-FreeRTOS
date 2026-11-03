import importlib.util
import pathlib
import struct
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('kitchen_cli',ROOT/'tools/kitchen_cli.py')
cli=importlib.util.module_from_spec(spec); spec.loader.exec_module(cli)

class CodecTests(unittest.TestCase):
    def test_crc_and_fragmentation(self):
        self.assertEqual(cli.crc(b'123456789'),0x4b37)
        d=cli.Decoder(); packet=cli.encode(3,99,b'hello'); result=[]
        for b in packet: result+=d.feed(bytes([b]))
        self.assertEqual(result,[(3,99,b'hello')])
    def test_bad_frame_and_noise(self):
        d=cli.Decoder(); bad=bytearray(cli.encode(1,1)); bad[-1]^=1
        self.assertEqual(d.feed(b'noise'+bad+cli.encode(2,3,b'abc')),[(2,3,b'abc')])

class IntegrationTests(unittest.TestCase):
    def test_commands_and_flash_restart(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=pathlib.Path(tmp)/'flash.bin'; c=cli.Client(sim=True,state=path)
            try:
                self.assertEqual(c.status()['temperature_c'],25)
                cfg=c.config(); cfg['temp_on']=350
                c.request(3,cli.CONFIG.pack(*(cfg[x] for x in cli.FIELDS)))
                self.assertTrue(c.status()['unsaved'])
                c.request(7); self.assertFalse(c.status()['unsaved'])
                c.request(4,struct.pack('<BBI',1,1,1000))
                self.assertEqual(c.status()['mode'],'MANUAL'); self.assertTrue(c.status()['lamp'])
                c.request(4,b'\0'*6); self.assertEqual(c.status()['mode'],'AUTO')
                with self.assertRaisesRegex(RuntimeError,'bad length'): c.request(5,b'bad')
                with self.assertRaisesRegex(RuntimeError,'unsupported'): c.request(0x70)
            finally: c.close()
            reboot=cli.Client(sim=True,state=path)
            try: self.assertEqual(reboot.config()['temp_on'],350)
            finally: reboot.close()
    def test_unsaved_changes_lost_after_restart(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=pathlib.Path(tmp)/'flash.bin'; c=cli.Client(sim=True,state=path)
            try:
                cfg=c.config(); cfg['temp_on']=390
                c.request(3,cli.CONFIG.pack(*(cfg[x] for x in cli.FIELDS)))
            finally: c.close()
            c=cli.Client(sim=True,state=path)
            try: self.assertEqual(c.config()['temp_on'],300)
            finally: c.close()

class ProjectTests(unittest.TestCase):
    def test_keil_files_and_reserved_flash(self):
        import xml.etree.ElementTree as E
        root=E.parse(ROOT/'firmware/Kitchen_RTOS_Pro.uvprojx')
        for node in root.iter('FilePath'):
            self.assertTrue((ROOT/'firmware'/node.text.replace('\\','/')).is_file(),node.text)
        rom=root.find('.//OCR_RVCT4')
        self.assertEqual(int(rom.findtext('StartAddress'),0)+int(rom.findtext('Size'),0),0x0800F800)
        self.assertFalse((ROOT/'firmware/vendor/freertos/include/FreeRTOSConfig.h').exists())
        sources=[n.text for n in root.iter('FilePath')]
        self.assertEqual(sum('startup_' in p for p in sources),1)
        self.assertEqual(sum('heap_' in p for p in sources),0)

if __name__=='__main__': unittest.main()

#!/usr/bin/env python3
"""Binary protocol client: serial hardware or native C business simulator."""
import argparse, csv, json, os, pathlib, selectors, struct, subprocess, sys, time
ROOT=pathlib.Path(__file__).resolve().parents[1]
CONFIG=struct.Struct('<hhHHHHII')
FIELDS=('temp_on','temp_off','humid_on','humid_off','dark_on','dark_off','stale_ms','mute_ms')
ERRORS={1:'bad length',2:'bad value',3:'busy; retry later',4:'unsupported',5:'sequence conflict',6:'storage I/O failure'}
def crc(data):
    value=0xffff
    for b in data:
        value^=b
        for _ in range(8): value=(value>>1)^(0xa001 if value&1 else 0)
    return value

def encode(command,seq,payload=b''):
    if len(payload)>128: raise ValueError('payload too long')
    body=struct.pack('<BBHH',1,command,seq,len(payload))+payload
    return b'\xaa\x55'+body+struct.pack('<H',crc(body))

class Decoder:
    def __init__(self): self.buffer=bytearray()
    def feed(self,data):
        self.buffer.extend(data); frames=[]
        while True:
            head=self.buffer.find(b'\xaa\x55')
            if head<0:
                self.buffer=self.buffer[-1:] if self.buffer[-1:]==b'\xaa' else bytearray()
                return frames
            if head: del self.buffer[:head]
            if len(self.buffer)<8: return frames
            version,cmd,seq,length=struct.unpack_from('<BBHH',self.buffer,2)
            if version!=1 or length>128: del self.buffer[0]; continue
            n=length+10
            if len(self.buffer)<n: return frames
            if crc(self.buffer[2:n-2])!=struct.unpack_from('<H',self.buffer,n-2)[0]: del self.buffer[0]; continue
            frames.append((cmd,seq,bytes(self.buffer[8:n-2]))); del self.buffer[:n]

class Client:
    def __init__(self,port=None,sim=False,state=None):
        self.process=None; self.serial=None; self.seq=int(time.time()*1000)&65535; self.decoder=Decoder()
        if sim:
            cmd=[str(ROOT/'build/simulator')]
            if state: cmd.append(str(state))
            self.process=subprocess.Popen(cmd,stdin=subprocess.PIPE,stdout=subprocess.PIPE,bufsize=0)
            self.selector=selectors.DefaultSelector(); self.selector.register(self.process.stdout,selectors.EVENT_READ)
        else:
            try: import serial
            except ImportError: raise RuntimeError('For hardware install: python3 -m pip install pyserial')
            self.serial=serial.Serial(port,115200,timeout=0.1,write_timeout=1)
    def close(self):
        if self.serial: self.serial.close()
        if self.process:
            self.process.stdin.close()
            try: self.process.wait(timeout=2)
            except subprocess.TimeoutExpired: self.process.terminate(); self.process.wait(timeout=2)
            self.selector.close(); self.process.stdout.close()
    def request(self,command,payload=b'',timeout=2):
        self.seq=(self.seq+1)&65535; seq=self.seq; packet=encode(command,seq,payload)
        if self.serial: self.serial.write(packet); self.serial.flush()
        else: self.process.stdin.write(packet); self.process.stdin.flush()
        until=time.monotonic()+timeout
        while time.monotonic()<until:
            if self.serial: data=self.serial.read(256)
            else:
                ready=self.selector.select(max(0,min(0.1,until-time.monotonic())))
                data=os.read(self.process.stdout.fileno(),512) if ready else b''
                if ready and not data: raise RuntimeError('simulator exited')
            for cmd,response_seq,body in self.decoder.feed(data):
                if cmd==(command|0x80) and response_seq==seq:
                    if not body: raise RuntimeError('empty response')
                    if body[0]: raise RuntimeError(ERRORS.get(body[0],f'error {body[0]}'))
                    return body[1:]
        raise TimeoutError('No matching response; check wiring, firmware and COM port')
    def config(self): return dict(zip(FIELDS,CONFIG.unpack(self.request(2))))
    def status(self):
        data=self.request(1)
        if len(data) not in (22,27): raise RuntimeError('status schema mismatch')
        now,temp,humid,light,flags,manual,dirty,age,revision=struct.unpack('<IhHHHBBII',data[:22])
        return dict(uptime_ms=now,temperature_c=temp/10,humidity_pct=humid/10,light_raw=light,
                    lamp=bool(flags&1),ventilation=bool(flags&2),buzzer=bool(flags&4),
                    hot=bool(flags&8),sensor_fault=bool(flags&16),climate_fault=bool(flags&32),light_fault=bool(flags&64),
                    light_age_ms=struct.unpack_from('<I',data,22)[0] if len(data)==27 else age,mode='MANUAL' if manual else 'AUTO',
                    unsaved=bool(dirty),sample_age_ms=age,config_revision=revision)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    connection=p.add_mutually_exclusive_group(required=True); connection.add_argument('--port'); connection.add_argument('--sim',action='store_true')
    p.add_argument('--state',help='simulated flash file (sim only)')
    sub=p.add_subparsers(dest='action',required=True)
    for cmd in ['status','config','ack','save','diag','timing','crash','clear-crash']: sub.add_parser(cmd)
    s=sub.add_parser('set'); s.add_argument('assignments',nargs='+',help='e.g. temp_on=300 temp_off=270; tenths of C / RH')
    m=sub.add_parser('manual'); m.add_argument('--lamp',type=int,choices=[0,1],default=0); m.add_argument('--vent',type=int,choices=[0,1],default=0); m.add_argument('--seconds',type=int,default=30)
    sub.add_parser('auto')
    logs=sub.add_parser('logs'); logs.add_argument('--count',type=int,default=16)
    mon=sub.add_parser('monitor'); mon.add_argument('--csv'); mon.add_argument('--interval',type=float,default=1); mon.add_argument('--count',type=int,default=0)
    args=p.parse_args(); client=None
    try:
        client=Client(args.port,args.sim,args.state); result=None
        if args.action=='status': result=client.status()
        elif args.action=='config': result=client.config()
        elif args.action=='set':
            values=client.config()
            for assignment in args.assignments:
                name,value=assignment.split('=',1)
                if name not in FIELDS: raise ValueError('unknown field: '+name)
                values[name]=int(value)
            client.request(3,CONFIG.pack(*(values[x] for x in FIELDS))); result={'applied':values,'persisted':False}
        elif args.action in ('manual','auto'):
            if args.action=='manual':
                if not 1<=args.seconds<=300: raise ValueError('manual seconds must be 1..300')
                payload=struct.pack('<BBI',args.lamp,args.vent,args.seconds*1000)
            else: payload=b'\0'*6
            client.request(4,payload); result=client.status()
        elif args.action in ('ack','save'):
            client.request(5 if args.action=='ack' else 7); result={'result':'OK'}
        elif args.action=='clear-crash':
            client.request(11); result={'cleared':True}
        elif args.action=='crash':
            data=client.request(10)
            if len(data)!=89: raise RuntimeError('crash schema mismatch')
            result={'valid':bool(data[0])}
            if data[0]:
                words=struct.unpack('<22I',data[1:])
                result.update(sequence=words[2],reason=hex(words[3]),uptime_ms=words[4],exception_return=hex(words[5]),
                    pc=hex(words[12]),lr=hex(words[11]),xpsr=hex(words[13]),cfsr=hex(words[14]),hfsr=hex(words[15]),
                    mmfar=hex(words[16]),bfar=hex(words[17]),msp=hex(words[18]),psp=hex(words[19]),task_mask=words[20])
        elif args.action=='timing':
            data=client.request(9)
            result=dict(zip(('cycles','max_interval_us','max_execution_us','late_intervals','overruns','build_abi'),struct.unpack('<6I',data)))
        elif args.action=='diag':
            data=client.request(8)
            if len(data)!=58: raise RuntimeError('diagnostic schema mismatch')
            names=('reset_flags','heap_free','heap_min','sensor_errors','rx_dropped','uart_errors','command_drops','reply_drops','tx_errors','display_errors','crc_errors','parser_timeouts')
            result=dict(zip(names,struct.unpack_from('<12I',data)))
            result['stack_free_words']=dict(zip(('sensor','control','ui','comm','storage'),struct.unpack_from('<5H',data,48)))
        elif args.action=='logs':
            result=[]
            for i in range(min(16,max(0,args.count))):
                try: total,at,event,value=struct.unpack('<BIHH',client.request(6,bytes([i])))
                except RuntimeError as error:
                    if str(error)=='bad value': break
                    raise
                result.append(dict(at_ms=at,event=event,value=value))
                if i+1>=total: break
        elif args.action=='monitor':
            if args.interval<0.1: raise ValueError('interval must be >=0.1 seconds')
            out=open(args.csv,'w',newline='',encoding='utf-8') if args.csv else None
            writer=None; count=0
            try:
                while not args.count or count<args.count:
                    state=client.status(); print(json.dumps(state,ensure_ascii=False),flush=True)
                    if out:
                        if writer is None: writer=csv.DictWriter(out,fieldnames=state.keys()); writer.writeheader()
                        writer.writerow(state); out.flush()
                    count+=1
                    if not args.count or count<args.count: time.sleep(args.interval)
            finally:
                if out: out.close()
        if result is not None: print(json.dumps(result,ensure_ascii=False,indent=2))
    except KeyboardInterrupt: pass
    except (OSError,ValueError,RuntimeError,TimeoutError,struct.error) as error:
        print(str(error),file=sys.stderr); return 1
    finally:
        if client: client.close()
    return 0
if __name__=='__main__': raise SystemExit(main())

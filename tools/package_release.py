#!/usr/bin/env python3
"""Package a checked GNU build with hashes tying binaries to the source tree.
Run make test && make firmware first. No upload or publishing occurs here.
"""
import hashlib,json,pathlib,shutil,subprocess
from source_fingerprint import fingerprint
ROOT=pathlib.Path(__file__).resolve().parents[1]

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    subprocess.run(['make','test'],cwd=ROOT,check=True)
    build=ROOT/'build/firmware'; out=ROOT/'release'; out.mkdir(exist_ok=True)
    report=json.loads((build/'kitchen.resources.json').read_text())
    assert report['static_allocation'] and not report['hardware_validated']
    assert report['source_build_fingerprint']==fingerprint(),'Source changed after checked build; run make firmware again'
    for name,expected in report['artifact_sha256'].items():
        assert digest(build/name)==expected,'Build artifact changed: '+name
    source={}
    for path in sorted(ROOT.rglob('*')):
        rel=path.relative_to(ROOT)
        if not path.is_file() or any(p in {'build','release','.git','__pycache__'} for p in rel.parts): continue
        if path.suffix in {'.pyc','.zip','.uvoptx'}: continue
        source[str(rel)]=digest(path)
    tree=hashlib.sha256(json.dumps(source,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    artifacts={}
    for name in ['kitchen.elf','kitchen.hex','kitchen.bin','kitchen.map','kitchen.resources.json']:
        shutil.copy2(build/name,out/name); artifacts[name]=digest(out/name)
    manifest=dict(build_abi=2,source_tree_sha256=tree,source_files=source,artifacts=artifacts,
                  target='STM32F103C8T6',hardware_validated=False,
                  checks='Host tests, ARM GNU full compile/link, ELF address/static-allocation validation')
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
    print('Packaged checked build in release/; source tree SHA256:',tree)
if __name__=='__main__': main()

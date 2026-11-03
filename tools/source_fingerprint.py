"""Source identity shared by ELF verification and release packaging."""
import hashlib,json,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
def fingerprint():
    files={}
    for path in sorted(ROOT.rglob('*')):
        rel=path.relative_to(ROOT)
        if not path.is_file() or any(p in {'build','release','.git','__pycache__'} for p in rel.parts): continue
        if path.suffix not in {'.c','.h','.s','.S','.ld','.sct','.uvprojx','.py'} and path.name!='Makefile': continue
        files[str(rel)]=hashlib.sha256(path.read_bytes()).hexdigest()
    return hashlib.sha256(json.dumps(files,sort_keys=True,separators=(',',':')).encode()).hexdigest()

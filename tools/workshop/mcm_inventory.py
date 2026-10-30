"""Verify documentation inventory against built exports without loading DLLs."""
from pathlib import Path
import struct
import build

def exports_mcm(path):
    data=path.read_bytes()
    u16=lambda p:struct.unpack_from('<H',data,p)[0]
    u32=lambda p:struct.unpack_from('<I',data,p)[0]
    pe=u32(0x3c)
    if data[pe:pe+4]!=b'PE\0\0' or u16(pe+24)!=0x20b:raise ValueError('Expected PE32+ plugin: '+str(path))
    sections=[]
    start=pe+24+u16(pe+20)
    for index in range(u16(pe+6)):
        p=start+40*index;sections.append((u32(p+12),max(u32(p+8),u32(p+16)),u32(p+20)))
    def offset(rva):
        for address,size,raw in sections:
            if address<=rva<address+size:return raw+rva-address
        if rva<min(s[0] for s in sections):return rva
        raise ValueError('Invalid export RVA: '+str(path))
    rva=u32(pe+24+112)
    if not rva:return False
    directory=offset(rva);count=u32(directory+24);names=offset(u32(directory+32))
    for i in range(count):
        p=offset(u32(names+4*i))
        if data[p:data.index(0,p)]==b'MCM_Describe':return True
    return False

def verify_catalog(catalog):
    known={entry['package'] for entry in catalog}
    build_root=Path(build.OURS).parent/'build';found={}
    for root in (Path(build.OURS),Path(build.FORKS)):
        for source in root.iterdir():
            if not source.is_dir():continue
            dlls=[p for p in (build_root/source.name).glob('*.dll') if exports_mcm(p)]
            if not dlls:continue
            mod='Character Inspector' if source.name=='CharacterInspector' else source.name
            found[mod]=[str(p) for p in dlls]
    missing=set(found)-known
    if missing:raise ValueError('MCM documentation misses built plugins: '+', '.join(sorted(missing)))
    for entry in catalog:
        if not entry['dll_export_verified']:raise ValueError('Unverified MCM support: '+entry['name'])
    return found

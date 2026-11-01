"""Read installed PE exports and running native code. ReadProcessMemory only."""
from pathlib import Path
import sys,struct,json,hashlib
sys.path.insert(0,'D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/BetterLooting/tests/_generated/pydeps')
sys.path.insert(0,'D:/DEV/Kenshi/tools')
import slots,livedis,capstone
out=Path(__file__).parent/'_generated';out.mkdir(exist_ok=True)
pid=livedis.find_pid();assert pid,'game must already be running; this script never launches it'
h=livedis.k32.OpenProcess(livedis.PROCESS_VM_READ|livedis.PROCESS_QUERY_INFORMATION,False,pid)
base=livedis.module_base(pid)
names=livedis.export_names();md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
patterns={'sfs-save':('saveGame@SaveFileSystem',0x800),'sfs-sync':('sync@SaveFileSystem',0x500),
          'load-info':('updateInfo@LoadSaveWindow',0x1200),'load-select':('selectGame@LoadSaveWindow',0x100),
          'save-request':('save@SaveManager',0x140),'load-show':('showLoad@SaveManager',0x100),
          'save-info':('loadInfo@SaveManager',0x350),
          'gui-shutdown':('shutDown@ForgottenGUI',0x400),
          'renderer-shutdown':('shutDown@Renderer',0x150),
          'render-owner-dtor':('_DESTRUCTOR@KingOfRenderThread',0x650),
          'native-save':('saveGame@SaveManager',0x3200)}
report={'pid':pid,'image_base':hex(base),'methods':{}}
try:
    for key,(pattern,count) in patterns.items():
        matches=slots.resolve(pattern);assert len(matches)==1,(key,matches)
        name,rva,slot=matches[0];code=livedis.read(h,base+rva,count)
        (out/(key+'.bin')).write_bytes(code)
        lines=[f'{name} RVA={rva:x} slot={slot} PID={pid}']
        for ins in md.disasm(code,base+rva):
            suffix=''
            if ins.mnemonic in ('call','jmp') and ins.op_str.startswith('0x'):
                target=int(ins.op_str,16)-base
                if target in names:suffix=' ; '+names[target]
            lines.append(f'{ins.address-base:08x} {ins.mnemonic:9} {ins.op_str}{suffix}')
            if ins.mnemonic=='int3':break
        (out/(key+'.asm')).write_text('\n'.join(lines),encoding='utf-8')
        report['methods'][key]={'name':name,'rva':hex(rva),'slot':slot,'sha256':hashlib.sha256(code).hexdigest()}
    for filename in ('KenshiLib.dll','OgreMain_x64.dll','MyGUIEngine_x64.dll','RenderSystem_Direct3D11_x64.dll'):
        p=Path('D:/SteamLibrary/steamapps/common/Kenshi')/filename
        if not p.exists():p=p.parent/'plugins'/filename
        if p.exists():report[filename]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
    def u64(a):return struct.unpack('<Q',livedis.read(h,a,8))[0]
    def u32(a):return struct.unpack('<I',livedis.read(h,a,4))[0]
    def rtti(obj):
        v=u64(obj);col=u64(v-8);module=col-u32(col+20)
        td=module+u32(col+12);ch=module+u32(col+16)
        value={'object':hex(obj),'vtable':hex(v),'complete_object_offset':u32(col+4),
               'type':livedis.read(h,td+16,160).split(b'\0')[0].decode(),'bases':[]}
        array=module+u32(ch+12)
        for j in range(u32(ch+8)):
            desc=module+u32(array+j*4);typ=module+u32(desc)
            value['bases'].append({'type':livedis.read(h,typ+16,160).split(b'\0')[0].decode(),
                                   'member_offset':u32(desc+8)})
        return value
    mb=livedis.module_base(pid,'myguiengine_x64.dll')
    manager=u64(mb+0x382380);report['mygui_render_manager']=rtti(manager)
    kb=livedis.module_base(pid,'kenshilib.dll');au=u64(kb+0xe5e58);renderer=u64(au+8);window=u64(renderer+0x78)
    report['native_window']=rtti(window)
    vtable=u64(window)
    report['native_window']['virtuals']=[hex(u64(vtable+j*8)) for j in range(35)]
    for key,address,count in [('window-copy',u64(vtable+26*8),0x850),('backend-render',u64(u64(manager+8)+3*8),0x180)]:
        code=livedis.read(h,address,count)
        (out/(key+'.asm')).write_text('\n'.join(f'{ins.address:016x} {ins.mnemonic:9} {ins.op_str}' for ins in md.disasm(code,address)),encoding='utf-8')
    (out/'save-preview-native-audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
finally:livedis.k32.CloseHandle(h)
print(f'Read-only native save/selection audit recorded for PID {pid}')

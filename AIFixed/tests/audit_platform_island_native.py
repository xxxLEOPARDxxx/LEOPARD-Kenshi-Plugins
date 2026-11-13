"""Offline proof of the supported native island query; never opens a process.

The vanilla GOG image is a comparison sample, not a supported RVA-table target.
Requires the already-vendored pefile/capstone beside SkillLevelUpInfo tests.
"""
from pathlib import Path
import hashlib, json, re, struct, sys
sys.dont_write_bytecode = True
sys.path.insert(0, 'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile, capstone

GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
PROFILES = (
    ('Steam', GAME/'RE_Kenshi/Kenshi_x64.exe',
     '504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e', 0x3a4d40, 0x3a4570),
    ('GOG-RE-supported', GAME/'_codex_work/20261105-keo-gog-native/re_kenshi/kenshi_GOG_x64.exe',
     '2defc287c813798b9682b93acf68d340da895cf5164d342145ba0faeb9024681', 0x3a4960, 0x3a4190),
    ('GOG-vanilla-comparison', GAME/'_codex_work/20261105-keo-gog-native/vanilla/kenshi_GOG_x64.exe',
     'c2fbac686513bc73a5058b8706fc4be5043598c21e2e4d4b248925fceb2cfe56', 0x3a4820, 0x3a4050),
)
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = True

def rows(instructions):
    return [dict(rva=hex(i.address), bytes=i.bytes.hex(), mnemonic=i.mnemonic,
                 operands=i.op_str) for i in instructions]

def normalize(instructions, start):
    result = []
    for i in instructions:
        operands = re.sub(r'\[rip(?: [+-] 0x[0-9a-f]+)?\]', '[rip + RELOCATION]', i.op_str)
        if i.mnemonic == 'call' and i.operands[0].type == capstone.x86.X86_OP_IMM:
            operands = 'FUNCTION_RELOCATION'
        elif i.mnemonic.startswith('j') and i.operands[0].type == capstone.x86.X86_OP_IMM:
            operands = hex(i.operands[0].imm-start)
        result.append((i.mnemonic, operands))
    return result

def rip(i):
    operand = next(o for o in i.operands if o.type == capstone.x86.X86_OP_MEM
                   and o.mem.base == capstone.x86.X86_REG_RIP)
    return i.address+i.size+operand.mem.disp

profiles = []
normal = None
resolve_normal = None
for name, path, expected, query, resolve in PROFILES:
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == expected, name
    pe = pefile.PE(data=raw, fast_load=True)
    ins = list(md.disasm(pe.get_data(query, 328), query))
    assert len(ins) == 72 and ins[-1].mnemonic == 'ret', name
    normalized = normalize(ins, query)
    if normal is None: normal = normalized
    assert normalized == normal, name
    # Exact wrapper semantics: fresh point lookup; radius doubled; threshold
    # squared; only successful flood exhaustion (status zero) admits an island.
    search = next(i for i in ins if i.mnemonic == 'movss' and i.op_str.startswith('xmm0, dword ptr [rip'))
    multiplier = next(i for i in ins if i.mnemonic == 'mulss' and i.op_str.startswith('xmm6,'))
    assert struct.unpack('<f', pe.get_data(rip(search), 4))[0] == 1.0
    assert struct.unpack('<f', pe.get_data(rip(multiplier), 4))[0] == 2.0
    assert any(i.mnemonic == 'cmp' and i.op_str == 'eax, -1' for i in ins)
    assert any(i.mnemonic == 'mulss' and i.op_str == 'xmm0, xmm0' for i in ins)
    assert any(i.mnemonic == 'cmp' and i.op_str == 'byte ptr [rsp + 0x3d], 0' for i in ins)
    resolve_ins = list(md.disasm(pe.get_data(resolve, 0x7c2), resolve))
    assert resolve_ins[-1].mnemonic == 'ret', name
    normalized_resolve = normalize(resolve_ins, resolve)
    if resolve_normal is None: resolve_normal = normalized_resolve
    assert normalized_resolve == resolve_normal, name
    assert any('0x1f4' in i.op_str for i in resolve_ins), name  # bounded fallback candidates
    assert not any('[r' in i.op_str and ('+ 0x200]' in i.op_str or '+ 0x1e0]' in i.op_str)
                   for i in ins+resolve_ins), name
    profiles.append(dict(name=name, executable=str(path), sha256=expected,
                         query_rva=hex(query), resolve_rva=hex(resolve),
                         wrapper=rows(ins), resolve_point=rows(resolve_ins)))
    if name == 'Steam': steam_pe = pe

def steam_window(start, size):
    return list(md.disasm(steam_pe.get_data(start, size), start))

path_lock = steam_window(0x3a5d68, 0x5d)
assert path_lock[0].op_str == 'rbx, [rcx + 0x200]'
assert any(i.mnemonic == 'bt' and i.op_str == 'ecx, 0x16' for i in path_lock)
assert any(i.mnemonic == 'lock cmpxchg' for i in path_lock)
assert any(i.mnemonic == 'test' and i.op_str == 'ecx, ecx' for i in path_lock)
visitor = steam_window(0x3b8250, 0xc8)
assert len([i for i in visitor if i.mnemonic == 'jae']) == 2
flood_status = steam_window(0xce54be, 0x51)
assert any(i.op_str == 'ebx, 4' for i in flood_status)
assert any(i.op_str == 'byte ptr [rax + 0xd], bl' for i in flood_status)
flood_budget = steam_window(0xce5010, 0x98)
assert any(i.op_str == 'eax, 0x20000' for i in flood_budget)
assert any(i.op_str == 'eax, 0x90200' for i in flood_budget)
native_caller = steam_window(0x14635e, 0x1d)
assert any(i.op_str == 'xmm3, dword ptr [rbx + 0x70]' for i in native_caller)
assert any(i.op_str == 'dword ptr [rsp + 0x20], xmm6' for i in native_caller)
assert struct.unpack('<f', steam_pe.get_data(0x167ead4, 4))[0] == 10.0
table_gog = (GAME/'RE_Kenshi/RVAs/GOG_1.0.65.br').read_bytes()
assert struct.unpack_from('<I', table_gog, 5006*4)[0] == 0x3a4960
ray_proof=[]
for symbol,slot,steam_rva,expect_shape in [('UtilityT::trace',7164,0x9b5bc0,True),('UtilityT::sphereTrace',7168,0x9b6060,False)]:
    reference=None
    for profile in profiles[:2]:
        pe_image=pefile.PE(profile['executable'],fast_load=True)
        rva=steam_rva if profile['name']=='Steam' else struct.unpack_from('<I',table_gog,slot*4)[0]
        body=[]
        for instruction in md.disasm(pe_image.get_data(rva,0x400),rva):
            body.append(instruction)
            if instruction.mnemonic=='ret':break
        assert body[-1].mnemonic=='ret'
        normalized=normalize(body,rva)
        if reference is None:reference=normalized
        assert reference==normalized,(symbol,profile['name'])
        shape_store=any(i.mnemonic=='mov' and i.op_str.startswith('qword ptr [rbx + 0x28],') for i in body)
        assert shape_store==expect_shape,(symbol,profile['name'])
        ray_proof.append(dict(name=symbol,profile=profile['name'],slot=slot,rva=hex(rva),populates_shape=shape_store,body=rows(body)))
out = Path(__file__).resolve().parent/'_generated'
out.mkdir(exist_ok=True)
report = dict(profiles=profiles, wrapper_instruction_count=72,ray_shape_proof=ray_proof,
    official_export_slot=5006,
    conclusions=dict(units='Havok meters: exact native HK position/radius, not world coordinates',
        fresh_face_lookup=True, no_face_result=False, only_exhausted_flood_result=True,
        bounded_resolve_candidates=500, default_search_memory_bytes=[0x20000, 0x90200],
        locking='Caller must try-shared-lock nav.changeMutex (+0x200); reject if busy; native wrapper does not lock.',
        budget_caveat='Spatial threshold is not a hard time limit; traversal also has finite native memory budgets.',
        runtime_result='Not measured: game exited during read-only physical capture.'),
    steam_windows=dict(path_exists_try_shared=rows(path_lock), edge_visitor=rows(visitor),
        flood_status=rows(flood_status), flood_budget=rows(flood_budget), native_caller=rows(native_caller)))
(out/'platform-island-native.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('AIFixed platform island native: PASS; matching 72-instruction wrapper + 485-instruction resolvePoint on Steam/supported GOG/vanilla comparison; units, finite budgets, try-shared lock; Steam/GOG ray retains NxShape, sphere does not')

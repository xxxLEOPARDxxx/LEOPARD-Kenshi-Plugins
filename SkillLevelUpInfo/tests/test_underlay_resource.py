"""Primary XML and installed native container/getter proof; no game calls."""
from pathlib import Path
import hashlib,json,struct,sys,math,xml.etree.ElementTree as ET
here=Path(__file__).resolve().parent
game=Path(r'D:/SteamLibrary/steamapps/common/Kenshi')
profiles=[]
for path in [game/'data/gui/skins/kenshi_skins.xml',game/'mods/Dark UI Advanced/gui/skins/kenshi_skins.xml']:
 root=ET.parse(path).getroot()
 skin=next(x for x in root.iter('Resource')if x.get('name')=='Kenshi_StatsPanelSkin')
 basis=skin.findall('BasisSkin');assert len(basis)==1 and basis[0].get('type')=='SubSkin'
 state=next(x for x in basis[0].findall('State')if x.get('name')=='normal')
 atlas=list(map(int,state.get('offset').split()));profiles.append(atlas)
assert profiles==[[0,907,240,165],[0,907,277,165]],profiles
print('PASS primary stock/Dark StatsPanel atlas geometry distinguishes the backed column profile without colour detection')
capture=json.loads((here/'underlay_profile.json').read_text())
assert capture['samples']==24 and capture['valid_samples']==22 and capture['polling_races']==2
mapped=math.ceil(capture['background_quad'][2]*capture['value_backing_atlas_x']/capture['stock_atlas'][2])-capture['content_absolute'][0]
assert mapped==capture['candidate_bar_local'][0] and capture['candidate_name_width']>=capture['measured_caption_width']
sys.path.insert(0,str(here/'_tools'))
import pefile,capstone
path=game/'MyGUIEngine_x64.dll';pe=pefile.PE(str(path));machine=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
def body(term,size):
 r=next(e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name and term in e.name)
 for _ in range(4):
  if pe.get_data(r,1)!=b'\xe9':break
  r+=5+struct.unpack('<i',pe.get_data(r+1,4))[0]
 return r,pe.get_data(r,size)
for term,expected in [(b'getBasisInfo@ResourceSkin',bytes.fromhex('488d4160c3')),(b'getStateInfo@ResourceSkin',bytes.fromhex('488d8180000000c3'))]:
 r,data=body(term,12);assert data.startswith(expected),(term,hex(r),data.hex())
r,data=body(b'_createSkinItem@SkinItem',1800)
instructions=[(x.mnemonic,x.op_str)for x in machine.disasm(data,pe.OPTIONAL_HEADER.ImageBase+r)]
assert any(m=='add'and op.endswith(', 0x40')for m,op in instructions),instructions
r,data=body(b'setStateData@SubSkin',48)
assert any(x.mnemonic=='lea'and x.op_str=='rdx, [rax + 8]'for x in machine.disasm(data,pe.OPTIONAL_HEADER.ImageBase+r))
print('PASS installed native ResourceSkin vector/map getters, SubWidgetInfo64 stride and state-rect8 match production VC10 layout')
(here/'_generated'/'underlay_resource_proof.json').write_text(json.dumps({'engine_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'atlas_profiles':profiles,'runtime_calls':False},indent=2)+'\n')

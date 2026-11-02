"""Generated-copy mutants; production source is never edited or bypassed."""
from pathlib import Path
import json,os,subprocess,sys
import generate_catalog as generator

HERE=Path(__file__).resolve().parent
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=str(VC/'include')+';'+str(SDK/'Include'),LIB=str(VC/'lib/amd64')+';'+str(SDK/'Lib/x64'))
header=(HERE/'RegionCatalog.h').read_text();data=(HERE/'RegionCatalogData.h').read_text();cpp=(HERE/'catalog_tests.cpp').read_text()
mutants={
 'residual_subtraction':('out[0]-=out[i];','out[0]+=out[i];'),
 'wrong_blend_channel':('rgba[c.selector[i]>>4]','rgba[0]'),
 'wrong_parent_paint_guard':('r.paintX!=-147456+int(x/2)*73728','(r.paintX< -147456||r.paintX>147456)'),
 'skip_deterministic_padding':('if(source!=r.banks[begin])return false;',''),
 'skip_tree_hash_guard':('!Hash(cell.treeSha256)||',''),
 'wrong_global_local_code':('unsigned(r.biomes[b].source)+1==globalCode','unsigned(r.biomes[b].source)==globalCode'),
 'ascending_native_slots':('for(int slot=4;slot>=0;--slot)','for(int slot=0;slot<5;++slot)'),
 'skip_selector_padding':('if(r.selector[(zCell*8+xCell)*4+slot]!=(xCell<4&&zCell<4?r.cells[zCell*4+xCell].selector[slot]:0))return false;','(void)r.selector[0];'),
}
cases=[(name,header.replace(old,new),data) for name,(old,new) in mutants.items() if header.count(old)==1]
assert len(cases)==len(mutants),'mutant source seam changed'
doc=json.loads((HERE/'catalog.json').read_text())
for index,sid in ((1,'4415-Newwworld.mod'),(28,'2743-Newwworld.mod'),(34,'2990-Newwworld.mod'),(53,'3116-Newwworld.mod'),(62,'51632-Newwworld.mod')):
    bad=json.loads(json.dumps(doc));code=next(i+1 for i,b in enumerate(bad['biomes']) if b['sid']==sid)
    cell=next(c for c in bad['regions'][index]['cells'] if code in c['slots']);cell['slots'][cell['slots'].index(code)]=0
    cases.append(('drop_unused_slot_'+str(index),header,generator.cpp_data(bad)))
for name,h,d in cases:
    folder=HERE/'build/mutants'/name;folder.mkdir(parents=True,exist_ok=True)
    (folder/'RegionCatalog.h').write_text(h);(folder/'RegionCatalogData.h').write_text(d);(folder/'catalog_tests.cpp').write_text(cpp)
    result=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4','/WX','/I'+str(HERE),
        '/Fe'+str(folder/'test.exe'),'/Fo'+str(folder/'test.obj'),str(folder/'catalog_tests.cpp')],env=env,capture_output=True,text=True)
    if result.returncode:raise RuntimeError('mutant must compile '+name+'\n'+result.stdout+result.stderr)
    result=subprocess.run([str(folder/'test.exe'),str(HERE/'build/source.fixture')],capture_output=True,text=True)
    (folder/'result.log').write_text(result.stdout+result.stderr)
    if result.returncode==0:raise RuntimeError('SURVIVING MUTANT '+name)
    print('PASS mutant rejected:',name,(result.stdout+result.stderr).splitlines()[0])

# Remove real production file verification in an isolated generator copy. The
# existing payload mutation test must detect the missing hash behavior.
folder=HERE/'build/mutants/skip_file_sha';folder.mkdir(parents=True,exist_ok=True)
source=(HERE/'generate_catalog.py').read_text();old="require(sha(path)==entry['sha256'],'source SHA256 changed: '+str(path))"
assert source.count(old)==1
(folder/'generate_catalog.py').write_text(source.replace(old,'pass # generated mutant'))
for name in ('test_generator.py','source-manifest.json'):(folder/name).write_bytes((HERE/name).read_bytes())
(folder/'build').mkdir(exist_ok=True)
result=subprocess.run([sys.executable,str(folder/'test_generator.py'),'CatalogGeneratorTests.test_strict_dds_header_payload_hash_and_png_shape_refusals'],capture_output=True,text=True)
(folder/'result.log').write_text(result.stdout+result.stderr)
if result.returncode==0 or 'AssertionError: ValueError not raised' not in result.stderr:raise RuntimeError('file SHA mutant not detected as intended')
print('PASS mutant rejected: skip_file_sha (actual changed DDS payload accepted without guard)')
print('PASS14 generated-copy mutants, production files untouched')

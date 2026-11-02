"""Actual production DXBC/native-file contracts, not a second shader implementation."""
import generate_models as g
import hashlib,json,re
j=json.loads((g.OUT/'models.json').read_text());checks=0
def check(v):
 global checks
 assert v
 checks+=1
for row in j['sources']:check(hashlib.sha256((g.GAME/row['path']).read_bytes()).hexdigest()==row['sha256'])
for shader in j['shaders']:
 code=(g.OUT/(shader['symbol']+'.cso')).read_bytes();check(hashlib.sha256(code).hexdigest()==shader['sha256'])
vs=(g.OUT/'AtlasModelVertexShader.reflection.txt').read_text();ps=(g.OUT/'AtlasModelPixelShader.reflection.txt').read_text()
for name,offset,size in [('worldViewProjMatrix',0,64),('worldMatrix',64,64)]:check(bool(re.search(r'\b'+name+r';.*Offset:\s*'+str(offset)+r'\s+Size:\s*'+str(size)+r'\b',vs)))
for name,offset,size in [('cameraPos',0,12),('farClip',12,4)]:check(bool(re.search(r'\b'+name+r';.*Offset:\s*'+str(offset)+r'\s+Size:\s*'+str(size)+r'\b',ps)))
check('diffuseMap' in ps and 't0' in ps and 'sampleState' in ps and 's0' in ps)
check(all(x in vs for x in ['POSITION','NORMAL','COLOR','TEXCOORD']))
for n,v in {0:41,1:16,2:6,3:2,4:28,10:28,11:28}.items():check(j['nativeVertexFormats'][str(n)]==v)
native=(g.HERE/'NativeModel.h').read_text();check(native.index('model current material unsupported')<native.index('Ogre::RenderOperation op;'))
check('?_getRenderOperation@SubMesh' in native and 'getRenderOperation@SubEntity' not in native)
check('getPosition(' not in native and 'getPosition@Town' not in native)
for forbidden in ['->load','createEntity(','->clone','loadParts(','showDistantTown(','->setVisible','injectRender','setToLoaded','_notifyResourceLoaded']:
 check(forbidden not in native)
check('RawSceneWorld()' in (g.HERE/'AtlasModel.h').read_text())
check('AtlasView::GpuCap-PrivateBytes' in (g.HERE/'ModelPolicy.h').read_text())
print(f'PASS {checks} actual native source/DXBC/ABI/production route contracts')

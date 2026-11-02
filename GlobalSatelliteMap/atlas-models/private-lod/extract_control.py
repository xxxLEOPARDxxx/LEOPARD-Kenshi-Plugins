from pathlib import Path
import argparse
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
a=argparse.ArgumentParser();a.add_argument('--source',type=Path,default=HERE/'NativePrivateLod.h');args=a.parse_args();text=args.source.read_text()
build=text[text.index(' void Build(){'):text.index(' void Import(Ogre::Resource*r){')]
identity=text[text.index(' bool Identity(){'):text.index(' void ClearPacket(){')]
native_import=text[text.index(' void Import(Ogre::Resource*r){'):text.index(' bool Identity(){')]
(OUT/'control.extract.h').write_text(build+'\n'+native_import+'\n'+identity)
objects=text[text.index('struct NativeStorage {'):text.index('class State;')]
(OUT/'objects.extract.h').write_text(objects)
start=text.index('  if(!enabled){',text.index(' HRESULT Read('));end=text.index('BlobInfo info;',start)
(OUT/'entry.extract.h').write_text('HRESULT Entry(bool enabled,bool verified){\n'+text[start:end]+'return S_OK;}\n')
print('Extracted actual private Build/Identity and OFF/source-entry prefix')

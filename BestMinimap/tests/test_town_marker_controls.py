"""Compile the production index with missing lifetime/map-identity controls."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[1]
out=Path('D:/DEV/Kenshi/build/BestMinimapTests/marker-controls');out.mkdir(parents=True,exist_ok=True)
s=(root/'BestMinimap.cpp').read_text(encoding='utf-8-sig')
s=s[s.index('    std::map<TownBase*, MyGUI::ImageBox*> g_markerByTown;'):s.index('    // Что у поселения')]
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include');env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
for name,old,new in [('ignored-widget-destruction','if (m_images.find(widget) != m_images.end())','if (false)'),('ignored-map-replacement','map == g_markersIndexedFor && ','')]:
 assert s.count(old)==1
 d=out/name;d.mkdir(exist_ok=True);(d/'town_marker_runtime_under_test.h').write_text(s.replace(old,new),encoding='utf-8')
 cmd=[str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/I'+str(d),'/Fo'+str(d/'test.obj'),'/Fe'+str(d/'test.exe'),str(root/'tests/town_marker_lifecycle_tests.cpp')]
 c=subprocess.run(cmd,env=env,capture_output=True,text=True);(d/'compile.log').write_text(c.stdout+c.stderr);assert c.returncode==0,(name,c.stdout,c.stderr)
 r=subprocess.run([str(d/'test.exe')],capture_output=True,text=True);(d/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==1 and 'CHECK line' in r.stdout,(name,r.returncode,r.stdout,r.stderr)
 print(name,'compiled, CHECK exit1')
print('PASS 2 production lifetime controls')

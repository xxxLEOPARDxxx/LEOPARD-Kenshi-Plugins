from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent
sys.path.insert(0,r'D:/DEV/Kenshi/tools/_native_python');import pefile
p=pefile.PE(r'D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe')
out=here/'_generated/engineer_priority';out.mkdir(parents=True,exist_ok=True)
image=out/'steam_mapped.bin';image.write_bytes(p.get_memory_mapped_image().ljust(p.OPTIONAL_HEADER.SizeOfImage,b'\0'))
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ['/Od','/O2']:
 exe=out/('native'+opt[1:]+'.exe')
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(here/'engineer_priority_native_fixture.cpp'),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True)
 subprocess.run([str(exe),str(image)],check=True)

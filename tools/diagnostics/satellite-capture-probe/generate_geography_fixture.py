from pathlib import Path
from PIL import Image
import json,struct,math
p=Path(__file__).resolve().parent;out=p/'build/geography';out.mkdir(exist_ok=True)
manifest=json.loads((p/'geography-inputs.json').read_text());image=Image.open('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/blendmap.png').convert('RGBA')
(out/'blend.rgba').write_bytes(image.tobytes());pixels=image.load();points=[];expected=[]
def point(x,z,weights):
    if -73728<=x<0 and -73728<=z<0:
        cell=int((x+73728)//9216)+8*int((z+73728)//9216);codes=manifest['cells'][cell]['selector']
        result=[0.0]*4;result[0]=1.0
        for i in range(1,4):
            if codes[i]&15:result[i]=weights[codes[i]>>4];result[0]-=result[i]
    else:codes=[0]*4;result=[0]*4
    points.append(struct.pack('<ff',x,z));expected.append(struct.pack('<4I4f',*codes,*result))
for z in range(256):
    for x in range(256):point(-73728+(x+.5)*288,-73728+(z+.5)*288,[v/255.0 for v in pixels[x+256,z+256]])
for x,z in ((360,256),(361,256),(360,257),(455,266),(456,266),(457,266)):
    rgba=[sum(pixels[px,pz][c] for px,pz in ((x,z),(x+1,z),(x,z+1),(x+1,z+1)))/(4*255) for c in range(4)]
    point(-147456+(x+1)*288,-147456+(z+1)*288,rgba)
for x,z in ((-73728.125,-40000),(.125,-40000),(-40000,-73728.125),(-40000,.125)):point(x,z,[0]*4)
(out/'sampling.points').write_bytes(b''.join(points));(out/'sampling.expected').write_bytes(b''.join(expected))
print('Prepared actual native blend PNG and',len(points),'world-pixel GPU cases, all65536fine centers +6bilinear fixtures +outside bounds')

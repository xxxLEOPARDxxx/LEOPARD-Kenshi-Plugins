"""Generate a narrow modern-compute adapter from the installed native far-water body."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
WATER=GAME/'data/materials/forward/water.hlsl'
LIGHT=GAME/'data/materials/common/lightingFunctions.hlsl'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
WATER_HASH='347e9c59ada723e6033cc8a875dd3e724ee8ba5549b4d8bac38ae3cd4265baa5'
LIGHT_HASH='51dd7cec6f898b3a79352067dbfbe586c1d364967154609ed821667d22966f9f'

def verify_hash(raw,expected):
    if hashlib.sha256(raw).hexdigest()!=expected:raise ValueError('native source identity changed; audit before adapting')

def function(source,marker):
    start=source.index(marker);begin=source.index('{',start);depth=0
    for i in range(begin,len(source)):
        if source[i]=='{':depth+=1
        elif source[i]=='}':
            depth-=1
            if depth==0:return source[start:i+1]
    raise ValueError('unbalanced actual native function')

def once(text,old,new):
    if text.count(old)!=1:raise ValueError('native adapter site changed: '+old)
    return text.replace(old,new)

def canonical(text):return re.sub(r'[ \t]*([(),;.])[ \t]*',r'\1',text)

def generate():
    OUT.mkdir(exist_ok=True)
    verify_hash(WATER.read_bytes(),WATER_HASH);verify_hash(LIGHT.read_bytes(),LIGHT_HASH)
    pre=OUT/'native-preprocessed.hlsl'
    subprocess.run([str(FXC),'/nologo','/P',str(pre),'/D','DISTANT=1','/D','NO_REFLECTION=1','/I',str(LIGHT.parent),str(WATER)],check=True,capture_output=True)
    native=re.sub(r'^\s*#line.*$','',pre.read_text(encoding='latin1').rstrip('\0'),flags=re.M)
    native=canonical(native)
    pre.write_text(native,encoding='ascii')
    # Preserve all pure native BRDF functions; modernize only sampler syntax in
    # the actual CalcEnvironmentLight implementation, never recreate its math.
    light_start=native.index('static const float PI')
    env=function(native,'LightingData CalcEnvironmentLight')
    lighting=native[light_start:native.index('LightingData CalcEnvironmentLight')]+env
    lighting=once(lighting,'samplerCUBE irradianceSampler,samplerCUBE specularitySampler,','TextureCube<float4> irradianceTexture, TextureCube<float4> specularityTexture, SamplerState irradianceSampler, SamplerState specularitySampler,')
    lighting=once(lighting,'texCUBElod(irradianceSampler,float4(normal,3.0f))','irradianceTexture.SampleLevel(irradianceSampler,normal,3.0f)')
    lighting=once(lighting,'texCUBElod(specularitySampler,float4(specDir,reflMip))','specularityTexture.SampleLevel(specularitySampler,specDir,reflMip)')
    sample=function(native,'float3 sampleNormal')
    sample=once(sample,'sampler2D map,','Texture2D<float4> map, SamplerState mapSampler,')
    sample=once(sample,'float time)','float time,float2 baseDx,float2 baseDy,float2 flowDx,float2 flowDy)')
    sample=once(sample,'tex2D(map,coord + direction * speed * t)','map.SampleGrad(mapSampler,coord + direction * speed * t,baseDx + flowDx*t,baseDy + flowDy*t)')
    water=function(native,'void waterFP');body=water[water.index('{')+1:-1]
    body,count=re.subn(r'float clipVal = dist - 4000;.*?clip\(clipVal\);','/* Map explicitly selects native distant material; no perspective LOD switch. */',body,count=1,flags=re.S)
    if count!=1:raise ValueError('native distant selector changed')
    body=once(body,'float3 rf = saturate(tex2D(reflection,refTex).rrr + 1.0);','float3 rf = 1.0f;')
    body=once(body,'direction /= speed;','direction = speed > 0.0f ? direction / speed : float2(0,0);')
    flow_begin=body.index('float2 direction = info.rg')
    flow_end=body.index('gameTime *= distortion.x;')
    displacement='float2 NativeFlowDisplacement(float4 info,float2 speedMult,float distortionX){\n'+body[flow_begin:flow_end].replace('distortion.x','distortionX')+'\nreturn direction*speed;\n}\n'
    body=once(body,'tex2D(colourmap,map.xy)','colourmap.SampleGrad(colourSampler,map.xy,mapDx,mapDy)')
    body=once(body,'tex2D(flowmap,map.xy)','flowmap.SampleGrad(flowSampler,map.xy,mapDx,mapDy)')
    body=body.replace('sampleNormal(normalmap,','sampleNormal(normalmap,normalSampler,')
    body,calls=re.subn(r'(direction,speed,gameTime(?: \+ 0\.(?:33|66))?)\);',r'\1,baseDx,baseDy,flowDx,flowDy);',body)
    if calls!=3:raise ValueError('native three temporal normal samples changed')
    body=once(body,'CalcEnvironmentLight(irradianceCube,specularityCube,','CalcEnvironmentLight(irradianceCube,specularityCube,irradianceSampler,specularitySampler,')
    coordinates='''
float2 NativeAbsoluteXZ(uint2 p){return tileBounds.xy+(float2(p)+.5f)/sourceWindow.x*tileBounds.zw;}
float2 NativeFlowAt(uint2 p){
 float2 worldStep=tileBounds.zw/sourceWindow.x;
 float2 uv=(NativeAbsoluteXZ(p)-nativeMapBounds.xy)/(nativeMapBounds.zw-nativeMapBounds.xy);
 float2 dx=float2(worldStep.x/(nativeMapBounds.z-nativeMapBounds.x),0);
 float2 dy=float2(0,worldStep.y/(nativeMapBounds.w-nativeMapBounds.y));
 return NativeFlowDisplacement(flowmap.SampleGrad(flowSampler,uv,dx,dy),normalScale.xy*5000,materialParams.y);
}
'''
    local='''
float4 NativeWaterDistantMaterial(uint2 pixel, float3 parallelView) {
 float2 absoluteXZ=NativeAbsoluteXZ(pixel);
 float4 tex=absoluteXZ.xyxy*normalScale;
 float4 map=float4((absoluteXZ-nativeMapBounds.xy)/(nativeMapBounds.zw-nativeMapBounds.xy),absoluteXZ*.01f);
 float2 speedMult=normalScale.xy*5000;
 float3 sun=waterSunDirection.xyz,cam=parallelView;
 float4 sunColour=waterSunColour,envColour=waterEnvColour;
 float invStrength=materialParams.x,gloss=materialParams.z,glow=materialParams.w;
 float2 distortion=float2(materialParams.y,0);
 float gameTime=waterTime.x;
 float2 worldStep=tileBounds.zw/sourceWindow.x;
 float2 mapDx=float2(worldStep.x/(nativeMapBounds.z-nativeMapBounds.x),0);
 float2 mapDy=float2(0,worldStep.y/(nativeMapBounds.w-nativeMapBounds.y));
 float2 baseDx=float2(worldStep.x*normalScale.x,0),baseDy=float2(0,worldStep.y*normalScale.y);
 uint2 quad=pixel&~uint2(1,1);
 float2 flowDx=NativeFlowAt(uint2(quad.x+1,pixel.y))-NativeFlowAt(uint2(quad.x,pixel.y));
 float2 flowDy=NativeFlowAt(uint2(pixel.x,quad.y+1))-NativeFlowAt(uint2(pixel.x,quad.y));
 float4 colour;
'''
    generated=lighting+'\n'+sample+'\n'+displacement+coordinates+local+body+'\n return colour;\n}\n'
    if any(word in generated for word in ['tex2D(', 'texCUBElod(', 'depthmap','scummap','refTex']):raise ValueError('unexpected native dependency remains')
    (OUT/'NativeWaterMaterial.hlsl').write_text(generated,encoding='ascii')
    evidence=dict(waterSha256=WATER_HASH,lightingSha256=LIGHT_HASH,generatedSha256=hashlib.sha256(generated.encode()).hexdigest(),adapterSites=dict(distanceSelector=1,reflectionFold=1,stationaryGuard=1,sampleNormal=1,waterSamples=2,cubeSamples=2),scope='DISTANT+NO_REFLECTION; explicit map material selection; finite stationary-flow extension; no main-camera/depth/reflection binding')
    (OUT/'source-contract.json').write_text(json.dumps(evidence,indent=2)+'\n')
    return evidence

if __name__=='__main__':print(json.dumps(generate(),indent=2))

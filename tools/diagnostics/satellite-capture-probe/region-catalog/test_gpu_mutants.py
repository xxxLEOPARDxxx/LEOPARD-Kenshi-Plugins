"""Real WARP mutants of the actual shared production regional selector."""
from pathlib import Path
import subprocess
HERE=Path(__file__).resolve().parent;source=(HERE.parent/'GeographySampling.hlsl').read_text();wrapper=(HERE/'region_sampling_cs.hlsl').read_text()
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
mutants={
 'fixed_primary_origin':('(worldXZ-selectorRegion.xy)','(worldXZ-float2(-73728.0,-73728.0))'),
 'patch_center':('(worldXZ-selectorRegion.xy)','(float2(-36864.0,-36864.0)-selectorRegion.xy)'),
 'wrong_coarse_scale':('/9216.0','/36864.0'),
 'ignore_active4x4':('>=(int2)selectorRegion.zw','>=int2(8,8)'),
 'wrong_source_channel':('blendWeights[selected[i]>>4u]','blendWeights[0]'),
 'clamped_residual':('return weights;','return max(weights,0.0);'),
}
for name,(old,new) in mutants.items():
    assert source.count(old)==1,name
    out=HERE/'build/gpu-mutants';out.mkdir(exist_ok=True)
    path=out/(name+'.hlsl');path.write_text(wrapper.replace('#include "../GeographySampling.hlsl"',source.replace(old,new)))
    cso=path.with_suffix('.cso');result=subprocess.run([fxc,'/nologo','/T','cs_5_0','/E','main','/O3','/Ges','/Fo'+str(cso),str(path)],capture_output=True,text=True)
    if result.returncode:raise RuntimeError('mutant must compile '+name+'\n'+result.stdout+result.stderr)
    result=subprocess.run([str(HERE/'build/region_sampling_warp.exe'),str(cso),str(HERE/'build/blend.rgba')],capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr)
    if result.returncode==0 or 'FAIL' not in result.stderr:raise RuntimeError('SURVIVING GPU MUTANT '+name)
    print('PASS real GPU mutant rejected:',name,result.stderr.strip())
print('PASS6 real WARP shared regional selector mutants')

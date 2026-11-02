"""Compile functional copies; never add production skip flags or edit native assets."""
from pathlib import Path
import os, subprocess, json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
CL=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/cl.exe')
CASES=[
 ('reject-completed-state2','ModelAdmission.h','state!=2&&state!=3','state!=3','admission'),
 ('accept-pending-state1','ModelAdmission.h','state!=2&&state!=3','state!=1&&state!=2&&state!=3','admission'),
 ('index-start','AtlasModel.h','d.shape.indexCount,d.shape.indexStart,INT(d.shape.vertexStart)','d.shape.indexCount,0,INT(d.shape.vertexStart)','warp'),
 ('base-vertex','AtlasModel.h','INT(d.shape.vertexStart)','0','warp'),
 ('matrix-transpose','ModelPolicy.h','out[c*4+r]=q;','out[r*4+c]=q;','warp'),
 ('add-unproved-origin','AtlasModel.h','std::memcpy(rawSceneWorld_,world,64);','std::memcpy(rawSceneWorld_,world,64);rawSceneWorld_[3]+=origin[0];','warp'),
 ('wrong-colour-stream','AtlasModel.h','e[i].InputSlot=r.shape.element[i].slot;','e[i].InputSlot=r.shape.element[i].semantic==2?0:r.shape.element[i].slot;','warp'),
 ('wrong-winding','AtlasModel.h','rs.FrontCounterClockwise=TRUE;','rs.FrontCounterClockwise=FALSE;','warp'),
 ('no-depth-test','AtlasModel.h','ds.DepthEnable=TRUE;','ds.DepthEnable=FALSE;','warp'),
 ('wrong-radial-depth','AtlasModel.h','v.cameraScene[2],v.farClip','v.cameraScene[2],v.farClip*2','warp'),
 ('keep-old-GS','AtlasModel.h','c.p->GSSetShader(NULL,NULL,0);','','warp'),
 ('omit-PS-restore','GraphicsState.h','c_->PSSetShader(ps_,classes_[4],count_[4]);','','warp'),
 ('inherit-predicate','AtlasModel.h','c.p->SetPredication(NULL,FALSE);','','warp'),
 ('accept-SO-cursor','AtlasModel.h','!c.p||!NoStreamOutput(c.p)','!c.p','warp'),
 ('allow-source-output-alias','AtlasModel.h','if(AtlasView::Identity(r.p,texture_[k].p))return E_INVALIDARG;','','warp'),
 ('stale-world','AtlasModel.h','v.generation!=p.generation_','false','warp'),
 ('double-retain-buffer','AtlasModel.h','r.vertex[i]->AddRef();','r.vertex[i]->AddRef();r.vertex[i]->AddRef();','warp'),
 ('unverified-query','ModelAdmission.h','if(!verified||!generation)','if(!generation)','admission'),
 ('stale-registry-index','ModelAdmission.h','reinterpret_cast<const void*const*>(begin)[index]==entity','true','admission'),
]
def main():
 results=[]
 for name,file,old,new,test in CASES:
  dest=OUT/('mutant-'+name);dest.mkdir(exist_ok=True)
  for header in ['AtlasModel.h','ModelPolicy.h','GraphicsState.h','ModelAdmission.h']:
   text=(HERE/header).read_text()
   if header==file:
    assert text.count(old)==1,(name,text.count(old));text=text.replace(old,new)
   (dest/header).write_text(text)
  define='/DATLAS_MODEL_HEADER="'+str(dest/'AtlasModel.h').replace('\\','/')+'"' if test=='warp' else '/DMODEL_ADMISSION_HEADER="'+str(dest/'ModelAdmission.h').replace('\\','/')+'"'
  exe=dest/'test.exe'
  args=[str(CL),'/nologo','/EHsc','/O2','/MT',define,str(HERE/(test+'_tests.cpp')),'/Fo'+str(dest/'test.obj'),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib']
  build=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(dest/'build.log').write_text(build.stdout)
  assert build.returncode==0,(name,'compile failure',build.stdout)
  run=subprocess.run([str(exe)],cwd=OUT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(dest/'run.log').write_text(run.stdout)
  assert run.returncode!=0 and 'FAIL ' in run.stdout,(name,'survived or failed unfairly',run.stdout)
  reason=next(s for s in run.stdout.splitlines() if 'FAIL ' in s);results.append(dict(name=name,result=reason));print('Expected RED:',name,'->',reason)
 (OUT/'mutants.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS',len(results),'compiled functional mutants rejected')
if __name__=='__main__':main()

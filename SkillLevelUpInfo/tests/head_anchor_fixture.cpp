bool Near(float a,float b){return fabs(a-b)<0.001f;}
bool Same(const Ogre::Vector3& a,const Ogre::Vector3& b){return Near(a.x,b.x)&&Near(a.y,b.y)&&Near(a.z,b.z);}
void Gain(Character& c){c.stats.value+=1;Scan();}
int main(){
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;
 Character c;player.playerCharacters.push_back(&c);Scan();
 check(c.boneCalls==0&&c.positionCalls==0,"silent baseline performs no anchor queries");
 for(int i=0;i<1000;++i)Scan();
 check(c.boneCalls==0&&labels==0,"stable scans perform no bone queries or label creation");
 c.bones[0]=c.base+Ogre::Vector3(1,9,-2);Gain(c);
 check(Same(c.base+ui.label.offset,c.bones[0]+Ogre::Vector3(0,.4f,0)),"actual Scan starts at head world point plus gap, not fixed feet offset");
 check(c.boneCalls==1&&ui.label.tracked.ptr==&c,"one successful bone lookup and native character tracking per notification");
 Ogre::Vector3 offset=ui.label.offset,shift(1300,-45,780);c.base+=shift;c.bones[0]+=shift;Gain(c);
 check(Same(ui.label.offset,offset),"tracking offset is invariant under world translation");
 Ogre::Vector3 movingBase=c.base+Ogre::Vector3(8,2,9);
 check(Same(movingBase+ui.label.offset,c.bones[0]+Ogre::Vector3(8,2.4f,9)),"native relative tracking follows later character movement");
 c.bones[0]=c.base+Ogre::Vector3(-2,-1,3);Gain(c);
 check(Same(c.base+ui.label.offset,c.bones[0]+Ogre::Vector3(0,.4f,0)),"lying character preserves all three world-space head components");
 c.bones[0]=c.base+Ogre::Vector3(0,18,0);Gain(c);
 check(Near(ui.label.offset.y,18.4f),"tall model uses actual bone height");
 c.bones[0]=c.base;c.bones[1]=c.base+Ogre::Vector3(0,1.1f,0);int before=c.boneCalls;Gain(c);
 check(c.boneCalls-before==2&&Near(ui.label.offset.y,1.5f),"invalid first alias falls through to short model Head");
 c.boneFault[0]=true;c.boneFault[1]=true;c.bones[2]=c.base+Ogre::Vector3(.3f,3,0);before=c.boneCalls;Gain(c);
 check(c.boneCalls-before==3&&Near(ui.label.offset.y,3.4f),"native bone failures fall through to lowercase head under SEH");
 c.boneFault[0]=c.boneFault[1]=false;c.bones[0]=c.base+Ogre::Vector3(13,9,0);c.bones[1]=c.base+Ogre::Vector3(0,21,0);c.bones[2]=c.base;Gain(c);
 check(Same(ui.label.offset,Ogre::Vector3(0,2.6f,0)),"implausible or absent bones use bounded fallback");
 union {unsigned u;float f;} bad;bad.u=0x7f800000;c.bones[0]=c.base+Ogre::Vector3(0,bad.f,0);bad.u=0x7fc00001;c.bones[1]=c.base+Ogre::Vector3(bad.f,0,0);Gain(c);
 check(Same(ui.label.offset,Ogre::Vector3(0,2.6f,0)),"infinite and NaN bone coordinates never reach native label");
 c.positionFault=true;before=c.boneCalls;Gain(c);
 check(c.boneCalls==before&&Same(ui.label.offset,Ogre::Vector3(0,2.6f,0)),"failed base read uses fallback without querying bones");
 c.positionFault=false;int reads=c.boneCalls;c.stats.value+=.1f;Scan();
 check(c.boneCalls==reads,"fractional gain does not query bones");
 ui.failLabel=true;Gain(c);check(c.boneCalls==reads,"failed native label creation does not query bones");ui.failLabel=false;
 Ogre::Vector3 bound;
 check(SkillHeadAnchor::FromWorldHead(c.base,c.base+Ogre::Vector3(12,20,-12),bound)&&Near(bound.y,20.4f),"plausibility boundary accepts valid world head without clamping its height");
 check(!SkillHeadAnchor::FromWorldHead(c.base,c.base+Ogre::Vector3(12.01f,20,0),bound),"world head beyond plausibility boundary is rejected");
 Ogre::Vector3 invalidBase=c.base;bad.u=0x7fc00001;invalidBase.x=bad.f;
 check(!SkillHeadAnchor::FromWorldHead(invalidBase,c.bones[0],bound),"nonfinite base cannot produce an invalid relative offset");
 g_enabled=false;Gain(c);check(c.boneCalls==reads,"disabled notifications do not query bones");
 printf("head anchor regressions: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

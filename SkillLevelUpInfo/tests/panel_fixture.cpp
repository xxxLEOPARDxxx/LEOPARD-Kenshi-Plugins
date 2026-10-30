void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
int main(){
 setvbuf(stdout,NULL,_IONBF,0);
 InitializeCriticalSection(&g_lock);XpPanelTextStyle::g_nativeProfile=true;g_uiThread=GetCurrentThreadId();g_origPanel=NoopPanel;
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;
 Character c;player.playerCharacters.push_back(&c);Scan();c.stats.value+=.03f;Scan();
 DatapanelGUI panel;panel.Vanilla(10);panel.Vanilla(30);Panel(&c.stats,&panel,0,false);
 check(g_slots.count==2&&panel.named.size()==2,"recent training creates at most configured native rows");
 DataPanelLine_Progress*row=static_cast<DataPanelLine_Progress*>(panel.getLine(SlotKey(0),0));
 check(g_labels[0].get()->caption=="Strength"&&row->progressText->caption=="52%","panel preserves skill label and fractional percent separately");
 check(row->progress->getLeft()==198&&row->progress->getWidth()==192,"XP bar preserves native48percent width and aligns with vanilla value right edge");
 check(row->progress->getTop()>=48&&row->progress->getBottom()<=300,"XP rows stay below vanilla stats and inside panel");
 check(g_divider.get()&&g_divider.get()->getVisible()&&g_divider.get()->getTop()>=48&&g_divider.get()->getBottom()<=row->progress->getTop(),"divider stays in free space between vanilla stats and XP rows");
 int moves=row->progress->moves,updates=row->updates;Panel(&c.stats,&panel,0,false);
 check(row->progress->moves==moves&&row->updates==updates,"stable panel does not repeat geometry or progress writes");
 sdkBrokenEquality=true;unsigned abiResets=g_contextResets;
 check(ContextMask(&panel,g_panelRoot.get(),hand(&c),0)==0,"Native SDK broken virtual equality cannot invalidate stable owner context");
 for(int abi=0;abi<100;++abi)Panel(&c.stats,&panel,0,false);
 check(g_contextResets==abiResets,"100 stable frames avoid owner context resets with actual SDK equality trap");
 sdkBrokenEquality=false;
 g_bars=false;g_panelDirty=true;Panel(&c.stats,&panel,0,false);
 check(!row->progress->visible&&!g_divider.get()->visible,"bar toggle immediately hides rows and divider");
 g_bars=true;g_panelDirty=true;Panel(&c.stats,&panel,0,false);check(row->progress->visible,"bar toggle restores recent rows");
 fakeNow+=120001;Panel(&c.stats,&panel,0,false);check(!row->progress->visible&&!g_divider.get()->visible,"expired activity hides rows and divider");
 fakeNow=10000;panel.root.coord.height=60;Panel(&c.stats,&panel,0,false);
 check(g_slots.count==1&&row->progress->getBottom()<=60,"cramped panel reflows only a complete row inside its real viewport");
 panel.root.coord.height=300;
 for(int i=0;i<2;++i){panel.lines[i]->w1->coord=MyGUI::IntCoord(12,10+i*20,384,18);panel.lines[i]->w2->coord=MyGUI::IntCoord(12,10+i*20,372,18);}
 g_panelDirty=true;Panel(&c.stats,&panel,0,false);
 check(g_slots.count==2,"native full-row overlapping name/value text boxes still permit recent XP rows");
 check(row->progress->visible&&row->progress->getLeft()==202&&row->progress->getWidth()==192,
       "XP bar uses native48percent width and original value-right-edge placement instead of value-text left");
 // getLineByNum iterates the native key map, not the visual row order.
 // Full-row boxes below mimic native createMe; 46px after divider fits TWO20px rows.
 DatapanelGUI shuffled;shuffled.root.coord.height=160;
 const int tops[]={10,30,90,50,70};
 for(int i=0;i<5;++i){shuffled.Vanilla(tops[i]);DataPanelLine*l=shuffled.lines.back();
  l->w1->coord=MyGUI::IntCoord(12,tops[i],384,18);l->w2->coord=MyGUI::IntCoord(12,tops[i],372,18);}
 Panel(&c.stats,&shuffled,0,false);
 check(g_slots.count==2,"native map order cannot inflate20px visual pitch to60px and discard the second row");
 DataPanelLine_Progress*second=shuffled.lineExists(SlotKey(1),0)?static_cast<DataPanelLine_Progress*>(shuffled.getLine(SlotKey(1),0)):NULL;
 check(second&&second->progress->getBottom()<=160,"second recent skill fits actual native viewport after unordered vanilla rows");
 // Slow XP at level86 must activate a row before the1percent announcement threshold.
 Reset();player.playerCharacters.clear();Character runner;runner.stats.independent=true;runner.stats.firstRef=1000;
 runner.stats.refs[STAT_ATHLETICS]=86.911f;player.playerCharacters.push_back(&runner);Scan();
 runner.stats.refs[STAT_ATHLETICS]+=0.00000762939453125f;Scan();Panel(&runner.stats,&shuffled,0,false);
 Snapshot&runState=g_seen[hand(&runner)];
 check(runState.skills[21].active&&runState.skills[21].pending<.01f&&g_slots.count==1&&g_slots.ids[0]==21,
       "one float ULP of slow level86 Athletics activates the native panel independently of MinPercent");
 for(int i=0;i<300;++i){fakeNow+=16;Scan();Panel(&runner.stats,&shuffled,0,false);}
 check(g_slots.count==1&&static_cast<DataPanelLine_Progress*>(shuffled.getLine(SlotKey(0),0))->progress->visible,
       "slow-XP row survives300 unchanged refreshes instead of depending on another XP gain");
 // Two same-named characters have distinct hand identities and per-skill baselines.
 Character robotA,robotB;robotA.displayName=robotB.displayName="same robot";
 robotA.stats.independent=robotB.stats.independent=true;robotA.stats.firstRef=robotB.stats.firstRef=1000;
 player.playerCharacters.push_back(&robotA);player.playerCharacters.push_back(&robotB);Scan();
 robotA.stats.refs[STAT_DEXTERITY]=81.138f;robotA.stats.refs[STAT_MARTIALARTS]=66.031f;
 robotB.stats.refs[STAT_DEXTERITY]=81.117f;robotB.stats.refs[STAT_MARTIALARTS]=65.999f;Scan();
 Panel(&robotA.stats,&shuffled,0,false);
 check(g_slots.count==2,"first same-named robot retains two independently trained skills");
 Panel(&robotB.stats,&shuffled,0,false);
 check(g_slots.count==2&&g_seen[hand(&robotB)].skills[13].value==65.999f,
       "second same-named robot uses its own martial arts baseline");
 DatapanelGUI alternate;alternate.root.coord.height=160;
 for(int i=0;i<5;++i){alternate.Vanilla(tops[i]);DataPanelLine*l=alternate.lines.back();
  l->w1->coord=MyGUI::IntCoord(12,tops[i],384,18);l->w2->coord=MyGUI::IntCoord(12,tops[i],372,18);}
 bool bothStable=true;
 for(int i=0;i<50;++i){fakeNow+=16;Panel(&robotA.stats,&shuffled,0,false);if(g_slots.count!=2)bothStable=false;
  Panel(&robotB.stats,&alternate,0,false);if(g_slots.count!=2)bothStable=false;}
 check(bothStable,"alternating native panels and same-named characters cannot discard recent XP or the second row");
 // A human has six ordinary rows, a robot five. Same-height HUD must retain
 // available recent training without using map order as an accidental limit.
 shuffled.Vanilla(110);shuffled.lines.back()->w1->coord=MyGUI::IntCoord(12,110,384,18);
 shuffled.lines.back()->w2->coord=MyGUI::IntCoord(12,110,372,18);
 Panel(&runner.stats,&shuffled,0,false);
 check(g_slots.count==1&&g_slots.ids[0]==21,"six-row human layout still shows slow Athletics in its one remaining native row");
 MyGUI::Widget sourceDirect,destination;sourceDirect.text.fontName="NativeFace";sourceDirect.text.fontHeight=25;
 XpPanelTextStyle::Copy(&destination,XpPanelTextStyle::RenderedText(&sourceDirect));
 check(destination.text.fontName=="NativeFace"&&destination.text.fontHeight==25,"actual rendered source font face/height copied without constants");
 const int styleWrites=destination.text.fontWrites+destination.text.heightWrites;
 for(int n=0;n<100;++n)XpPanelTextStyle::Copy(&destination,XpPanelTextStyle::RenderedText(&sourceDirect));
 check(destination.text.fontWrites+destination.text.heightWrites==styleWrites,"100 stable font copies perform zero setter writes");
 MyGUI::Widget sourceClient,destClient;sourceDirect.client=&sourceClient;sourceDirect.textEnabled=false;destination.client=&destClient;
 sourceClient.text.fontName="CompositeNativeFace";sourceClient.text.fontHeight=25;
 XpPanelTextStyle::Copy(&destination,XpPanelTextStyle::RenderedText(&sourceDirect));
 check(destClient.text.fontName=="CompositeNativeFace"&&destClient.text.fontHeight==25,"composite EditBox follows actual client despite empty nonvirtual getters");
 sourceClient.text.hasFont=false;XpPanelTextStyle::Copy(&destination,XpPanelTextStyle::RenderedText(&sourceDirect));
 check(destClient.text.fontName=="CompositeNativeFace","missing source font preserves the last valid native font");
 MyGUI::Widget cycleA,cycleB;cycleA.client=&cycleB;cycleB.client=&cycleA;cycleA.textEnabled=cycleB.textEnabled=false;
 check(!XpPanelTextStyle::RenderedText(&cycleA)&&cycleA.clientReads+cycleB.clientReads<=4,
       "malformed composite-client cycle stops within bounded font lookup without child traversal");
 MyGUI::EditText absent;absent.nativeFont=NULL;absent.fontName="NonzeroNativeTextureDecoy";
 check(!XpPanelTextStyle::HasFont(&absent),"native0xe0 null-font guard rejects decoy data at incompatible SDK0xe8 offset");
 MyGUI::Widget padSource,padOther;padSource.coord.top=padOther.coord.top=109;
 padSource.text.fontHeight=padOther.text.fontHeight=25;padSource.text.measuredHeight=25;padOther.text.measuredHeight=26;
 padSource.text.caption="old";padOther.text.caption="new";padSource.getSubWidgetText();padOther.getSubWidgetText();
 XpPanelTextStyle::MeasureCache captionCache;
 check(XpPanelTextStyle::PaddingBefore(&padSource,134,captionCache),"initial25px rendered caption proves padding before134");
 padSource.text.caption="new";padSource.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padSource,134,captionCache),"changed rendered caption remeasures26px text instead of reusing25px padding proof");
 XpPanelTextStyle::MeasureCache identityCache;padSource.text.measuredHeight=25;
 XpPanelTextStyle::PaddingBefore(&padSource,134,identityCache);
 check(!XpPanelTextStyle::PaddingBefore(&padOther,134,identityCache),"another native text source cannot inherit the prior source padding proof");
 XpPanelTextStyle::MeasureCache shadowCache;padSource.text.shadow=false;padSource.text.measuredHeight=25;
 XpPanelTextStyle::PaddingBefore(&padSource,134,shadowCache);padSource.text.shadow=true;padSource.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padSource,134,shadowCache),"public shadow change invalidates padding using actual newly measured text size");
 XpPanelTextStyle::MeasureCache wrapCache;padSource.isEditBox=true;padSource.editWordWrap=false;padSource.text.measuredHeight=25;
 XpPanelTextStyle::PaddingBefore(&padSource,134,wrapCache);padSource.editWordWrap=true;padSource.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padSource,134,wrapCache),"public EditBox word-wrap change invalidates padding without guessing extra pixels");
 XpPanelTextStyle::MeasureCache unlinkCache;padSource.text.measuredHeight=25;
 XpPanelTextStyle::PaddingBefore(&padSource,134,unlinkCache);MyGUI::UnlinkWidget(&padSource);padSource.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padSource,134,unlinkCache),"native unlink invalidates cached source even if its address is immediately reused");
 MyGUI::Widget padOuter,padClient;padOuter.textEnabled=false;padOuter.client=&padClient;padClient.coord.top=109;
 padClient.text.fontHeight=25;padClient.text.measuredHeight=25;padClient.text.caption="same";
 XpPanelTextStyle::MeasureCache clientCache;XpPanelTextStyle::PaddingBefore(&padOuter,134,clientCache);
 MyGUI::UnlinkWidget(&padClient);padClient.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padOuter,134,clientCache),"rendered-client unlink invalidates proof while logical EditBox owner remains alive");
 padClient.isEditBox=true;padClient.editWordWrap=false;padClient.text.measuredHeight=25;
 XpPanelTextStyle::MeasureCache clientWrapCache;XpPanelTextStyle::PaddingBefore(&padOuter,134,clientWrapCache);
 padClient.editWordWrap=true;padClient.text.measuredHeight=26;
 check(!XpPanelTextStyle::PaddingBefore(&padOuter,134,clientWrapCache),"rendered EditBox wrap changes invalidate proof when logical wrapper has no wrap getter");
 XpPanelTextStyle::g_nativeProfile=false;
 check(!XpPanelTextStyle::RenderedText(&destination),"unknown MyGUI profile skips unsafe font reads without SEH");
 printf("native panel boundary: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

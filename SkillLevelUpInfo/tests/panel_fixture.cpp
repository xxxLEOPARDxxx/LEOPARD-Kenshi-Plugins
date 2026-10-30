void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
int main(){
 InitializeCriticalSection(&g_lock);XpPanelTextStyle::g_nativeProfile=true;g_uiThread=GetCurrentThreadId();g_origPanel=NoopPanel;
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;
 Character c;player.playerCharacters.push_back(&c);Scan();c.stats.value+=.03f;Scan();
 DatapanelGUI panel;panel.Vanilla(10);panel.Vanilla(30);Panel(&c.stats,&panel,0,false);
 check(g_slots.count==2&&panel.named.size()==2,"recent training creates at most configured native rows");
 DataPanelLine_Progress*row=static_cast<DataPanelLine_Progress*>(panel.getLine(SlotKey(0),0));
 check(row->textBox->caption=="Strength"&&row->progressText->caption=="52%","panel preserves skill label and fractional percent separately");
 check(row->progress->getLeft()==198&&row->progress->getWidth()==192,"XP bar preserves native48percent width and aligns with vanilla value right edge");
 check(row->progress->getTop()>=48&&row->progress->getBottom()<=300,"XP rows stay below vanilla stats and inside panel");
 check(g_divider.get()&&g_divider.get()->getVisible()&&g_divider.get()->getTop()>=48&&g_divider.get()->getBottom()<=row->progress->getTop(),"divider stays in free space between vanilla stats and XP rows");
 int moves=row->progress->moves,updates=row->updates;Panel(&c.stats,&panel,0,false);
 check(row->progress->moves==moves&&row->updates==updates,"stable panel does not repeat geometry or progress writes");
 g_bars=false;g_panelDirty=true;Panel(&c.stats,&panel,0,false);
 check(!row->progress->visible&&!g_divider.get()->visible,"bar toggle immediately hides rows and divider");
 g_bars=true;g_panelDirty=true;Panel(&c.stats,&panel,0,false);check(row->progress->visible,"bar toggle restores recent rows");
 fakeNow+=120001;Panel(&c.stats,&panel,0,false);check(!row->progress->visible&&!g_divider.get()->visible,"expired activity hides rows and divider");
 fakeNow=10000;panel.root.coord.height=60;Panel(&c.stats,&panel,0,false);
 check(g_slots.count==0&&!row->progress->visible,"cramped panel creates no overlapping rows");
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
 // Read-only PID13268 snapshot: native root/scroll parent320x185, rowheight22,
 // tops5/30/55 (map order30/5/55). Extend that actual25px grid to5/6 rows.
 DatapanelGUI combatPanel;combatPanel.root.coord=MyGUI::IntCoord(0,0,320,185);
 const int unarmedTops[]={30,5,55,80,105,130},armedTops[]={30,5,130,55,80,105};
 for(int i=0;i<6;++i){combatPanel.Vanilla(unarmedTops[i]);DataPanelLine*l=combatPanel.lines.back();
  l->w1->coord=MyGUI::IntCoord(9,unarmedTops[i],307,22);l->w2->coord=MyGUI::IntCoord(9,unarmedTops[i],307,22);}
 DataPanelLine*combatExtra=combatPanel.lines[5];combatExtra->setVisible(false);
 Panel(&robotA.stats,&combatPanel,0,false);bool armedStable=g_slots.count==2;
 combatExtra->setVisible(true);
 for(int i=0;i<6;++i){combatPanel.lines[i]->w1->coord.top=armedTops[i];combatPanel.lines[i]->w2->coord.top=armedTops[i];}
 Panel(&robotA.stats,&combatPanel,0,false);armedStable=armedStable&&g_slots.count==1;
 combatExtra->setVisible(false);
 for(int i=0;i<5;++i){combatPanel.lines[i]->w1->coord.top=unarmedTops[i];combatPanel.lines[i]->w2->coord.top=unarmedTops[i];}
 Panel(&robotA.stats,&combatPanel,0,false);armedStable=armedStable&&g_slots.count==2;
 check(armedStable&&g_seen[hand(&robotA)].skills[2].active&&g_seen[hand(&robotA)].skills[13].active,
       "same robot weapon mode5-to6-to5 vanilla rows adapts two-to-one-to-two XP rows without losing activity");
 g_maxRows=5;shuffled.root.coord.height=240;Panel(&robotA.stats,&shuffled,0,false);
 Snapshot&robotState=g_seen[hand(&robotA)];for(int i=0;i<5;++i){robotState.skills[i].Observe(1,fakeNow);robotState.skills[i].Observe(1.01f,fakeNow);}
 Panel(&robotA.stats,&shuffled,0,false);
 check(g_slots.count==5,"configured maximumfive renders five rows when native viewport has space");
 shuffled.root.coord.height=200;Panel(&robotA.stats,&shuffled,0,false);
 check(g_slots.count==3&&static_cast<DataPanelLine_Progress*>(shuffled.getLine(SlotKey(2),0))->progress->getBottom()<=200,
       "maximumfive adapts to three safe rows in a shorter viewport without changing font or overlapping HUD");
 // PID26720 measured actual185px parent,26px ordinary rows/pitch: six-row
 // bottom161 leaves EXACTLY24px, five-row bottom135 leaves EXACTLY50px.
 DatapanelGUI measured;measured.root.coord=MyGUI::IntCoord(0,0,320,185);
 const int measuredTops[]={5,31,83,57,109,135};
 for(int i=0;i<6;++i){measured.Vanilla(measuredTops[i]);DataPanelLine*l=measured.lines.back();
  l->w1->coord=MyGUI::IntCoord(9,measuredTops[i],307,26);l->w2->coord=MyGUI::IntCoord(9,measuredTops[i],307,26);}
 Panel(&robotA.stats,&measured,0,false);
 check(g_slots.count==1,"actual185px viewport fits one24px XP row after six vanilla rows ending161");
 DataPanelLine_Progress*exactFirst=measured.lineExists(SlotKey(0),0)?static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(0),0)):NULL;
 check(exactFirst&&exactFirst->progress->getTop()==161&&exactFirst->progress->getHeight()==24&&exactFirst->progress->getBottom()==185,
       "single measured XP row ends exactly at185 with unchanged24px widget height");
 check(g_divider.get()&&g_divider.get()->visible&&g_divider.get()->getTop()==160&&g_divider.get()->getBottom()<=161,
       "exact-fit six-row panel retains a1px divider inside safe native text padding at160");
 measured.lines[5]->setVisible(false);Panel(&robotA.stats,&measured,0,false);
 check(g_slots.count==2,"actual185px viewport fits two24px XP rows on26px pitch after five rows ending135");
 DataPanelLine_Progress*exactSecond=measured.lineExists(SlotKey(1),0)?static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(1),0)):NULL;
 check(exactFirst&&exactSecond&&exactFirst->progress->getTop()==135&&exactSecond->progress->getTop()==161&&exactSecond->progress->getBottom()==185,
       "second measured XP row ends exactly at185 without overlap or trailing-pitch padding");
 check(g_divider.get()&&g_divider.get()->visible&&g_divider.get()->getTop()==134&&g_divider.get()->getBottom()<=135,
       "exact-fit five-row panel retains its1px divider at134 without stealing XP row capacity");
 bool measuredStable=true;
 for(int frame=0;frame<100;++frame){measured.lines[5]->setVisible(frame%2!=0);Panel(&robotA.stats,&measured,0,false);
  if(g_slots.count!=(frame%2?1:2))measuredStable=false;}
 check(measuredStable,"actual measured five-six-five row transitions remain stable over100 refreshes");
 bool bounded=true;
 for(int maximum=1;maximum<=5;++maximum){g_maxRows=maximum;
  for(int fit=0;fit<=5;++fit){measured.lines[5]->setVisible(true);measured.root.coord.height=161+(fit?24+(fit-1)*26:23);
   Panel(&robotA.stats,&measured,0,false);const int expected=fit<maximum?fit:maximum;
   if(g_slots.count!=expected)bounded=false;
   for(int n=0;n<g_slots.count;++n){DataPanelLine_Progress*l=static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(n),0));
    if(!l||l->progress->getTop()<161||l->progress->getHeight()!=24||l->progress->getBottom()>measured.root.getHeight())bounded=false;
    if(n){DataPanelLine_Progress*before=static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(n-1),0));if(before->progress->getBottom()>l->progress->getTop())bounded=false;}}
  }
 }
 check(bounded,"every configured maximum1..5 honors exact-fit capacity and nonoverlap including one-pixel-too-short viewport");
 g_maxRows=3;measured.lines[5]->setVisible(false);measured.root.coord.height=187;
 Panel(&runner.stats,&measured,0,false);
 DataPanelLine_Progress*stableFirst=static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(0),0));
 const int reservedTop=stableFirst->progress->getTop();runner.stats.refs[STAT_DEXTERITY]+=.0001f;Scan();
 Panel(&runner.stats,&measured,0,false);
 check(g_slots.count==2&&reservedTop==137&&stableFirst->progress->getTop()==reservedTop&&g_divider.get()&&g_divider.get()->visible,
       "conditional divider reserves gap by allowed capacity so a second active skill never moves the first row");
 for(int i=0;i<6;++i){measured.lines[i]->w1->text.fontName="ActualVanillaBold";measured.lines[i]->w1->text.fontHeight=25;
  measured.lines[i]->w2->text.fontName="ActualVanillaValue";measured.lines[i]->w2->text.fontHeight=23;}
 Panel(&runner.stats,&measured,0,false);
 check(stableFirst->textBox->text.fontName=="ActualVanillaBold"&&stableFirst->textBox->text.fontHeight==25,
       "XP skill names copy direct rendered vanilla font face and effective height");
 check(stableFirst->progressText->text.fontName=="ActualVanillaValue"&&stableFirst->progressText->text.fontHeight==23,
       "XP percentages copy rendered vanilla value style independently from skill labels");
 const int styleWrites=stableFirst->textBox->text.fontWrites+stableFirst->textBox->text.heightWrites+
  stableFirst->progressText->text.fontWrites+stableFirst->progressText->text.heightWrites;
 const int styleMoves=stableFirst->progress->moves;const MyGUI::IntCoord styledCoord=stableFirst->progress->getCoord();
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(styleWrites==stableFirst->textBox->text.fontWrites+stableFirst->textBox->text.heightWrites+
  stableFirst->progressText->text.fontWrites+stableFirst->progressText->text.heightWrites&&styleMoves==stableFirst->progress->moves,
       "100 stable font refreshes perform zero extra font setters or geometry writes");
 MyGUI::Widget sourceClient,sourceDeep,destClient,destValue;
 sourceClient.client=&sourceDeep;sourceClient.textEnabled=false;
 sourceDeep.text.fontName="CompositeVanillaBold";sourceDeep.text.fontHeight=25;
 for(int i=0;i<6;++i){measured.lines[i]->w1->client=&sourceClient;measured.lines[i]->w1->textEnabled=false;}
 stableFirst->textBox->client=&destClient;stableFirst->progressText->client=&destValue;
 Panel(&runner.stats,&measured,0,false);
 check(destClient.text.fontName=="CompositeVanillaBold"&&destClient.text.fontHeight==25&&
       destValue.text.fontName=="ActualVanillaValue"&&destValue.text.fontHeight==23,
       "composite EditBox styles follow rendered client text despite empty nonvirtual TextBox getters");
 check(!(stableFirst->progress->getCoord()!=styledCoord)&&stableFirst->progress->getHeight()==24&&g_slots.count==2,
       "copying25px vanilla font preserves accepted24px XP rectangles and two-row capacity");
 const std::string kept=destClient.text.fontName;sourceDeep.text.hasFont=false;
 for(int i=0;i<6;++i)measured.lines[i]->w1->textEnabled=false;
 Panel(&runner.stats,&measured,0,false);
 check(destClient.text.fontName==kept,"missing actual source font never clears or invents XP label font");
 for(int i=0;i<6;++i){measured.lines[i]->w1->client=NULL;measured.lines[i]->w1->textEnabled=true;measured.lines[i]->w1->text.hasFont=true;
  measured.lines[i]->w1->text.fontHeight=25;measured.lines[i]->w2->text.fontHeight=21;}
 measured.root.coord.height=185;Panel(&runner.stats,&measured,0,false);
 MyGUI::ISubWidgetText&lastName=measured.lines[4]->w1->text;MyGUI::ISubWidgetText&lastValue=measured.lines[4]->w2->text;
 check(g_slots.count==2&&g_divider.get()&&g_divider.get()->visible&&g_divider.get()->getTop()==134&&
       lastName.absolute.top+(26-25)/2+25<=g_divider.get()->getTop()&&lastValue.absolute.top+(26-21)/2+21<=g_divider.get()->getTop(),
       "measured25px name and21px value glyph cells leave safe1px divider padding in26px vanilla row");
 const int measurements=lastName.measures+lastValue.measures;
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(lastName.measures+lastValue.measures==measurements,"100 stable padding checks perform zero extra native text-size measurements");
 lastName.fontHeight=26;Panel(&runner.stats,&measured,0,false);
 check(g_slots.count==2&&(!g_divider.get()||!g_divider.get()->visible),"unverified full-height glyph cell hides exact-fit divider without losing either XP row");
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
 measured.lines[5]->setVisible(false);measured.root.coord.height=185;g_maxRows=2;lastName.fontHeight=25;
 exactFirst->progressText->client=NULL;exactSecond->progressText->client=NULL;
 Panel(&runner.stats,&measured,0,false);
 MyGUI::Widget*bg=exactSecond->progress->skinChildren[0];MyGUI::Widget*track=exactSecond->progress->skinChildren[1];
 check(g_slots.count==2&&bg->getTop()==4&&bg->getHeight()==19&&track->getTop()==5&&track->getHeight()==17,
       "native stretch-skin background and fill track move down2px without shrinking19px or17px physical height");
 check(exactSecond->progress->getTop()==161&&exactSecond->progress->getHeight()==24&&exactSecond->textBox->getTop()==161&&exactSecond->textBox->getHeight()==24,
       "visual nudge preserves logical24px progress row and accepted name rectangle");
 check(exactSecond->progressText->getTop()==163&&exactSecond->progressText->getHeight()==22&&exactSecond->progressText->getBottom()==185,
       "21px top-aligned percentage moves down2px within22px box ending at185");
 const int visualWrites=bg->moves+track->moves+exactSecond->progressText->moves;
 const int skinQueries=exactSecond->progress->skinQueries;
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(visualWrites==bg->moves+track->moves+exactSecond->progressText->moves&&track->getTop()==5,
       "100 stable visual-nudge refreshes neither accumulate offset nor repeat child geometry writes");
 check(skinQueries==2&&exactSecond->progress->skinQueries==skinQueries&&exactSecond->progress->getChildCount()==0,
       "actual skin children are resolved independently of empty content children and never searched again on stable frames");
 const int percentMeasures=exactSecond->progressText->text.measures;
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(exactSecond->progressText->text.measures==percentMeasures,"stable percentage nudge reuses existing caption/style measurement cache");
 MyGUI::ProgressBar unknown;MyGUI::Widget unknownBg,unknownTrack;
 unknown.coord=MyGUI::IntCoord(0,0,153,24);unknownBg.coord=MyGUI::IntCoord(10,2,134,20);unknownTrack.coord=MyGUI::IntCoord(11,3,132,17);
 unknownBg.parent=&unknown;unknownTrack.parent=&unknown;unknownTrack.skinName="TrackPlace";
 unknown.skinChildren.push_back(&unknownBg);unknown.skinChildren.push_back(&unknownTrack);BarVisualCache unknownCache;LowerProgressVisual(&unknown,unknownCache);
 check(unknownBg.getTop()==2&&unknownTrack.getTop()==3&&unknownBg.getHeight()==20,"unknown progress skin is left unchanged rather than resized or guessed");
 lastValue.fontHeight=25;Panel(&runner.stats,&measured,0,false);
 check(g_slots.count==2&&exactSecond->progressText->getTop()==161&&exactSecond->progressText->getHeight()==24,
       "percentage text too tall for22px keeps its original24px rectangle instead of clipping for the nudge");
 lastValue.fontHeight=22;exactSecond->progressText->text.shadow=true;Panel(&runner.stats,&measured,0,false);
 check(exactSecond->progressText->getTop()==161&&exactSecond->progressText->getHeight()==24,
       "shadowed percentage with no remaining padding safely retains its full24px text rectangle");
 lastValue.fontHeight=21;exactSecond->progressText->text.coord.top=0;Panel(&runner.stats,&measured,0,false);
 exactSecond->progressText->setCoord(MyGUI::IntCoord(165,161,153,24));
 XpPanelTextStyle::Measure(exactSecond->progressText,g_percentMeasure[1]);const int originMeasures=exactSecond->progressText->text.measures;
 exactSecond->progressText->text.coord.top=3;
 Panel(&runner.stats,&measured,0,false);
 check(exactSecond->progressText->getTop()==161&&exactSecond->progressText->getHeight()==24&&
       exactSecond->progressText->getSubWidgetText()->getAbsolutePosition().top+21==185&&track->getTop()==5,
       "actual3px rendered text inset retains24px percentage box and visible21px glyphs while the bar remains lowered");
 const int insetWrites=exactSecond->progressText->moves;
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(exactSecond->progressText->text.measures==originMeasures&&exactSecond->progressText->moves==insetWrites,
       "origin-only geometry changes invalidate nudge permission without stale cached fit or repeated measurements/writes");
 exactSecond->progressText->text.coord.top=0;exactSecond->progressText->text.coord.height=22;
 Panel(&runner.stats,&measured,0,false);const int bottomInsetWrites=exactSecond->progressText->moves;
 for(int n=0;n<100;++n)Panel(&runner.stats,&measured,0,false);
 check(exactSecond->progressText->getTop()==161&&exactSecond->progressText->getHeight()==24&&
       exactSecond->progressText->moves==bottomInsetWrites&&track->getTop()==5,
       "2px lower text padding retains full percentage box for100 frames without22-to24px geometry oscillation");
 exactSecond->progressText->text.coord.height=24;measured.lines[5]->setVisible(true);Panel(&runner.stats,&measured,0,false);
 bool nudgeSwitch=g_slots.count==1&&exactFirst->textBox->getTop()==161&&exactFirst->progressText->getTop()==163&&exactFirst->progressText->getBottom()==185;
 measured.lines[5]->setVisible(false);Panel(&runner.stats,&measured,0,false);
 check(nudgeSwitch&&g_slots.count==2&&exactSecond->progressText->getBottom()==185&&track->getHeight()==17,
       "nudge preserves both five-row and six-row capacities and the185px lower bound");
 XpPanelTextStyle::g_nativeProfile=false;
 check(!XpPanelTextStyle::RenderedText(stableFirst->textBox),"unknown MyGUI getter profile skips font access safely without SEH or default-style writes");
 printf("native panel boundary: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

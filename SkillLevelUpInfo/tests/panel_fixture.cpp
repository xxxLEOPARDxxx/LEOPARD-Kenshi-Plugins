void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
int main(){
 InitializeCriticalSection(&g_lock);g_uiThread=GetCurrentThreadId();g_origPanel=NoopPanel;
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
 check(!g_divider.get()||!g_divider.get()->visible,"divider hides when exact-fit rows leave no safe gap");
 measured.lines[5]->setVisible(false);Panel(&robotA.stats,&measured,0,false);
 check(g_slots.count==2,"actual185px viewport fits two24px XP rows on26px pitch after five rows ending135");
 DataPanelLine_Progress*exactSecond=measured.lineExists(SlotKey(1),0)?static_cast<DataPanelLine_Progress*>(measured.getLine(SlotKey(1),0)):NULL;
 check(exactFirst&&exactSecond&&exactFirst->progress->getTop()==135&&exactSecond->progress->getTop()==161&&exactSecond->progress->getBottom()==185,
       "second measured XP row ends exactly at185 without overlap or trailing-pitch padding");
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
 printf("native panel boundary: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

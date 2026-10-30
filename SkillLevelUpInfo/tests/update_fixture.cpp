void NoopUpdate(ForgottenGUI*){}
void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
void RealIncrease(float& n,float amount,float limit){n+=amount;if(n>limit)n=limit;}
Character*reusedCharacter=NULL;
void ReplaceRegistration(float& n,float,float){n=1.5f;++reusedCharacter->serial;Scan();n=2.5f;}
void RegisterUnknown(float& n,float,float){n=1.5f;ou->player->playerCharacters.push_back(reusedCharacter);Scan();n=2.5f;}
void RefreshRegistration(float& n,float amount,float){Scan();n+=amount;}
int main(){
 InitializeCriticalSection(&g_lock);g_origUpdate=NoopUpdate;g_origPanel=NoopPanel;g_origIncrease=RealIncrease;InterlockedExchange(&g_trackEnabled,1);
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;Character c;player.playerCharacters.push_back(&c);
 g_emit=100;fakeNow=10000;Update(&ui);fakeNow=10001;Increase(c.stats.value,.02f,100);fakeNow=10100;Update(&ui);
 check(labels==0,"fractional messages do not emit before their configured100ms collection time");
 fakeNow=10101;Update(&ui);check(labels>0,"100ms announcement interval emits before250ms baseline scan");
 Reset();labels=0;g_emit=60000;c.stats.value=1.5f;fakeNow=15000;Update(&ui);
 fakeNow=15001;Increase(c.stats.value,1,100);Update(&ui);check(labels>0&&lastGold,"level crossing emits next GUI update without a polling delay");
 Reset();labels=0;g_emit=1000;fakeNow=20000;Update(&ui);Increase(c.stats.value,.02f,100);fakeNow=23000;Update(&ui);
 check(labels>0,"pending XP survives a3-second GUI gap");check(g_seen.begin()->second.skills[0].active,"120-second recent activity survives a3-second GUI gap");
 Reset();fakeNow=30000;Scan();c.stats.value+=.03f;Scan();DatapanelGUI panel;MyGUI::Widget viewport;viewport.coord.height=75;panel.Vanilla(10);panel.Vanilla(30);
 for(size_t i=0;i<panel.lines.size();++i)panel.lines[i]->w1->parent=panel.lines[i]->w2->parent=&viewport;
 Panel(&c.stats,&panel,0,false);
 check(g_slots.count==2,"nested75px viewport bounds the reversible native reflow independently of300px outer window");
 if(g_slots.count){DataPanelLine_Progress*row=static_cast<DataPanelLine_Progress*>(panel.getLine(SlotKey(g_slots.count-1),0));
  check(row->progress->getBottom()<=viewport.getHeight(),"XP rows remain inside their actual parent viewport");}
 Reset();c.stats.value=1.5f;Scan();reusedCharacter=&c;g_origIncrease=ReplaceRegistration;Increase(c.stats.value,1,100);
 check(g_seen.begin()->second.skills[0].pending==0,"old in-flight hook cannot target same-generation replacement registration");
 Reset();player.playerCharacters.clear();c.stats.value=1;g_origIncrease=RegisterUnknown;Increase(c.stats.value,1,100);
 check(g_seen.begin()->second.skills[0].pending==0,"unregistered old call cannot target registration created during original function");
 g_origIncrease=RealIncrease;const size_t previousLabels=labels;fakeNow+=1;Increase(c.stats.value,.02f,100);Scan();fakeNow+=1000;Update(&ui);
 check(labels>previousLabels,"unchanged registration remains active across routine scan refreshes");
 Reset();c.stats.value=1.5f;Scan();g_origIncrease=RefreshRegistration;Increase(c.stats.value,.02f,100);
 check(g_seen.begin()->second.skills[0].pending>.019f,"unchanged binding refreshed inside original call retains its pre-call token");
 Reset();g_origIncrease=RealIncrease;g_emit=100;c.stats.value=1.05f;fakeNow=40000;Update(&ui);
 bool tenHz=true;for(int i=0;i<10;++i){const size_t prior=labels;fakeNow=40001+i*100;Increase(c.stats.value,.02f,100);fakeNow+=100;Update(&ui);if(labels<=prior || lastGold)tenHz=false;}
 check(tenHz,"configured100ms interval sustains ten fractional deliveries in one second");
 printf("GUI timing and registration regressions: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

void OriginalIncrease(float& n,float amount,float limit){n+=amount;if(n>limit)n=limit;}
void OriginalAcrossLoad(float& n,float,float){
 InterlockedIncrement(&g_loadGeneration);n=1.5f;Reset();g_seenGeneration=g_loadGeneration;Scan();n=2.5f;
}
int main(){
 InitializeCriticalSection(&g_lock);InterlockedExchange(&g_trackEnabled,1);g_origIncrease=OriginalIncrease;
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;
 Character c;player.playerCharacters.push_back(&c);Scan();
 check(labels==0,"first native scan establishes silent baseline");
 for(size_t j=0;j<kCount;++j)c.stats.getStatRef(kStats[j])+=.03f;
 Scan();check(labels==0,"fractional native gain waits for interval");
 fakeNow+=1000;Scan();check(labels==1&&!lastGold&&lastCaption.find("+3% ")==0,"fractional native scan emits original blue small rising label");
 check(lastCaption.find("+33 more")!=std::string::npos,"more than four simultaneous skills use original remainder summary");
 size_t before=labels;Scan();check(labels==before,"repeated scan does not emit XP twice");
 g_floating=false;c.stats.value+=.1f;Scan();fakeNow+=1000;Scan();check(labels==before,"XP toggle suppresses fractional labels");
 g_levels=false;c.stats.value+=1;Scan();check(labels==before,"level toggle independently suppresses levels");
 g_levels=true;c.stats.value+=1;Scan();check(labels==before+1&&lastGold,"gold level messages remain independent of gain toggle");
 g_selectedOnly=true;player.selected=false;before=labels;c.stats.value+=1;Scan();player.selected=true;Scan();
 check(labels==before,"unselected gains are consumed without replay on later selection");
 g_floating=true;Reset();Scan();float unknown=5;Increase(unknown,1,100);fakeNow+=1000;Scan();
 check(labels==before,"unrelated increaseStat calls never become player XP");
 Increase(c.stats.value,.03f,100);Scan();fakeNow+=1000;Scan();
 check(labels==before+1&&!lastGold,"real indexed increaseStat and polling fallback emit one gain");
 before=labels;Scan();check(labels==before,"hook and fallback do not double announce");
 InterlockedIncrement(&g_loadGeneration);Increase(c.stats.value,1,100);Reset();g_seenGeneration=g_loadGeneration;Scan();
 check(labels==before,"save generation discards pre-load references and gains");
 c.stats.value=1;Reset();Scan();g_origIncrease=OriginalAcrossLoad;Increase(c.stats.value,1,100);
 check(g_seen.begin()->second.skills[0].pending==0,"worker call spanning load cannot target a reused new-world stat reference");
 c.stats.alias[STAT_HIVEMEDIC]=STAT_MEDIC;c.stats.alias[STAT_VET]=STAT_MEDIC;c.stats.unsupported=STAT_SURVIVAL;Reset();Scan();
 check(g_index.size()==34,"full original enum coverage deduplicates medic aliases and excludes native dummy reference");
 check(g_index.find(&c.stats.getStatRef(STAT_SURVIVAL))==g_index.end(),"global unsupported stat reference is never indexed");
 check(g_index.find(&c.stats.getStatRef(STAT_MASSCOMBAT))!=g_index.end(),"native mass-combat field omitted by old visible-only scope is now tracked");
 g_origIncrease=OriginalIncrease;c.stats.value=69.6f;
 c.stats.displayMultiplier=.97f;Reset();Scan();before=labels;
 Increase(c.stats.value,.01f,100);ProcessNotifications();
 check(labels==before,"fractional raw XP at an age-scaled display level is not a gold level-up");
 Reset();c.stats.displayMultiplier=.97f;Scan();before=labels;
 for(int i=0;i<10;++i){c.stats.displayMultiplier=i%2?.97f:1.f;fakeNow+=250;Scan();}
 check(labels==before,"temporary display-level crossings with no raw XP stay silent");
 c.stats.value=69.99f;Reset();Scan();before=labels;
 Increase(c.stats.value,.02f,100);ProcessNotifications();Scan();
 check(labels==before+1&&lastGold&&lastCaption=="Strength 70","a true raw whole level emits once under an age multiplier");
 CharacterStatsWindow::nativeNameIndex=STAT_MASSCOMBAT;translateMassCombat=true;
 Increase(c.stats.getStatRef(STAT_MASSCOMBAT),1,100);ProcessNotifications();
 check(lastGold&&lastCaption=="Localized mass combat 1","hidden Mass combat uses the plugin's active locale over the native English name");
 CharacterStatsWindow::nativeNameIndex=-1;translateMassCombat=false;
 c.stats.refs[STAT_TOUGHNESS]=81.25f;c.stats.alias[STAT_STRENGTH]=STAT_TOUGHNESS;before=labels;
 Scan();
 check(labels==before&&g_seen[hand(&c)].skills[0].value==81.25f,"rebound stat storage establishes a silent baseline");
 player.playerCharacters.clear();Scan();check(g_index.empty()&&g_seen.empty(),"departed characters remove every indexed native reference");
 g_enabled=false;before=labels;Scan();check(labels==before,"disabled module creates no labels");
 printf("native XP boundary: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

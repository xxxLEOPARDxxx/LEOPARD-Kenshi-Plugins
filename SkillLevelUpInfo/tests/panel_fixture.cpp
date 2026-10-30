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
 check(row->progress->getTop()>48&&row->progress->getBottom()<=300,"XP rows stay below vanilla stats and inside panel");
 check(g_divider.get()&&g_divider.get()->getVisible()&&g_divider.get()->getTop()==50,"divider separates vanilla stats and XP rows");
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
 printf("native panel boundary: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

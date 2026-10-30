void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
void NativeRebuild(CharStats*,DatapanelGUI*panel,int,bool){
 for(size_t i=panel->lines.size();i>0;--i){
  DataPanelLine*base=panel->lines[i-1];if(base->getType()!=DataPanelLine::DPL_PROGRESS)continue;
  DataPanelLine_Progress*r=static_cast<DataPanelLine_Progress*>(base);
  MyGUI::Gui::getInstance().destroyWidget(r->textBox);MyGUI::Gui::getInstance().destroyWidget(r->progress);MyGUI::Gui::getInstance().destroyWidget(r->progressText);
  delete r;panel->lines.erase(panel->lines.begin()+i-1);
 }panel->named.clear();
}
void UnderlayRow(DatapanelGUI&panel,int y){
 DataPanelLine*r=new DataPanelLine;
 r->w1=panel.root.createWidgetT("EditBox",r->skinW1,MyGUI::IntCoord(8,y,panel.root.getWidth()-12,25),0);
 r->w2=panel.root.createWidgetT("EditBox",r->skinW2,MyGUI::IntCoord(8,y,panel.root.getWidth()-12,25),0);
 r->w1->setCaption("base:");r->w2->setCaption("20");XpPanelTextStyle::RenderedText(r->w1)->fontHeight=25;XpPanelTextStyle::RenderedText(r->w2)->fontHeight=21;panel.lines.push_back(r);
}
int main(){
 InitializeCriticalSection(&g_lock);XpPanelTextStyle::g_nativeProfile=true;g_uiThread=GetCurrentThreadId();g_origPanel=NoopPanel;MyGUI::vanillaUiProfile=true;
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;Character c;player.playerCharacters.push_back(&c);Scan();c.stats.value+=.03f;Scan();
 MyGUI::ResourceSkin stock;stock.texture="Kenshi_UI.png";stock.basis.push_back(MyGUI::SubWidgetInfo());MyGUI::SubSkinStateInfo normal;
 normal.rect=MyGUI::FloatRect(0,907.f/1080,240.f/1920,1072.f/1080);stock.states["normal"].push_back(&normal);MyGUI::ResourceManager::getInstancePtr()->skin=&stock;
 MyGUI::Widget bg;bg.coord=MyGUI::IntCoord(0,1228,324,219);bg.textureName="Kenshi_UI.png";MyGUI::SubSkin main;main.coord=MyGUI::IntCoord(0,0,324,219);bg.mainSkin=&main;
 DatapanelGUI panel;panel.root.coord=MyGUI::IntCoord(10,19,297,181);panel.root.parent=&bg;
 for(int n=0;n<5;++n)UnderlayRow(panel,5+n*25);
 Panel(&c.stats,&panel,0,false);DataPanelLine_Progress*bar=static_cast<DataPanelLine_Progress*>(panel.getLine(SlotKey(0),0));
 check(g_slots.count==2&&bar->progress->getAbsoluteLeft()==203&&bar->progress->getWidth()==100&&bar->progress->getBottom()<=181,
       "actual324px background stock UV maps value-backed bar to absolute203..303 without changing two-row height/capacity");
 check(g_labels[0].get()->getWidth()>=150&&g_labels[1].get()->getWidth()>=150,
       "mapped value column leaves the measured150px Martial arts caption fully inside the native name column");
 MyGUI::Widget*const firstLabel=g_labels[0].get();MyGUI::Widget*const firstDivider=g_divider.get();
 UnderlayRow(panel,130);DataPanelLine*sixth=panel.lines.back();Panel(&c.stats,&panel,0,false);
 check(g_slots.count==1&&bar->progress->getAbsoluteLeft()==203&&bar->progress->getBottom()<=181&&
       g_labels[0].get()==firstLabel&&!g_labels[1].get()->getVisible()&&g_divider.get()==firstDivider,
       "actual stock six-row weapon mode retains one complete value-backed XP row and the same native label/divider");
 MyGUI::Gui::getInstance().destroyWidget(sixth->w1);MyGUI::Gui::getInstance().destroyWidget(sixth->w2);
 panel.lines.erase(std::find(panel.lines.begin(),panel.lines.end(),sixth));delete sixth;Panel(&c.stats,&panel,0,false);
 check(g_slots.count==2&&g_labels[0].get()==firstLabel&&g_labels[1].get()->getVisible()&&g_divider.get()==firstDivider,
       "returning stock six-to-five attributes restores two rows without divider/label reallocation");
 const int reads=MyGUI::ResourceManager::getInstancePtr()->reads,creates=MyGUI::widgetCreates;
 for(int n=0;n<100;++n)Panel(&c.stats,&panel,0,false);
 check(MyGUI::ResourceManager::getInstancePtr()->reads==reads&&MyGUI::widgetCreates==creates,
       "stable background binding performs no repeated resource lookups or widget creation");
 const int typeReads=g_labelTypeReads;const int ownCreates=g_labelCreates,ownDestroys=g_labelDestroys;
 for(int n=0;n<20;++n){
  DataPanelLine*r=panel.lines[4];const MyGUI::IntCoord coord=r->w1->getCoord();MyGUI::Gui::getInstance().destroyWidget(r->w1);
  r->w1=panel.root.createWidgetT("EditBox",r->skinW1,coord,0);XpPanelTextStyle::RenderedText(r->w1)->fontHeight=25;
  Panel(&c.stats,&panel,0,false);
 }
 check(g_labelTypeReads-typeReads==20&&g_labelCreates==ownCreates&&g_labelDestroys==ownDestroys,
       "native source churn validates one shared source type per new binding without rebuilding either owned label");
 bg.coord.width=480;main.coord.width=480;panel.root.coord.width=453;
 for(int n=0;n<5;++n){MyGUI::IntCoord a=panel.lines[n]->w1->coord,b=panel.lines[n]->w2->coord;a.width=b.width=441;panel.lines[n]->w1->setCoord(a);panel.lines[n]->w2->setCoord(b);}
 Panel(&c.stats,&panel,0,false);
 check(bar->progress->getAbsoluteLeft()==300&&bar->progress->getLeft()+bar->progress->getWidth()==449,
       "different actual render width uses live affine mapping rather than a203px or parent-percent position constant");
 // Another active UV profile (including the Dark277px atlas) must retain ca0 placement.
 normal.rect.right=277.f/1920;MyGUI::SubSkin darkMain;darkMain.coord=main.coord;bg.mainSkin=&darkMain;
 Panel(&c.stats,&panel,0,false);
 const int legacyWidth=(int)(453*.48f),legacyRight=451;
 check(bar->progress->getLeft()==legacyRight-legacyWidth&&bar->progress->getWidth()==legacyWidth,
       "Dark/unknown atlas UV profile keeps existing native width and right anchor without palette or skin changes");
 bg.textureName="Other_UI.png";MyGUI::SubSkin otherMain;otherMain.coord=main.coord;bg.mainSkin=&otherMain;normal.rect.right=240.f/1920;
 Panel(&c.stats,&panel,0,false);check(bar->progress->getLeft()==legacyRight-legacyWidth,
       "unknown backing texture has a conservative ca0 fallback");
 bg.textureName="Kenshi_UI.png";MyGUI::SubSkin custom;custom.coord=main.coord;bg.mainSkin=&custom;
 custom.setNativeUv(MyGUI::FloatRect(0,.1f,.125f,.3f));Panel(&c.stats,&panel,0,false);
 check(bar->progress->getLeft()==legacyRight-legacyWidth,
       "a custom actual quad UV cannot borrow the separately registered stock resource profile on the same texture");
 custom.setNativeUv(normal.rect);Panel(&c.stats,&panel,0,false);
 check(bar->progress->getAbsoluteLeft()==300,"same-pointer actual native state restoration safely enables its matching stock UV");
 custom.setNativeUv(MyGUI::FloatRect(0,.1f,.125f,.3f));Panel(&c.stats,&panel,0,false);
 check(bar->progress->getLeft()==legacyRight-legacyWidth,"same-pointer native state change invalidates the applied mapping without stale profile reuse");
 MyGUI::ISubWidgetRect unknownMain;unknownMain.coord=main.coord;bg.mainSkin=&unknownMain;Panel(&c.stats,&panel,0,false);
 check(!g_underlay.applied&&bar->progress->getLeft()==legacyRight-legacyWidth,"unknown actual quad RTTI retains fallback without reading a SubSkin-only member");
 check(g_panelTraceHead==0,"Debug-disabled production path performs no diagnostic native lookups or ring writes");
 PluginDiagnostics::enabled=true;g_origPanel=NativeRebuild;
 const unsigned int nativeCreates=g_progressCreates,dividerCreates=g_dividerCreates;
 Panel(&c.stats,&panel,0,false);const PanelTrace&t=g_panelTrace[(g_panelTraceHead-1)%32];
 check(t.before[0]&&t.before[1]&&t.after[0]==0&&t.after[1]==0&&t.contextMask==0&&
       t.dividerBefore==t.dividerNative&&t.dividerNative==t.dividerAfter&&g_progressCreates-nativeCreates==2&&g_dividerCreates==dividerCreates,
       "bounded Debug ring separates original native progress removal from plugin context/divider recreation");
 const unsigned int traced=g_panelTraceHead;PluginDiagnostics::enabled=false;Panel(&c.stats,&panel,0,false);
 check(g_panelTraceHead==traced,"disabling Debug stops detailed tracing without changing native lifecycle handling");
 g_origPanel=NoopPanel;Character npc,quiet;npc.playerControlled=false;player.playerCharacters.push_back(&quiet);Scan();
 DatapanelGUI npcPanel,quietPanel;npcPanel.Vanilla(5);quietPanel.Vanilla(5);
 const unsigned int dividerStable=g_dividerCreates,destroyStable=g_dividerParentDestroys;
 MyGUI::Widget*const retainedDivider=g_divider.get();
 for(int n=0;n<100;++n){Panel(&npc.stats,&npcPanel,0,false);Panel(&c.stats,&panel,0,false);}
 check(g_divider.get()==retainedDivider&&g_dividerCreates==dividerStable&&g_dividerParentDestroys==destroyStable,
       "100 NPC panel interleaves retain the live same-parent divider without destroy/create churn");
 for(int n=0;n<100;++n){Panel(&quiet.stats,&quietPanel,0,false);Panel(&c.stats,&panel,0,false);}
 check(g_divider.get()==retainedDivider&&g_dividerCreates==dividerStable&&g_dividerParentDestroys==destroyStable&&g_divider.get()->getVisible(),
       "100 quiet-player panel interleaves also retain the same-parent divider and preserve return visibility");
 DatapanelGUI other;for(int n=0;n<5;++n)other.Vanilla(5+n*18);Panel(&c.stats,&other,0,false);
 check(g_divider.get()&&g_divider.get()->getParent()==&other.root&&g_dividerParentDestroys==destroyStable+1&&g_dividerCreates==dividerStable+1,
       "a genuinely different active parent retires the old divider exactly once before creating its replacement");
 printf("native underlay layout: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

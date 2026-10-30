#include "ui_profile_constants.h"
void NoopPanel(CharStats*,DatapanelGUI*,int,bool){}
MyGUI::Widget* VisibleLabel(DatapanelGUI& panel,DataPanelLine_Progress* line,int slot){
 for(size_t n=0;n<panel.root.children.size();++n){MyGUI::Widget*w=panel.root.children[n];
  if(w->visible && w->caption==Name(g_slots.ids[slot]))return w;}
 return line->textBox;
}
void SourceRow(DatapanelGUI& panel,int top,int height){
 DataPanelLine*r=new DataPanelLine;r->keyValue="native";
 r->w1=panel.root.createWidgetT("EditBox",r->skinW1,MyGUI::IntCoord(8,top,panel.root.getWidth()-12,height),0);
 r->w2=panel.root.createWidgetT("EditBox",r->skinW2,MyGUI::IntCoord(8,top,panel.root.getWidth()-12,height),0);
 r->w1->setCaption("base:");r->w2->setCaption("20");
 MyGUI::ISubWidgetText*a=XpPanelTextStyle::RenderedText(r->w1),*b=XpPanelTextStyle::RenderedText(r->w2);
 a->fontName="Kenshi_PaintedTextFont_Medium";a->fontHeight=25;b->fontHeight=21;
 panel.lines.push_back(r);
}
bool BaseRestored(DatapanelGUI& panel,const UiProfile&p){
 for(int n=0;n<p.count;++n)if(panel.lines[n]->w1->coord.top!=p.tops[n] || panel.lines[n]->w2->coord.top!=p.tops[n] ||
  panel.lines[n]->w1->coord.height!=p.rowHeight)return false;return true;
}
int GeometryWrites(DatapanelGUI& panel){
 int n=0;for(size_t i=0;i<panel.lines.size();++i){DataPanelLine*l=panel.lines[i];if(l->type==DataPanelLine::DPL_BASE)n+=l->w1->moves+l->w2->moves;
  else{DataPanelLine_Progress*p=static_cast<DataPanelLine_Progress*>(l);n+=p->textBox->moves+p->progress->moves+p->progressText->moves;}}
 for(size_t i=0;i<panel.root.children.size();++i)n+=panel.root.children[i]->moves;return n;
}
int main(){
 InitializeCriticalSection(&g_lock);XpPanelTextStyle::g_nativeProfile=true;g_uiThread=GetCurrentThreadId();g_origPanel=NoopPanel;
 PlayerInterface player;GameWorld world;world.player=&player;ou=&world;ForgottenGUI ui;gui=&ui;
 Character c;player.playerCharacters.push_back(&c);Scan();c.stats.value+=.03f;Scan();
 for(size_t profile=0;profile<sizeof(profiles)/sizeof(profiles[0]);++profile){
  const UiProfile&p=profiles[profile];printf("PROFILE %s\n",p.name);MyGUI::vanillaUiProfile=p.stock;
  DatapanelGUI&panel=*new DatapanelGUI;panel.root.coord=MyGUI::IntCoord(0,0,p.width,p.height);
  for(int n=0;n<p.count;++n)SourceRow(panel,p.tops[n],p.rowHeight);
  g_bars=true;g_maxRows=2;g_panelDirty=true;fakeNow+=100;c.stats.value+=.01f;Scan();Panel(&c.stats,&panel,0,false);
  check(g_slots.count==p.capacity,"measured5/6-row UI retains its expected two/one XP capacity");
  bool readable=true,skin=true,frame=true,bounds=true,focus=true,source=true;int baseBottom=0;
  for(int n=0;n<p.count;++n){MyGUI::Widget*a=panel.lines[n]->w1,*b=panel.lines[n]->w2;
   source=source&&a->getHeight()==p.rowHeight&&b->getHeight()==p.rowHeight&&a->getTop()>=0&&a->getBottom()<=p.height;
   if(n)source=source&&panel.lines[n-1]->w1->getBottom()<=a->getTop();baseBottom=a->getBottom();}
  for(int slot=0;slot<g_slots.count;++slot){DataPanelLine_Progress*r=static_cast<DataPanelLine_Progress*>(panel.getLine(SlotKey(slot),0));
   MyGUI::Widget*label=VisibleLabel(panel,r,slot);MyGUI::ISubWidgetText*t=XpPanelTextStyle::RenderedText(label);
   readable=readable&&t&&t->getFontHeight()==25&&t->getCoord().height>=25+(t->getShadow()?1:0);
   skin=skin&&label->skinResource==panel.lines[0]->skinW1&&label->typeName=="EditBox"&&!r->textBox->visible;
   bounds=bounds&&label->getTop()>baseBottom&&label->getBottom()<=p.height&&r->progress->getTop()==label->getTop()+2&&
      r->progress->getHeight()==p.rowHeight-2&&r->progress->getBottom()<=p.height&&r->textBox->getBottom()<=p.height&&r->progressText->getCoord().top==r->progress->getTop();
   frame=frame&&r->progress->skinChildren[0]->getTop()==p.emptyTop&&r->progress->skinChildren[1]->getTop()==p.trackTop&&
      r->progress->skinChildren[2]->getTop()==0&&r->progress->skinChildren[2]->getHeight()==r->progress->getHeight();
   for(MyGUI::Widget*w=label;w;w=w->client)focus=focus&&!w->needMouse&&!w->needKey&&!w->inheritsPick;
  }
  check(readable,"full native rendered font regions replace the measured3px vanilla name sliver");
  check(skin,"owned label inherits actual native runtime type and skin; generic framed name stays hidden");
  check(bounds&&source,"all native attributes/full XP rows remain non-overlapping inside the measured viewport");
  check(frame,"whole bar/frame moves together; native scaled/unscaled skin offsets and physical height remain unchanged");
  check(focus,"owned native label and bounded clients cannot consume mouse/key/picking");
  check(g_divider.get()&&g_divider.get()->getVisible()&&g_divider.get()->getTop()==baseBottom&&g_divider.get()->getHeight()==1,
        "divider owns one clear pixel between native and XP blocks without glyph overlap");
  const int creates=MyGUI::widgetCreates,destroys=MyGUI::widgetDestroys,writes=GeometryWrites(panel),types=MyGUI::typeReads;
  for(int n=0;n<100;++n)Panel(&c.stats,&panel,0,false);
  check(MyGUI::widgetCreates==creates&&MyGUI::widgetDestroys==destroys&&GeometryWrites(panel)==writes&&MyGUI::typeReads==types,
        "100 stable frames allocate/destroy no widgets, re-read no runtime types and rewrite no geometry");
  g_bars=false;g_panelDirty=true;Panel(&c.stats,&panel,0,false);
  check(BaseRestored(panel,p),"disabling XP restores exact original native row coordinates and sizes");
  g_bars=true;g_panelDirty=true;Panel(&c.stats,&panel,0,false);
  // Native relayout can change the baseline while the plugin's arrangement is active.
  for(int n=0;n<p.count;++n){MyGUI::IntCoord a=panel.lines[n]->w1->coord,b=panel.lines[n]->w2->coord;
   a.top=p.tops[n]+1;b.top=a.top;panel.lines[n]->w1->setCoord(a);panel.lines[n]->w2->setCoord(b);}
  Panel(&c.stats,&panel,0,false);g_bars=false;g_panelDirty=true;Panel(&c.stats,&panel,0,false);
  bool external=true;for(int n=0;n<p.count;++n)external=external&&panel.lines[n]->w1->getTop()==p.tops[n]+1;
  check(external,"external native relayout becomes the new restore baseline instead of being overwritten by stale cache");
 }
 MyGUI::vanillaUiProfile=true;const UiProfile&five=profiles[0];const UiProfile&six=profiles[1];
 DatapanelGUI&switching=*new DatapanelGUI;switching.root.coord=MyGUI::IntCoord(0,0,297,181);
 for(int n=0;n<5;++n)SourceRow(switching,five.tops[n],25);
 g_bars=true;g_panelDirty=true;Panel(&c.stats,&switching,0,false);bool transition=g_slots.count==2;
 SourceRow(switching,six.tops[5],25);DataPanelLine*extra=switching.lines.back();
 for(int n=0;n<6;++n){DataPanelLine*r=n<5?switching.lines[n]:extra;MyGUI::IntCoord a=r->w1->coord,b=r->w2->coord;a.top=b.top=six.tops[n];r->w1->setCoord(a);r->w2->setCoord(b);}
 Panel(&c.stats,&switching,0,false);transition=transition&&g_slots.count==1;
 extra->setVisible(false);
 for(int n=0;n<5;++n){MyGUI::IntCoord a=switching.lines[n]->w1->coord,b=switching.lines[n]->w2->coord;a.top=b.top=five.tops[n];switching.lines[n]->w1->setCoord(a);switching.lines[n]->w2->setCoord(b);}
 Panel(&c.stats,&switching,0,false);transition=transition&&g_slots.count==2;
 g_bars=false;g_panelDirty=true;Panel(&c.stats,&switching,0,false);
 check(transition&&BaseRestored(switching,five),"native5-to6-to5 relayout preserves2/1/2 XP and restores the new five-row baseline even when coordinates match prior compacting");
 g_bars=true;g_panelDirty=true;Panel(&c.stats,&switching,0,false);
 DataPanelLine_Progress*percentLine=static_cast<DataPanelLine_Progress*>(switching.getLine(SlotKey(1),0));
 MyGUI::Widget*percentLabel=VisibleLabel(switching,percentLine,1);
 percentLine->progressText->setCoord(MyGUI::IntCoord(percentLine->progress->getLeft(),percentLabel->getTop(),percentLine->progress->getWidth(),25));
 percentLine->progressText->text.coord=MyGUI::IntCoord(0,3,percentLine->progress->getWidth(),22);
 Panel(&c.stats,&switching,0,false);
 check(percentLine->progressText->getTop()==percentLabel->getTop()&&percentLine->progressText->getHeight()==25&&
       percentLine->progressText->getSubWidgetText()->getAbsolutePosition().top+21<=181,
       "unknown percentage top inset retains full native row instead of clipping while complete bar still moves down2px");
 percentLine->progressText->text.coord.top=0;percentLine->progressText->text.coord.height=23;
 Panel(&c.stats,&switching,0,false);const int insetWrites=percentLine->progressText->moves,insetMeasures=percentLine->progressText->text.measures;
 for(int n=0;n<100;++n)Panel(&c.stats,&switching,0,false);
 check(percentLine->progressText->getHeight()==25&&percentLine->progressText->moves==insetWrites&&percentLine->progressText->text.measures==insetMeasures,
       "percentage lower inset uses stable full-height fallback with zero repeated measurements or22/24 oscillation");
 // Actual source type/skin can change at runtime; clone it once, including insets.
 for(int n=0;n<5;++n){DataPanelLine*r=switching.lines[n];r->skinW1="InsetProfile";
  MyGUI::IntCoord a=r->w1->coord,b=r->w2->coord;a.top=b.top=5+n*29;a.height=b.height=29;r->w1->setCoord(a);r->w2->setCoord(b);
  r->w1->client->coord=MyGUI::IntCoord(0,2,a.width,25);r->w1->client->text.coord=MyGUI::IntCoord(0,0,a.width,25);}
 const int beforeSkin=MyGUI::widgetCreates;Panel(&c.stats,&switching,0,false);
 DataPanelLine_Progress*skinLine=static_cast<DataPanelLine_Progress*>(switching.getLine(SlotKey(0),0));MyGUI::Widget*skinLabel=VisibleLabel(switching,skinLine,0);
 check(g_slots.count==1&&skinLabel->skinResource=="InsetProfile"&&XpPanelTextStyle::RenderedText(skinLabel)->getCoord().height>=25&&MyGUI::widgetCreates>beforeSkin,
       "changed native source skin clones its composite client/insets once and preserves full font height");
 const int afterSkin=MyGUI::widgetCreates;for(int n=0;n<100;++n)Panel(&c.stats,&switching,0,false);
 check(MyGUI::widgetCreates==afterSkin,"unchanged replacement skin performs no repeated creation");
 switching.root.coord.height=170;Panel(&c.stats,&switching,0,false);
 bool shrinkSafe=g_slots.count==0;for(size_t n=0;n<switching.root.children.size();++n){MyGUI::Widget*w=switching.root.children[n];if(w->caption=="Strength")shrinkSafe=shrinkSafe&&!w->visible;}
 check(shrinkSafe,"shrunk viewport hides only XP when a complete native-font row cannot fit, preserving native attributes");
 switching.root.coord.height=181;Panel(&c.stats,&switching,0,false);check(g_slots.count==1,"restored viewport reuses the native-style XP row safely");
 fakeNow+=120001;Panel(&c.stats,&switching,0,false);
 bool expired=g_slots.count==0;for(int n=0;n<5;++n)expired=expired&&switching.lines[n]->w1->getTop()==5+n*29;
 check(expired,"expiry restores latest external native geometry and removes owned labels");
 printf("native UI profile integration: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

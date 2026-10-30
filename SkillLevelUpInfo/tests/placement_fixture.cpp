#include "panel_boundary_fakes.h"
#include "PanelPlacement.h"
int main(){
 MyGUI::Widget oldParent,newParent,row;MyGUI::Widget*widgets[]={&row};XpPanelPlacement::Cache cache;
 row.parent=&oldParent;row.coord=MyGUI::IntCoord(8,5,100,25);cache.Capture(&oldParent,widgets,1);
 cache.Apply(&row,MyGUI::IntCoord(8,0,100,25));row.parent=&newParent;cache.Restore();
 check(row.getTop()==0,"reparented native row cannot receive coordinates from its former parent during restore");
 row.parent=&oldParent;row.coord=MyGUI::IntCoord(8,5,100,25);cache.Capture(&oldParent,widgets,1);
 cache.Apply(&row,MyGUI::IntCoord(8,0,100,25));row.parent=&newParent;cache.Capture(&newParent,widgets,1);
 check(cache.Original(&row).top==0,"capture after reparent keeps the new parent's authoritative baseline");cache.Restore();
 row.parent=&oldParent;row.coord=MyGUI::IntCoord(8,5,100,25);cache.Capture(&oldParent,widgets,1);
 cache.Apply(&row,MyGUI::IntCoord(8,0,100,25));MyGUI::UnlinkWidget(&oldParent);row.parent=&newParent;cache.Restore();
 check(row.getTop()==0,"destroyed cached parent cannot authorize restoration of a surviving native row");
 row.parent=&oldParent;row.coord=MyGUI::IntCoord(8,5,100,25);cache.Capture(&oldParent,widgets,1);
 cache.Apply(&row,MyGUI::IntCoord(8,0,100,25));cache.Restore();
 check(row.getTop()==5,"unchanged live parent still restores its exact original native coordinates");
 printf("native placement ownership: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}

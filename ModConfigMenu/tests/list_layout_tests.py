"""Compile the actual BuildTab list block and layout helper at a fake UI boundary.

No game calls or installed files are changed. Skin metrics come from the current
vanilla/Dark UI templates; synthetic variants exercise clipping and rounding.
"""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
MODULE = HERE.parent
OUT = HERE / 'generated' / 'list-layout'
SOURCE = (MODULE / 'ModConfigMenu.cpp').read_text(encoding='utf-8-sig')
OUT.mkdir(parents=True, exist_ok=True)

def function(text, signature):
    start = text.index(signature)
    a = text.index('{', start)
    depth, b = 1, a+1
    while depth:
        depth += (text[b] == '{') - (text[b] == '}')
        b += 1
    return text[start:b]

def extract(text):
    start = text.index('        const int listW =', text.index('void BuildTab('))
    end = text.index('        // Разделитель между списком', start)
    helper = function(text, 'void CenterModListScrollbar(') if 'void CenterModListScrollbar(' in text else ''
    return helper, text[start:end]

game = Path('D:/SteamLibrary/steamapps/common/Kenshi')
skin_rows = []
for name, path in [('vanilla', game/'data/gui/templates/kenshi_templates.xml'),
                   ('Dark UI Advanced', game/'mods/Dark UI Advanced/gui/templates/kenshi_templates.xml')]:
    root = ET.parse(path).getroot()
    resource = next(x for x in root.findall('Resource') if x.get('name') == 'Kenshi_ScrollViewEmpty')
    outer = resource.find('Widget')
    children = {x.get('name'): x for x in outer.findall('Widget')}
    r = list(map(int, outer.get('position').split()))
    c = list(map(int, children['Client'].get('position').split()))
    v = list(map(int, children['VScroll'].get('position').split()))
    assert children['Client'].get('align') == 'Stretch'
    assert children['VScroll'].get('align') == 'Right VStretch'
    # Values are measured from actual skin, including its visual scrollbar inset.
    skin_rows.append((name, c[0], r[2]-c[0]-c[2], v[0]-c[0]-c[2], v[2]))

STUBS = r'''
#include <algorithm>
#include <cstdio>
#include <vector>
#include <string>
namespace MyGUI {
struct IntSize{int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct IntCoord{int left,top,width,height;IntCoord(int l=0,int t=0,int w=0,int h=0):left(l),top(t),width(w),height(h){}};
namespace Align{enum{Default=0,Left=1,VStretch=2,Top=4};}
struct Event{template<class T>void operator+=(T){}};
template<class T>int newDelegate(T){return 0;}
struct Widget{IntCoord c;int resizes;Widget():resizes(0){}virtual~Widget(){}
 IntSize getSize(){return IntSize(c.width,c.height);}IntCoord getCoord(){return c;}
 void setSize(int w,int h){c.width=w;c.height=h;++resizes;}
 template<class T>T*createWidget(const char*,IntCoord pos,int,const char*name=""){T*p=new T;p->c=pos;return p;}
};
struct Button:Widget{Event eventMouseButtonClick,eventMouseWheel;
 void setCaption(const std::string&){}void setUserString(const char*,const char*){}
};
int skinLeft=0,skinGutter=20,skinInset=0,skinBar=20;
struct ScrollView:Widget{int canvasWrites;bool hScroll;IntSize canvas;
 ScrollView():canvasWrites(0),hScroll(true){}
 void setVisibleHScroll(bool b){hScroll=b;}void setCanvasAlign(int){}
 IntCoord getViewCoord(){return IntCoord(skinLeft,0,c.width-skinLeft-(canvas.height>c.height?skinGutter:skinGutter-skinBar),c.height);}
 IntSize getCanvasSize(){return canvas;}
 void setCanvasSize(int w,int h){canvas=IntSize(w,h);++canvasWrites;}
};
}
struct Mod{std::string title,info;};
MyGUI::Widget rootWidget;MyGUI::Widget*g_tab=&rootWidget;MyGUI::ScrollView*g_list=NULL;
std::vector<MyGUI::Button*>g_buttons;std::vector<Mod*>g_mods;
const int BUTTON_H=30,BUTTON_STEP=24,LIST_GAP=20;
void OnModButton(MyGUI::Widget*){}void OnModListWheel(MyGUI::Widget*,int){}
std::string Tr(const char*p){return p;}void Tip(MyGUI::Widget*,const std::string&){}
int checks=0,failures=0;
void check(bool ok,const char*name){++checks;if(!ok){++failures;printf("FAIL %s\n",name);}}
void clear(){for(size_t i=0;i<g_buttons.size();++i)delete g_buttons[i];g_buttons.clear();delete g_list;g_list=NULL;for(size_t i=0;i<g_mods.size();++i)delete g_mods[i];g_mods.clear();}
'''

TESTS = r'''
void trial(int tabW,int h,int count,int left,int gutter,int inset,int bar){
 clear();MyGUI::skinLeft=left;MyGUI::skinGutter=gutter;MyGUI::skinInset=inset;MyGUI::skinBar=bar;
 for(int n=0;n<count;++n)g_mods.push_back(new Mod);
 BuildList(MyGUI::IntSize(tabW,h));
 const int nominal=std::max(200,tabW/4),divider=nominal+LIST_GAP/2-2;
 const MyGUI::IntCoord view=g_list->getViewCoord();
 check(!g_list->hScroll,"horizontal scroll stays disabled");
 check(g_list->c.left==0&&g_list->c.top==0&&g_list->c.height==h,"only list width changes");
 check(g_list->c.width<=divider,"list stays before unchanged divider");
 check(g_list->canvas.height==std::max(8+count*BUTTON_STEP+(BUTTON_H-BUTTON_STEP)+4,h),"native vertical canvas range preserved");
 if(count*BUTTON_STEP+18<=h)check(g_list->c.width==nominal,"hidden scrollbar keeps original list width even with skin padding");
 if(count*BUTTON_STEP+18>h&&count){
  int frame=0;
  for(size_t n=0;n<g_buttons.size();++n){MyGUI::IntCoord b=g_buttons[n]->getCoord();
   frame=std::max(frame,view.left+b.left+b.width);
   check(b.left==4&&b.top==8+int(n)*BUTTON_STEP&&b.height==BUTTON_H,"button positions/height/order preserved");
   check(b.width>0&&b.left+b.width<=view.width,"button fits native client");
  }
  const int barLeft=view.left+view.width+inset,barRight=barLeft+bar;
  check(barLeft>frame&&barRight<divider,"scrollbar separated from button and divider");
  const int a=barLeft-frame,b=divider-barRight;
  check(a-b>=-1&&a-b<=1,"actual skin scrollbar centered within one pixel");
  check(g_list->canvas.width<=view.width,"canvas fits native client without hidden horizontal overflow");
 }
}
int main(){
 SKIN_CASES
 for(int w=800;w<=2560;w+=137)for(int gutter=12;gutter<=48;gutter+=4)
  trial(w,640,64,0,gutter,2,gutter-4);
 trial(800,640,0,0,20,0,20);trial(800,640,2,0,20,0,20);trial(800,640,2,0,20,2,16);
 IDEMPOTENT
 clear();printf("%s %d layout checks\n",failures?"FAIL":"PASS",checks);return failures?1:0;
}
'''

def run(source, label):
    helper, block = extract(source)
    skins = '\n'.join('trial(%d,640,64,%d,%d,%d,%d);' % (w,l,g,i,b)
                      for _,l,g,i,b in skin_rows for w in [800,1024,1440,2160,3840])
    # Also exercise a wider client inset without a different renderer.
    skins += '\ntrial(1440,640,64,7,32,2,28);'
    idem = ''
    if helper:
        idem = r'''trial(1440,640,64,0,20,0,20);int oldWidth=g_list->c.width,oldCalls=g_list->resizes;
        CenterModListScrollbar(std::max(200,1440/4)+LIST_GAP/2-2);
        check(g_list->c.width==oldWidth&&g_list->resizes==oldCalls,"repeated layout does not compound or resize");
        MyGUI::ScrollView*saved=g_list;g_list=NULL;CenterModListScrollbar(368);g_list=saved;
        check(true,"null list is a zero-query no-op");
        MyGUI::Button*first=g_buttons[0];g_buttons[0]=NULL;CenterModListScrollbar(368);g_buttons[0]=first;
        check(g_list->c.width==oldWidth&&g_list->resizes==oldCalls,"null button refuses before any resize");
        int oldLeft=first->c.left;first->c.left=-1;CenterModListScrollbar(368);first->c.left=oldLeft;
        check(g_list->resizes==oldCalls,"negative button coordinate refuses before mutation");
        int oldButtonWidth=first->c.width;first->c.width=0;CenterModListScrollbar(368);first->c.width=oldButtonWidth;
        check(g_list->resizes==oldCalls,"empty button refuses before mutation");
        MyGUI::skinGutter=oldWidth+1;CenterModListScrollbar(368);MyGUI::skinGutter=20;
        check(g_list->resizes==oldCalls,"invalid skin client refuses before mutation");
        CenterModListScrollbar(oldWidth-1);
        check(g_list->resizes==oldCalls,"divider inside outer widget refuses before mutation");'''
    cpp = OUT/(label+'.cpp')
    cpp.write_text(STUBS+'\n'+helper+'\nvoid BuildList(MyGUI::IntSize size){\n'+block+'\n}\n'+TESTS.replace('SKIN_CASES',skins).replace('IDEMPOTENT',idem),encoding='utf-8')
    vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
    sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
    env=os.environ.copy();env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')
    env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
    exe=OUT/(label+'.exe')
    r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W1',str(cpp),'/Fo'+str(OUT/(label+'.obj')),'/Fe'+str(exe)],capture_output=True,text=True,env=env)
    if r.returncode:print(r.stdout,r.stderr);raise SystemExit(r.returncode)
    r=subprocess.run([str(exe)],capture_output=True,text=True,env=env)
    lines=r.stdout.strip().splitlines()
    print(label+': '+'\n'.join(lines if len(lines)<6 else lines[:3]+['...']+lines[-1:]));return r.returncode

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--skip-mutants',action='store_true');args=p.parse_args()
    if run(SOURCE,'actual'):raise SystemExit(1)
    if not args.skip_mutants:
        mutants=[('no-call','        CenterModListScrollbar(listW + LIST_GAP / 2 - 2);',''),
                 ('no-gap','const int targetRight = contentRight + view.left + (dividerX - contentRight - view.left - gutter) / 2;','const int targetRight = view.left + view.width;'),
                 ('untrimmed-canvas','std::min(canvas.width, view.width)','canvas.width'),
                 ('no-button-trim','if (c.width != width) b->setSize(width, c.height);','if (false) b->setSize(width, c.height);')]
        for label,needle,replacement in mutants:
            assert SOURCE.count(needle)==1,(label,'missing production mutation boundary')
            assert run(SOURCE.replace(needle,replacement),label)!=0,label+' survived'
        print('PASS four sensitive compiled layout mutants')

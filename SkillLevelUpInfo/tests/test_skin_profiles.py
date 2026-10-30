"""Run the production panel with address-free measured vanilla/Dark UI profiles."""
from pathlib import Path
import json
p=Path(__file__).resolve().parent
profiles=json.loads((p/'ui_profiles.json').read_text(encoding='utf-8'))['profiles']
rows=[]
for x in profiles:
 tops=x['row_tops']+[0]*(6-len(x['row_tops']))
 rows.append('{"%s",%d,%d,%d,%d,{%s},%s,%d,%d,%d}'%(x['name'],x['parent_width'],x['parent_height'],x['row_height'],len(x['row_tops']),','.join(map(str,tops)),str(x['stock']).lower(),x['expected_xp_capacity'],x['empty_top'],x['track_top']))
out=p/'_generated';out.mkdir(exist_ok=True)
(out/'ui_profile_constants.h').write_text('struct UiProfile{const char*name;int width,height,rowHeight,count,tops[6];bool stock;int capacity,emptyTop,trackTop;};\nconst UiProfile profiles[]={\n'+',\n'.join(rows)+'\n};\n')
driver=(p/'test_head_anchor.py').read_text(encoding='utf-8')
driver=driver.replace("(here / 'head_boundary_fakes.h')","(here / 'panel_boundary_fakes.h')")
driver=driver.replace("source.index('    const std::string& SlotKey')","source.index('    void (*g_origUpdate)')")
driver=driver.replace("source = source.replace('    WidgetRef g_panelRoot, g_divider;\\n', '')",'')
driver=driver.replace("source = source.replace('    DatapanelGUI* g_panel = NULL;\\n', '')",'')
driver=driver.replace("'head_anchor_fixture.cpp'","'skin_profiles_fixture.cpp'")
exec(compile(driver,str(p/'test_head_anchor.py'),'exec'),{'__file__':str(p/'test_head_anchor.py')})

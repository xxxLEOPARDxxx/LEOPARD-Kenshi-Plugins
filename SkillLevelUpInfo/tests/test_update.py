"""Execute actual Update/Increase/DrawPanel lifecycle and timing branches."""
from pathlib import Path
import sys
p=Path(__file__).resolve().parent
driver=(p/'test_head_anchor.py').read_text(encoding='utf-8')
driver=driver.replace("(here / 'head_boundary_fakes.h')","(here / 'panel_boundary_fakes.h')")
driver=driver.replace("source.index('    const std::string& SlotKey')","source.index('    // Re-baseline on save load')")
driver=driver.replace("source = source.replace('    WidgetRef g_panelRoot, g_divider;\\n', '')",'')
driver=driver.replace("source = source.replace('    DatapanelGUI* g_panel = NULL;\\n', '')",'')
driver=driver.replace("'head_anchor_fixture.cpp'","'update_fixture.cpp'")
exec(compile(driver,str(p/'test_head_anchor.py'),'exec'),{'__file__':str(p/'test_head_anchor.py')})

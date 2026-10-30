"""Run actual LoadSettings/MCM_Describe using real local INI parsing."""
from pathlib import Path
p=Path(__file__).resolve().parent
driver=(p/'test_head_anchor.py').read_text(encoding='utf-8')
start=driver.index("a = source.index('    void LoadSettings()')")
end=driver.index('stats =',start)
driver=driver[:start]+driver[end:]
driver=driver.replace("common += '\\n#include \"XpInfoCore.h\"\\n'",
 "common += '\\n#include \"XpInfoCore.h\"\\n#include <PluginSettings.h>\\n#include <ModConfigMenu.h>\\n'")
driver=driver.replace("source += '\\n}\\n'", "original = (mod / 'SkillLevelUpInfo.cpp').read_text(encoding='utf-8-sig')\nsource += '\\n}\\n' + original[original.index('extern \"C\" __declspec(dllexport) void MCM_Describe'):original.index('__declspec(dllexport) void startPlugin')]")
driver=driver.replace("mod, here, Path(","mod, here, mod.parent / 'shared', Path(")
driver=driver.replace("'head_anchor_fixture.cpp'","'settings_fixture.cpp'")
exec(compile(driver,str(p/'test_head_anchor.py'),'exec'),{'__file__':str(p/'test_head_anchor.py')})

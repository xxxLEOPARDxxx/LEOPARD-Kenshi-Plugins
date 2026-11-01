"""Cross-repository audit: all canonical loader users have a settings binding."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
catalog = json.loads((ROOT / 'tools/workshop/mcm_catalog.json').read_text('utf-8-sig'))
count = 0
for mod in catalog:
    if not mod.get('source'):
        continue
    root = Path(mod['source'])
    texts = [p.read_text('utf-8-sig', errors='replace')
             for p in root.rglob('*')
             if p.is_file() and p.suffix in {'.cpp', '.inl', '.h'}
             and not {'tests', 'tools', 'build', '.git'}.intersection(p.relative_to(root).parts)]
    if not any('#define KLOC_DOMAIN ' in t for t in texts):
        continue  # Formations uses its own, quiet built-in translations.
    assert any('KLocLog::SetEnabled(' in t or 'KLocLog::FollowConfiguration ' in t
               or 'PluginDiagnostics::Configure(' in t for t in texts), mod['package']
    count += 1
loader = (ROOT / 'shared/Localization.h').read_text('utf-8-sig')
assert 'DebugLog(' not in loader, 'loader info must go through KLocLog; errors through ErrorLog'
assert 'ErrorLog(message);' in loader  # Invalid printf translations remain visible.
assert 'ErrorLog(KLOC_DOMAIN ": cannot load translation "' in loader
assert 'KLocLog::SetSummary(message);' in loader
print(f'{count} canonical localization users have per-plugin diagnostics bindings; errors remain unconditional')

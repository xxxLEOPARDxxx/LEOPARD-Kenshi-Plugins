"""Verify three compiled locales and actual vanilla/Dark skin resources (no UI control)."""
from pathlib import Path
import gettext,json,xml.etree.ElementTree as ET
here=Path(__file__).resolve().parent;mod=here.parent
keys=['Saved games','Save screenshots','No preview available',
      'Capture the game with its HUD when a save succeeds, excluding save/load and pause menus. Select a save to see its screenshot below Load Game. Works with manual saves, quicksaves and autosaves; old saves remain loadable without an image.']
for language in ('en_GB','ru_RU','zh_CN'):
    with (mod/'locale'/language/'LC_MESSAGES/ui_tweaks.mo').open('rb') as f:catalog=gettext.GNUTranslations(f)
    for key in keys:
        assert key in catalog._catalog and catalog.gettext(key).strip(),(language,key)
        if language!='en_GB':assert catalog.gettext(key)!=key,(language,key)
source=(mod/'SavePreviews.h').read_text(encoding='utf-8')
assert 'setTextColour' not in source and 'setTextShadowColour' not in source,'theme colours must be inherited'
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
def resources(directory):
    result={}
    for folder in ('skins','templates'):
        for path in sorted((directory/folder).glob('*.xml')):
            for node in ET.parse(path).getroot().findall('Resource'):
                result[node.attrib['name']]=(node,path)
    return result
base=resources(game/'data/gui');report={}
for name,overrides in [('vanilla',{}),('Dark UI Advanced',resources(game/'mods/Dark UI Advanced/gui'))]:
    effective=base|overrides
    frame,frame_path=effective['Kenshi_FloatingPanelSkin'];template,text_path=effective['Kenshi_TextboxStandardText']
    widget=template.find('Widget');props={p.attrib['key']:p.attrib['value'] for p in widget.findall('Property')}
    assert frame.attrib.get('texture') and frame.findall('BasisSkin'),name
    assert props.get('FontName') and props.get('TextColour'),name
    empty,_=effective[widget.attrib['skin']]
    assert any(node.attrib.get('type') in ('SimpleText','EditText') for node in empty.findall('BasisSkin')),name
    assert 'ImageBox' in effective,name
    report[name]={'frame':str(frame_path),'text_template':str(text_path),
                  'font':props['FontName'],'text_colour':props['TextColour'],'frame_size':frame.attrib['size']}
out=here/'_generated';out.mkdir(exist_ok=True)
(out/'save-preview-skin-contract.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('PASS preview text: 12 compiled translations; vanilla + Dark XML frame/font/text-area/palette inheritance')

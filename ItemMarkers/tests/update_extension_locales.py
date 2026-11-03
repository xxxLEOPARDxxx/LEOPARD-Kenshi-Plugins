from pathlib import Path
import importlib.util, json, gettext
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('po2mo',root.parent/'tools'/'po2mo.py')
po2mo=importlib.util.module_from_spec(spec);spec.loader.exec_module(po2mo)
translations={
    'Reload custom grade rules': {
        'en_GB':'Reload custom grade rules',
        'ru_RU':'Перезагрузить правила качества',
        'zh_CN':'重新加载自定义品质规则'},
    'Reload profiles from the grades folder. Invalid files leave the previous rules active.': {
        'en_GB':'Reload profiles from the grades folder. Invalid files leave the previous rules active.',
        'ru_RU':'Перезагрузить профили из папки grades. Если в файлах ошибка, прежние правила продолжат работать.',
        'zh_CN':'重新加载 grades 文件夹中的配置。文件无效时保留之前的规则。'},
}
for lang in ('en_GB','ru_RU','zh_CN'):
    path=root/'locale'/lang/'LC_MESSAGES'/'item_markers.po'
    text=path.read_text(encoding='utf-8-sig')
    entries=po2mo.parse_po(str(path))
    for key,values in translations.items():
        if key not in entries:
            text+='\nmsgid '+json.dumps(key,ensure_ascii=False)+'\nmsgstr '+json.dumps(values[lang],ensure_ascii=False)+'\n'
        elif entries[key]!=values[lang]:raise ValueError('Unexpected existing translation: '+key)
    path.write_text(text,encoding='utf-8',newline='\n')
    po2mo.write_mo(po2mo.parse_po(str(path)),str(path.with_suffix('.mo')))
    with path.with_suffix('.mo').open('rb') as f:catalog=gettext.GNUTranslations(f)
    assert all(catalog.gettext(k)==v[lang] for k,v in translations.items())
print('ItemMarkers extension strings: EN/RU/ZH PO+MO verified')

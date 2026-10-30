"""Maintain and compile module catalogs only; does not touch installed files."""
from pathlib import Path
import json,re,ast,types,gettext
root=Path(__file__).resolve().parents[1]
compiler=Path(r'D:\DEV\Kenshi\po2mo.py')
tree=ast.parse(compiler.read_text(encoding='utf-8-sig'))
# The legacy script has unguarded top-level work. Import only its pure
# functions and imports, never its default installed-mod scan.
tree.body=[n for n in tree.body if isinstance(n,(ast.Import,ast.ImportFrom,ast.FunctionDef))]
po=types.ModuleType('po2mo');exec(compile(tree,str(compiler),'exec'),po.__dict__)
translations={
'Hive medic':('Медицина улья','蜂巢医疗'),
'Vet':('Ветеринария','兽医'),
'Mass combat':('Массовый бой','群体战斗'),
'Survival':('Выживание','生存'),
'+{1} more':('+ещё {1}','+另有 {1} 项'),
'XP gain text, skill level-ups and recent training bars from XP Info.':('Текст прибавок опыта, повышения навыков и полосы недавних тренировок из XP Info.','显示 XP Info 的经验获取文字、技能升级和近期训练进度条。'),
'Enable XP gain text, level-up messages and character-panel bars.':('Включить текст прибавок опыта, сообщения о повышении уровня и полосы в панели персонажа.','启用经验获取文字、升级提示和角色面板进度条。'),
'XP gain text':('Текст прибавок опыта','经验获取文字'),
'Show accumulated skill gains as rising blue text, such as +3% Athletics.':('Показывать накопленный прирост навыка синим всплывающим текстом, например +3% Атлетика.','以向上飘动的蓝色文字显示累积的技能经验，例如 +3% 运动。'),
'Skill level-up text':('Текст повышения навыка','技能升级文字'),
'Show reached whole skill levels as rising gold text.':('Показывать достигнутый целый уровень навыка золотым всплывающим текстом.','以向上飘动的金色文字显示达到的技能整数等级。'),
'XP bars in character panel':('Полосы опыта в панели персонажа','角色面板经验条'),
'Show recently trained skills below the vanilla character stats, separated by a divider.':('Показывать недавно тренируемые навыки под обычными характеристиками персонажа, после разделителя.','在原版角色属性下方显示近期训练的技能，并用分隔线区分。'),
'Maximum XP rows':('Максимум строк опыта','经验条最大行数'),
'Show up to two recent skills in the character panel. Only one row appears when space is limited.':('Показывать до двух недавно тренируемых навыков в панели персонажа. При нехватке места остаётся одна строка.','角色面板最多显示两项近期训练的技能。空间不足时仅显示一行。'),
'Minimum XP gain, %':('Минимальная прибавка опыта, %','最小经验增量，%'),
'Accumulate small skill gains until this percentage can be announced. Level-ups always appear immediately.':('Накапливать малые прибавки до этого процента. Повышение целого уровня показывается сразу.','累积小幅技能增量，达到此百分比后显示。技能升级会立即显示。'),
'XP announcement interval, s':('Интервал сообщений опыта, с','经验提示间隔，秒'),
'Minimum collection time for fractional XP messages. Level-up messages do not wait.':('Минимальное время накопления дробных прибавок опыта. Сообщения о повышении уровня не ждут.','小幅经验增量提示的最短累积时间。升级提示无需等待。'),
'Recent training duration, s':('Время недавней тренировки, с','近期训练保留时间，秒'),
'Keep a skill in the panel for this long after its last XP gain.':('Оставлять навык в панели на это время после последней прибавки опыта.','技能最后一次获得经验后，在面板中保留的时间。'),
'Details in RE_Kenshi_log.txt: settings, XP and level messages, and save-load baseline resets.':('В RE_Kenshi_log.txt: настройки, сообщения об опыте и уровнях, сброс исходных значений при загрузке.','在 RE_Kenshi_log.txt 中记录设置、经验和升级提示，以及读档后的基准值重置。'),
}
source=(root/'SkillLevelUpInfo.cpp').read_text(encoding='utf-8-sig')
needed=set(re.findall(r'Tr\("([^"]+)"\)',source))
needed.update(re.findall(r'"([A-Za-z][A-Za-z ]+)"',source[source.index('kStatNames'):source.index('struct Snapshot')]))
for locale in ('en_GB','ru_RU','zh_CN'):
 path=root/'locale'/locale/'LC_MESSAGES'/'skill_level_up_info.po'
 old=po.parse_po(str(path));entries={'':old['']}
 for msg in sorted(needed):
  entries[msg]=msg if locale=='en_GB' else translations[msg][0 if locale=='ru_RU' else 1] if msg in translations else old.get(msg)
  if not entries[msg]:raise ValueError(f'{locale} missing {msg}')
 path.write_text('\n\n'.join('msgid '+json.dumps(k,ensure_ascii=False)+'\nmsgstr '+json.dumps(v,ensure_ascii=False) for k,v in entries.items())+'\n',encoding='utf-8')
 # Use the common project compiler without installing/source PO in packages.
 po.write_mo(entries,str(path.with_suffix('.mo')))
 with path.with_suffix('.mo').open('rb') as f: catalog=gettext.GNUTranslations(f)
 assert all(catalog.gettext(m)==entries[m] for m in needed)
 print(locale,len(entries),'entries, MO roundtrip verified')

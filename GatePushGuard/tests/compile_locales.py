"""Generate source PO and verified MO-only runtime catalogs (standard library)."""
import argparse, ast, gettext, io, json, pathlib, re, struct
ROOT=pathlib.Path(__file__).resolve().parents[1]
ROWS=[
("Stops observed crowd-push entry through closed player gates and doors. Independently authored; use instead of Gate Push Fix.","Препятствует замеченному проталкиванию внутрь через закрытые ворота и двери игрока. Самостоятельная разработка; используется вместо Gate Push Fix.","阻止观察到的角色被人群推过玩家关闭的大门和房门。独立编写；请用它替代 Gate Push Fix。"),
("Protection","Защита","保护"),
("Enabled","Включено","启用"),
("Return a character only after an observed outside-to-inside crossing through a continuously closed player door. Existing occupants and gate closure are never enough to move anyone.","Возвращает персонажа только после замеченного прохода снаружи внутрь через непрерывно закрытую дверь игрока. Само закрытие ворот и нахождение внутри не вызывают перемещение.","仅在观察到角色从外侧穿过持续关闭的玩家房门进入内侧时将其送回。原本就在内侧或关门本身都不会导致角色被移动。"),
("Diagnostics","Диагностика","诊断"),
("Detailed log","Подробный журнал","详细日志"),
("Log recognized door geometry and corrected crossings in RE_Kenshi_log.txt. Errors are always logged.","Записывает распознанную геометрию дверей и исправленные проходы в RE_Kenshi_log.txt. Ошибки записываются всегда.","将识别到的门几何和纠正的穿门事件写入 RE_Kenshi_log.txt。错误始终记录。"),
]
def compile_mo(catalog):
 keys=sorted(catalog);n=len(keys);offset=28+16*n;payload=bytearray();tables=[]
 for values in (keys,[catalog[k] for k in keys]):
  entries=[]
  for text in values:
   raw=text.encode('utf-8');entries.append((len(raw),offset+len(payload)));payload.extend(raw+b'\0')
  tables.append(b''.join(struct.pack('<II',*entry) for entry in entries))
 return struct.pack('<7I',0x950412de,0,n,28,28+8*n,0,0)+b''.join(tables)+payload
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
 required={ast.literal_eval(s) for s in re.findall(r'\bTr\(("(?:[^"\\]|\\.)*")\)',(ROOT/'GatePushGuard.cpp').read_text(encoding='utf-8'))}
 assert required=={r[0] for r in ROWS},required^{r[0] for r in ROWS}
 for index,lang in enumerate(('en_GB','ru_RU','zh_CN')):
  header=f'Language: {lang}\nContent-Type: text/plain; charset=UTF-8\nMIME-Version: 1.0\n'
  catalog={'':header};lines=['# Gate Push Guard, independently authored by LEOPARD. GPL-3.0-or-later.','msgid ""','msgstr '+json.dumps(header,ensure_ascii=False),'']
  for row in ROWS:
   assert row[index];catalog[row[0]]=row[index]
   lines+=['msgid '+json.dumps(row[0],ensure_ascii=False),'msgstr '+json.dumps(row[index],ensure_ascii=False),'']
  source=ROOT/'locale'/lang/'LC_MESSAGES'/'gate_push_guard.po';source.parent.mkdir(parents=True,exist_ok=True);source.write_text('\n'.join(lines),encoding='utf-8')
  output=a.output/'locale'/lang/'LC_MESSAGES'/'gate_push_guard.mo';output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(compile_mo(catalog))
  loaded=gettext.GNUTranslations(io.BytesIO(output.read_bytes()))
  assert all(loaded.gettext(row[0])==row[index] for row in ROWS)
  assert not list((a.output/'locale'/lang).rglob('*.po'))
  print(f'{lang}: {len(ROWS)} strings, MO round-trip and runtime PO exclusion pass')
if __name__=='__main__':main()

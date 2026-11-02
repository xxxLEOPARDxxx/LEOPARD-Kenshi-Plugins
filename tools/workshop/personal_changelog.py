"""Local Russian-only owner journal. Never included in release content.

One writer at a time, called only after confirmed publication. Development,
build/test steps and release preparation are deliberately excluded.
"""
from datetime import datetime, timezone, timedelta
from pathlib import Path
import argparse, json, os, re, subprocess, tempfile

WORKSHOP=Path('D:/DEV/Kenshi/workshop')
REPOSITORY=Path('D:/DEV/Kenshi/ours')
JOURNAL=WORKSHOP/'ALL_CHANGELOG_MODS.txt'
MOSCOW=timezone(timedelta(hours=3))
SEPARATOR='\n'+'='*72+'\n'
HEADER='''ОБЩИЙ ЛИЧНЫЙ ЖУРНАЛ ИЗМЕНЕНИЙ МОДОВ KENSHI

Только для владельца сборки. В архивы модов и публичные описания не включается.
Новые записи сверху, старые снизу. Время московское.

Начальная история восстановлена из русских журналов выпусков. Для неё указано
время фиксации записи в репозитории, а не выдуманное время старой публикации.
Только выпущенные обновления: возможности, исправления ошибок и производительность.
Разработка, планы, сборки и внутренние технические этапы сюда не записываются.
'''


def russian(text):
    """Existing canonical history is English first, complete Russian last."""
    lines=text.splitlines()
    first=next((i for i,line in enumerate(lines) if re.search('[А-Яа-яЁё]',line)),None)
    if first is None:raise ValueError('В записи отсутствует русский текст')
    body='\n'.join(lines[first:]).strip()
    body=re.sub(r'^v\S+\s*[-:]?\s*Русский\s*\n','',body)
    body=re.sub(r'^v[\d.]+\s*:\s*','',body)
    body=re.sub(r'\[/?(?:b|i|u|list)\]|\[\*\]','',body)
    return body.strip()


def normalized(text):
    return re.sub(r'\s+',' ',re.sub(r'\[/?(?:b|i|u)\]','',text)).strip()


def gameplay(text):
    """Keep player-facing results; the full technical notes stay elsewhere."""
    result=[]
    for line in text.splitlines():
        line=line.strip()
        if not line:continue
        if re.match(r'-?\s*(Чист[а-яё]* (?:пакет|обновление)|Ошибки остаются видимыми|В подробный журнал добавлены ограниченные сводки)',line):continue
        if 'Необязательная диагностика языка.' in line:
            result.append('- Подробные сообщения появляются только при включённом журнале отладки; ошибки видны всегда.');continue
        if '.defaults.ini' in line:
            line=line.split(':',1)[0].rstrip('.')+'.'
        sentences=re.split(r'(?<=[.!?])\s+(?=[А-ЯЁ])',line)
        sentences=[x for x in sentences if not re.search(r'\b(?:SDK|ABI|API|CRC|VC10|PNG|JPEG)\b|1600x900|невиртуальн|виртуального сравнения|рабоч[а-я]+ каталог|скомпилированн|\bhand\b|/LTCG',x)]
        line=' '.join(sentences).strip()
        if line and line not in result:result.append(line)
    return '\n'.join(result)


def block(mod,version,text,when,status):
    if not re.search('[А-Яа-яЁё]',text):raise ValueError('Нужен русский текст изменений')
    if any('\n' in s or '\r' in s for s in (mod,version,status)):raise ValueError('Неверная подпись записи')
    stamp=when.astimezone(MOSCOW).strftime('%d.%m.%Y %H:%M:%S')
    return f'{stamp} МСК | {status}\nМод: {mod} | Версия: {version}\n\n{text.strip()}\n'


def atomic_write(path,text):
    path=Path(path);path.parent.mkdir(parents=True,exist_ok=True)
    fd,name=tempfile.mkstemp(prefix='.all-mods-',suffix='.tmp',dir=path.parent)
    try:
        with os.fdopen(fd,'w',encoding='utf-8',newline='\n') as out:
            out.write(text);out.flush();os.fsync(out.fileno())
        os.replace(name,path)
    finally:
        if os.path.exists(name):os.unlink(name)


def record(mod,version,text,status='Выпущено',when=None,target=JOURNAL):
    if status!='Выпущено':raise ValueError('В личный журнал входят только выпущенные обновления')
    text=gameplay(text)
    if not text:raise ValueError('Нет изменений для игрока')
    target=Path(target);old=target.read_text(encoding='utf-8') if target.exists() else HEADER
    title=f'Мод: {mod} | Версия: {version}'
    for entry in old.split(SEPARATOR):
        if title+'\n\n' in entry:
            previous=entry.split(title+'\n\n',1)[1]
            if normalized(previous)==normalized(text):return False
    entry=block(mod,version,text,when or datetime.now(MOSCOW),status)
    # Header remains at the top; every new record is inserted immediately below.
    if SEPARATOR in old:
        head,rest=old.split(SEPARATOR,1)
        entries=[entry]+rest.split(SEPARATOR)
        def date_key(value):
            try:return datetime.strptime(value[:19],'%d.%m.%Y %H:%M:%S')
            except ValueError:return datetime.min
        entries.sort(key=date_key,reverse=True)
        updated=head+SEPARATOR+SEPARATOR.join(entries)
    else:updated=old.rstrip()+'\n'+SEPARATOR+entry
    atomic_write(target,updated);return True


def initialize(repository=REPOSITORY,target=JOURNAL):
    target=Path(target)
    if target.exists():raise FileExistsError('Общий журнал уже существует: история не перезаписана')
    repository=Path(repository);relative='tools/workshop/changelog.json'
    commits=subprocess.check_output(['git','log','--reverse','--format=%H %aI','--',relative],cwd=repository,text=True,encoding='utf-8').splitlines()
    known={};dates={}
    for row in commits:
        sha,stamp=row.split(' ',1)
        data=json.loads(subprocess.check_output(['git','show',sha+':'+relative],cwd=repository).decode('utf-8-sig'))
        for mod,versions in data.items():
            for version,note in versions.items():
                key=(mod,str(version));ru=russian(note)
                if known.get(key)!=ru:dates[key]=datetime.fromisoformat(stamp)
                known[key]=ru
    order=sorted(known,key=lambda k:(dates[k],int(k[1]) if k[1].isdigit() else 0,k[0]),reverse=True)
    # Confirmed latest publication times override retrospective metadata dates.
    publication_log=repository/'tools/maintenance/steam-publication-37-2026-11-02.jsonl'
    if publication_log.exists():
        for row in publication_log.read_text(encoding='utf-8-sig').splitlines():
            event=json.loads(row)
            if event.get('kind')=='item_complete':dates[(event['package'],str(event['version']))]=datetime.fromisoformat(event['time_utc'])
    order=sorted(known,key=lambda k:(dates[k],int(k[1]) if k[1].isdigit() else 0,k[0]),reverse=True)
    entries=[]
    for mod,ver in order:
        body=gameplay(known[(mod,ver)])
        if body:entries.append(block(mod,ver,body,dates[(mod,ver)],'Выпущено'))
    atomic_write(target,HEADER.rstrip()+'\n'+SEPARATOR+SEPARATOR.join(entries))
    return len(entries)


def record_published(mod,version,here):
    history=json.loads((Path(here)/'changelog.json').read_text(encoding='utf-8-sig'))
    return record(mod,str(version),russian(history[mod][str(version)]))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description='Личный общий журнал изменений модов')
    parser.add_argument('--initialize',action='store_true')
    parser.add_argument('--mod');parser.add_argument('--version',required=False);parser.add_argument('--text')
    parser.add_argument('--published',action='store_true',help='Подтверждённый выпуск, не подготовка')
    args=parser.parse_args()
    if args.initialize:print('Восстановлено записей:',initialize())
    elif args.published and args.mod and args.version and args.text:print('Запись добавлена:',record(args.mod,args.version,args.text))
    else:parser.error('Нужны --initialize либо --published, --mod, --version и --text')

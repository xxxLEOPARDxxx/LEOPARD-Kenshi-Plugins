from pathlib import Path
from datetime import datetime,timezone,timedelta
import tempfile
import personal_changelog as log

def main():
    assert log.russian('v2 English\nEnglish text\n\nv2 - Русский\n- Исправлена ошибка.')=='- Исправлена ошибка.'
    assert log.russian('v1: English.\n\nv1: Добавлено меню.')=='Добавлено меню.'
    with tempfile.TemporaryDirectory() as directory:
        path=Path(directory)/'ALL_CHANGELOG_MODS.txt'
        first=datetime(2026,11,2,10,20,tzinfo=timezone.utc)
        assert log.record('Мод А','2','Исправлена ошибка.',when=first,target=path)
        before=path.read_text(encoding='utf-8');assert '02.11.2026 13:20:00 МСК' in before
        assert not log.record('Мод А','2','Исправлена   ошибка.',target=path)
        assert path.read_text(encoding='utf-8')==before
        owner='\nЛичная заметка владельца.\n';path.write_text(before+owner,encoding='utf-8')
        assert log.record('Мод Б','1','Добавлена настройка.',when=first+timedelta(seconds=1),target=path)
        after=path.read_text(encoding='utf-8')
        assert after.index('Мод Б')<after.index('Мод А') and owner in after
        assert log.record('Мод А','2','Исправлена другая ошибка.',when=first+timedelta(seconds=2),target=path)
        assert log.record('Мод В','1','Старая правка.',when=first-timedelta(days=1),target=path)
        after=path.read_text(encoding='utf-8');assert after.index('Мод Б')<after.index('Мод В') and owner in after
        assert not any(Path(directory).glob('*.tmp'))
        try:log.initialize(target=path)
        except FileExistsError:pass
        else:raise AssertionError('Журнал владельца был перезаписан')
        try:log.record('Мод','1','English only',target=path)
        except ValueError:pass
        else:raise AssertionError('Пропущен нерусский текст')
        try:log.record('Мод','1','Проверка сборки.',status='Разработка',target=path)
        except ValueError:pass
        else:raise AssertionError('В журнал попала разработка')
        assert not log.gameplay('- Чистый пакет обновления. Каталоги подготовлены.')
        assert 'SDK' not in log.gameplay('Исправлена ошибка сортировки. Для авторов плагинов: API и SDK обновлены.')
    print('Личный журнал: порядок, МСК, повторы, заметки владельца и защита истории проверены')

if __name__=='__main__':main()

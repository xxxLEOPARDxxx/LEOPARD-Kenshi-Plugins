"""Read-only save v15 inventory; never opens a process or changes a save.

Positions belong to platoon instances; their state IDs link character/medical
records. Runtime goals, navmesh, supports and collision normals are NOT saved.
"""
from pathlib import Path
import json
import sys

sys.path.insert(0, 'D:/DEV/Kenshi/tools')
from modread import Reader


def read_save(path):
    r = Reader(path.read_bytes())
    assert r.i32() == 15, path
    r.i32()  # next record ID
    result = []
    for _ in range(r.i32()):
        record = dict(instance_count=r.i32(), type=r.i32(), id=r.i32(),
                      name=r.string(), string_id=r.string(), change=r.i32())
        readers = [('bool', r.u8), ('float', r.f32), ('int', r.i32),
                   ('vec3', lambda: [r.f32() for _ in range(3)]),
                   ('vec4', lambda: [r.f32() for _ in range(4)]),
                   ('str', r.string), ('file', r.string)]
        for kind, read in readers:
            record[kind] = {r.string(): read() for _ in range(r.i32())}
        record['refs'] = {}
        for _ in range(r.i32()):
            category = r.string()
            record['refs'][category] = [
                [r.string(), r.i32(), r.i32(), r.i32()] for _ in range(r.i32())]
        record['instances'] = []
        for _ in range(r.i32()):
            identity, target = r.string(), r.string()
            position = [r.f32() for _ in range(3)]
            orientation = [r.f32() for _ in range(4)]
            states = [r.string() for _ in range(r.i32())]
            record['instances'].append(dict(id=identity, target=target,
                position=position, orientation=orientation, states=states))
        result.append(record)
    assert r.p == len(r.d), (path, r.p, len(r.d))
    return result


def inspect(save):
    actors = []
    parsed_files = 0
    for path in (save / 'platoon').glob('*.platoon'):
        records = read_save(path)
        parsed_files += 1
        index = {record['string_id']: record for record in records}
        for record in records:
            if record['type'] != 30:
                continue
            for instance in record['instances']:
                x, y, z = instance['position']
                if (x - 18778) ** 2 + (z - 47724) ** 2 >= 200 ** 2:
                    continue
                states = [index[state] for state in instance['states'] if state in index]
                character = next((state for state in states if state['type'] == 36), None)
                medical = next((state for state in states if state['type'] == 57), None)
                actors.append(dict(file=path.name, instance=instance,
                                   character=character, medical=medical))
    return dict(save=str(save), timestamp=(save / 'quick.save').stat().st_mtime,
                parsed_files=parsed_files, actors=actors)


if __name__ == '__main__':
    reports = [inspect(Path(path)) for path in sys.argv[1:]]
    output = Path(__file__).parent / '_generated' / 'saved_platform_actors.json'
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(reports, ensure_ascii=False, indent=2), encoding='utf-8')
    for report in reports:
        print(report['save'], report['parsed_files'], 'platoon files fully consumed')
        for actor in report['actors']:
            character = actor['character']
            print(actor['file'], character['str'].get('name') if character else '?',
                  actor['instance']['position'],
                  {key: value for key, value in character['int'].items()
                   if key.startswith('handle')} if character else {})

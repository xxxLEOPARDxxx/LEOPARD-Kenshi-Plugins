"""Production resolver and preparation boundary; never changes game files."""
import importlib.util
import os
from pathlib import Path
import tempfile

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('release_build_under_test',HERE/'build.py')
build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
old=os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)
try:
    original=build.mod_dir('UITweaks')
    assert original and Path(original).is_dir()
    with tempfile.TemporaryDirectory(prefix='kenshi-release-input-') as temporary:
        os.environ['KENSHI_RELEASE_INPUT_ROOT']=temporary
        candidate=Path(temporary)/'UITweaks';candidate.mkdir()
        def refused(name):
            try:build.mod_dir(name)
            except ValueError:return
            raise AssertionError('Invalid/missing candidate was accepted: '+name)
        refused('UITweaks')  # Incomplete folder cannot shadow an installation.
        (candidate/'RE_Kenshi.json').write_text('{"Plugins":["UITweaks.dll"]}',encoding='utf-8')
        assert Path(build.mod_dir('UITweaks'))==candidate.resolve()
        refused('StackSort')  # Exists in game, but absent from this candidate set.
        refused('AIFixed')   # Held package must not silently use the game DLL.
        refused('../UITweaks')
        refused('UnknownPackage')
        os.environ['KENSHI_RELEASE_INPUT_ROOT']='relative-candidates'
        refused('UITweaks')
    os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)
    assert build.mod_dir('UITweaks')==original
    # Cross references inspect the unchanged metadata catalog, not runtime input.
    assert any(i['mod']=='AIFixed' and str(i['version']).isdigit() for i in build.ITEMS)
    prepare=(HERE/'prepare_release.py').read_text('utf-8')
    assert 'dst=build.mod_dir(mod)' in prepare
    print('PASS: candidate/default resolution; missing/held/unknown/relative refusal; metadata cross reference; preparation resolver')
finally:
    if old is not None:os.environ['KENSHI_RELEASE_INPUT_ROOT']=old
    else:os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)

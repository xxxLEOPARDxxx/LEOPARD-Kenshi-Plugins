"""Exercise real prepare/packer refusal and old-ZIP preservation in temporary files."""
from pathlib import Path
import importlib.util
import os
import shutil
import sys
import tempfile

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import build
import nexus_pack
import prepare_release
old=os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)
try:
    with tempfile.TemporaryDirectory(prefix='kenshi-release-boundary-') as temporary:
        root=Path(temporary);candidate=root/'inputs/UITweaks';candidate.mkdir(parents=True)
        (candidate/'RE_Kenshi.json').write_text('{"Plugins":["UITweaks.dll"]}',encoding='utf-8')
        (candidate/'UITweaks.dll').write_bytes(b'isolated test candidate, never installed')
        (candidate/'README.txt').write_text('English / Русский test README',encoding='utf-8')
        os.environ['KENSHI_RELEASE_INPUT_ROOT']=str(root/'inputs')
        output=root/'nexus';output.mkdir();nexus_pack.OUT=str(output)
        old_zip=output/'UITweaks-12.zip';old_zip.write_bytes(b'preserve old release until verified')
        item={'mod':'UITweaks','version':'13'}
        try:nexus_pack.pack(item)
        except RuntimeError as exc:assert 'three runtime languages' in str(exc)
        else:raise AssertionError('Incomplete candidate incorrectly packed')
        assert old_zip.read_bytes()==b'preserve old release until verified'
        assert not (output/'UITweaks-13.zip').exists()
        # Use actual compiled catalogs; no fabricated gettext file is accepted.
        source=HERE.parents[1]/'UITweaks/locale'
        for language in ('en_GB','ru_RU','zh_CN'):
            target=candidate/'locale'/language/'LC_MESSAGES';target.mkdir(parents=True)
            shutil.copy2(source/language/'LC_MESSAGES/ui_tweaks.mo',target/'ui_tweaks.mo')
        real_defaults=build.defaults_ini
        bad_defaults=root/'bad.ini';bad_defaults.write_text('[Tweaks]\nDebug=1\n',encoding='ascii')
        build.defaults_ini=lambda name:str(bad_defaults)
        try:nexus_pack.pack(item)
        except RuntimeError as exc:assert 'logging is enabled' in str(exc)
        else:raise AssertionError('Debug-on defaults incorrectly packed')
        assert old_zip.exists() and not (output/'UITweaks-13.zip').exists()
        build.defaults_ini=real_defaults
        path,count=nexus_pack.pack(item)
        assert Path(path).is_file() and count==7 and not old_zip.exists()
        # Execute the real preparation caller, replacing only its subprocesses
        # and metadata writer. It must resolve selected runtime input fail-closed.
        prepare_release.HERE=root/'prepare';prepare_release.HERE.mkdir()
        prepare_release.sync_notes=lambda items:None
        calls=[];prepare_release.subprocess.run=lambda args,**kwargs:calls.append(args)
        original_source_dir=build.source_dir;build.source_dir=lambda name:None
        sys.argv=['prepare_release.py','UITweaks']
        assert prepare_release.main()==0 and len(calls)==5
        sys.argv=['prepare_release.py','AIFixed']
        try:prepare_release.main()
        except ValueError as exc:assert 'Missing clean release candidate' in str(exc)
        else:raise AssertionError('Held package fell back to installed DLL')
        build.source_dir=original_source_dir
    print('PASS: real pack/prepare boundaries; failed catalog/default preflight preserves old ZIP; valid candidate replaces it; held package cannot fall back')
finally:
    if old is not None:os.environ['KENSHI_RELEASE_INPUT_ROOT']=old
    else:os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)

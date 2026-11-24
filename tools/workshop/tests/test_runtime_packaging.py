from pathlib import Path
from tempfile import TemporaryDirectory
from unittest import TestCase, main
from unittest.mock import patch
from zipfile import ZipFile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import build
import nexus_pack


class RuntimePackaging(TestCase):
    def test_stale_source_bundle_is_excluded_from_runtime(self):
        item = next(item for item in build.ITEMS if item['mod'] == 'RangedDamageXp')
        with TemporaryDirectory() as directory:
            root = Path(directory)
            runtime = root / 'RangedDamageXp'
            runtime.mkdir()
            (runtime / 'RangedDamageXp.dll').write_bytes(b'compiled plugin')
            source_bundle = runtime / 'RangedDamageXp-source.zip'
            source_bundle.write_bytes(b'old source bundle')

            with patch.object(build, 'mod_dir', return_value=str(runtime)), \
                    patch.object(build, 'defaults_ini', return_value=None), \
                    patch.object(nexus_pack, 'OUT', str(root / 'nexus')):
                # Steam staging uses this same runtime filter directly.
                self.assertFalse(build.runtime_file_allowed(item['mod'], str(runtime), source_bundle.name))
                archive, count = nexus_pack.pack(item)

            with ZipFile(archive) as packaged:
                self.assertIsNone(packaged.testzip())
                self.assertEqual(count, 1)
                self.assertEqual(packaged.namelist(), ['RangedDamageXp/RangedDamageXp.dll'])
            self.assertTrue(source_bundle.exists())


if __name__ == '__main__':
    main()

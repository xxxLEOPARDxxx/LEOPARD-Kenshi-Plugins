import importlib.util, pathlib, tempfile, unittest
from unittest.mock import patch
from PIL import Image
path=pathlib.Path(__file__).resolve().parents[1]/'build.py'
spec=importlib.util.spec_from_file_location('release_build',path)
build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
class SquarePreview(unittest.TestCase):
    def test_missing_override_preserves_landscape(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(build,'HERE',tmp):
            self.assertEqual(build.steam_preview_source('Mod','original.png'),'original.png')
    def test_dedicated_square_is_selected(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(build,'HERE',tmp):
            target=pathlib.Path(tmp)/'steam-covers/Mod/000_LEOPARD_Steam_Cover.png';target.parent.mkdir(parents=True)
            Image.new('RGB',(16,16)).save(target)
            self.assertEqual(pathlib.Path(build.steam_preview_source('Mod','original.png')),target)
    def test_rectangle_in_square_folder_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(build,'HERE',tmp):
            target=pathlib.Path(tmp)/'steam-covers/Mod/000_LEOPARD_Steam_Cover.png';target.parent.mkdir(parents=True)
            Image.new('RGB',(16,9)).save(target)
            with self.assertRaises(ValueError):build.steam_preview_source('Mod','original.png')
if __name__=='__main__':unittest.main()

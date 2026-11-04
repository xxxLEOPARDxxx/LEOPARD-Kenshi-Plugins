import importlib.util
import pathlib
import struct
import tempfile
import unittest

HERE = pathlib.Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('gog_profile', HERE / 'keo_gog_profile.py')
PROFILE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PROFILE)


def image(machine=0x8664, magic=0x20b):
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, machine, 1, 0x1234, 0, 0, 240, 0x22)
    struct.pack_into('<H', data, 0x98, magic)
    struct.pack_into('<I', data, 0x98 + 16, 0x1000)
    struct.pack_into('<Q', data, 0x98 + 24, 0x140000000)
    struct.pack_into('<II', data, 0x98 + 56, 0x2000, 0x200)
    struct.pack_into('<8sIIIIIIHHI', data, 0x188, b'.text\0\0\0', 0x100, 0x1000, 0x200, 0x200, 0, 0, 0, 0, 0x60000020)
    data[0x200:0x220] = bytes(range(32))
    return data


class ProfileTests(unittest.TestCase):
    def test_valid_and_code_window(self):
        profile = PROFILE.parse_pe(image())
        self.assertEqual(profile['machine'], 0x8664)
        self.assertEqual(PROFILE.rva_bytes(image(), profile, 0x1000, 32), bytes(range(32)))

    def test_refuses_architecture_and_broken_images(self):
        for data in (b'', b'MZ', image(0x14c), image(magic=0x10b)):
            with self.assertRaises(ValueError):
                PROFILE.parse_pe(data)
        for offset, value in ((0x3c, 0xffffffff), (0x84 + 2, 0xffff), (0x188 + 20, 0xfffffff0)):
            data = image()
            if offset == 0x84 + 2:
                struct.pack_into('<H', data, offset, value)
            else:
                struct.pack_into('<I', data, offset, value)
            with self.assertRaises(ValueError):
                PROFILE.parse_pe(data)

    def test_windows_do_not_cross_section_or_accept_zero_fill(self):
        data = image()
        profile = PROFILE.parse_pe(data)
        for start, size in ((0xffff, 32), (0x11f0, 32), (0x1000, 65537), (0, 32), (-1, 1)):
            with self.assertRaises(ValueError):
                PROFILE.rva_bytes(data, profile, start, size)

    def test_unknown_executable_never_gog_accepted(self):
        self.assertEqual(PROFILE.identify_executable('df4a5a7ef8a29deb24b70e7b7f4a222a'), 'steam_1.0.65')
        self.assertEqual(PROFILE.identify_executable('213349bf76a0a758067f9ed0aef2ab01'), 'gog_1.0.65')
        self.assertEqual(PROFILE.identify_executable('0' * 32), 'unknown')

    def test_table_shape_and_rejects_zero_or_short(self):
        self.assertEqual(PROFILE.table_metadata(struct.pack('<II', 0x1000, 0))['entries'], 2)
        for raw in (b'', b'a', bytes(4 * 100001)):
            with self.assertRaises(ValueError):
                PROFILE.table_metadata(raw)

    def test_no_overwrite(self):
        with tempfile.TemporaryDirectory(dir=HERE) as task:
            target = pathlib.Path(task) / 'report.json'
            PROFILE.write_report(target, {'schema': 1})
            with self.assertRaises(FileExistsError):
                PROFILE.write_report(target, {})
            self.assertIn('schema', target.read_text())


if __name__ == '__main__':
    unittest.main()

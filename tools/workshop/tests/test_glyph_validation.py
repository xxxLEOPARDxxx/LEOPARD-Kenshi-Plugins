import importlib.util, pathlib, struct, tempfile, unittest
path=pathlib.Path(__file__).resolve().parents[1]/'build.py'
spec=importlib.util.spec_from_file_location('release_build',path)
build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
def mo(text):
    values={'':'Content-Type: text/plain; charset=UTF-8\n','tip':text}
    keys=sorted(values);offset=28+16*len(keys);tables=[];data=b''
    for entries in ([k.encode() for k in keys],[values[k].encode() for k in keys]):
        table=[]
        for value in entries:table.append((len(value),offset+len(data)));data+=value+b'\0'
        tables.append(b''.join(struct.pack('<II',*p) for p in table))
    return struct.pack('<7I',0x950412de,0,len(keys),28,28+8*len(keys),0,0)+b''.join(tables)+data
class GlyphValidation(unittest.TestCase):
    def run_catalog(self,text):
        with tempfile.TemporaryDirectory() as tmp:
            folder=pathlib.Path(tmp)/'locale/ru_RU/LC_MESSAGES';folder.mkdir(parents=True)
            (folder/'test.mo').write_bytes(mo(text))
            build.check_glyphs('Fixture',tmp)
    def test_runtime_mo_rejects_typographic_dash(self):
        with self.assertRaises(SystemExit):self.run_catalog('Слово — слово')
    def test_runtime_mo_accepts_game_font_hyphen(self):self.run_catalog('Слово - слово')
if __name__=='__main__':unittest.main()

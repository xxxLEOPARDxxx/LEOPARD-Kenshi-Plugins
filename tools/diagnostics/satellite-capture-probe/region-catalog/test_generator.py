"""Behavior tests for the actual isolated catalog generator; no game writes."""
from pathlib import Path
import copy, hashlib, importlib.util, json, subprocess, sys, tempfile, unittest
from pathlib import PureWindowsPath

HERE=Path(__file__).resolve().parent
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
MODULE=None
INPUTS=None
if (HERE/'generate_catalog.py').is_file():
    spec=importlib.util.spec_from_file_location('generate_catalog',HERE/'generate_catalog.py')
    MODULE=importlib.util.module_from_spec(spec);spec.loader.exec_module(MODULE)

class CatalogGeneratorTests(unittest.TestCase):
    def generator(self):
        self.assertIsNotNone(MODULE,'fixed regional generator is not implemented')
        return MODULE

    def data(self):
        global INPUTS
        g=self.generator()
        if INPUTS is None:INPUTS=g.load_pinned_inputs(HERE/'source-manifest.json',GAME)
        return INPUTS

    def test_64_regions_preserve_independent_literal_counts_and_coordinates(self):
        g=self.generator();inputs=self.data();doc=g.build_catalog(inputs)
        self.assertEqual((len(doc['assets']),len(doc['biomes']),len(doc['painted']),len(doc['regions'])),(219,57,128,64))
        self.assertEqual(max(len(r['biomes']) for r in doc['regions']),10)
        self.assertEqual(max(r['diffuse_count'] for r in doc['regions']),39)
        self.assertEqual(max(r['normal_count'] for r in doc['regions']),39)
        self.assertEqual(doc['regions'][0]['origin'],[-147456,-147456])
        self.assertEqual(doc['regions'][63]['origin'],[110592,110592])
        self.assertEqual(doc['regions'][9]['origin'],[-110592,-110592])
        self.assertEqual(doc['regions'][9]['paint_base'],[-147456,-147456])
        self.assertEqual(doc['regions'][18]['paint_base'],[-73728,-73728])

    def test_each_unused_native_source_slot_is_preserved_without_allocating_shader_row(self):
        g=self.generator();inputs=self.data();doc=g.build_catalog(inputs)
        cases=((1,0,'4415-Newwworld.mod'),(4,3,'2743-Newwworld.mod'),(2,4,'2990-Newwworld.mod'),
               (5,6,'3116-Newwworld.mod'),(6,7,'51632-Newwworld.mod'))
        for x,z,sid in cases:
            r=doc['regions'][z*8+x];global_code=next(i+1 for i,b in enumerate(doc['biomes']) if b['sid']==sid)
            self.assertTrue(any(global_code in c['slots'] for c in r['cells']),(x,z,sid))
            self.assertNotIn(global_code-1,[b['source'] for b in r['biomes']])
        self.assertEqual(sum(1 for r in doc['regions'] for c in r['cells'] for code in c['slots'] if code),
                         sum(1 for c in inputs['snapshot']['cells'] for sid in c['slots'] if sid))

    def test_padding_and_actual_alias_paths_remain_distinct(self):
        g=self.generator();doc=g.build_catalog(self.data());aliases=[]
        for r in doc['regions']:
            for begin,capacity,count in ((0,46,r['diffuse_count']),(46,46,r['normal_count'])):
                self.assertTrue(all(v==r['banks'][begin] for v in r['banks'][begin+count:begin+capacity]))
                self.assertEqual(len({doc['assets'][v]['sha256'] for v in r['banks'][begin:begin+count]}),count)
            for binding in r['biomes']:
                source=doc['biomes'][binding['source']]
                for kind in range(2):
                    for role in range(6):
                        a=source['sources'][kind][role];b=r['banks'][kind*46+binding['layers'][kind][role]]
                        self.assertEqual(doc['assets'][a]['sha256'],doc['assets'][b]['sha256'])
                        if a!=b:aliases.append((a,b))
        self.assertTrue(aliases,'actual duplicate-content paths must be represented, not erased')
        active={r['sid']:r for r in self.data()['active']['records'] if r.get('sid')}
        for biome in doc['biomes']:
            native=active[biome['sid']]['file']
            for kind,suffix in enumerate(('',' normal')):
                for role_index,role in enumerate(('base','slope','vertical','grass','dirt','road')):
                    expected=native.get('texture '+role+suffix) or native['texture base'+suffix]
                    expected='/'.join(PureWindowsPath(expected).parts).lower()
                    self.assertEqual(doc['assets'][biome['sources'][kind][role_index]]['path'],expected)

    def test_strict_dds_header_payload_hash_and_png_shape_refusals(self):
        g=self.generator();manifest=json.loads((HERE/'source-manifest.json').read_text())
        original=manifest['dds'][0];data=(GAME/original['path']).read_bytes()
        mutations=((4,0),(8,0),(12,0),(28,11),(84,0),(108,0))
        with tempfile.TemporaryDirectory(dir=HERE/'build') as td:
            p=Path(td)/'input.dds'
            for offset,value in mutations:
                changed=bytearray(data);changed[offset:offset+4]=value.to_bytes(4,'little');p.write_bytes(changed)
                entry=copy.deepcopy(original);entry['sha256']=hashlib.sha256(changed).hexdigest()
                with self.assertRaises(ValueError):g.verify_asset(p,entry,'dds')
            p.write_bytes(data[:-1]);entry=copy.deepcopy(original);entry['bytes']-=1;entry['sha256']=hashlib.sha256(data[:-1]).hexdigest()
            with self.assertRaises(ValueError):g.verify_asset(p,entry,'dds')
            changed=bytearray(data);changed[-1]^=1;p.write_bytes(changed)
            with self.assertRaises(ValueError):g.verify_asset(p,original,'dds')
            png=manifest['painted'][0];changed=bytearray((GAME/png['path']).read_bytes());changed[25]=2;p.write_bytes(changed)
            entry=copy.deepcopy(png);entry['sha256']=hashlib.sha256(changed).hexdigest()
            with self.assertRaises(ValueError):g.verify_asset(p,entry,'png')

    def test_path_traversal_and_snapshot_structure_mutations_refused(self):
        g=self.generator();inputs=self.data()
        for path in ('../outside.dds','D:/outside.dds','data/../outside.dds','/outside.dds','data//bad.dds'):
            with self.assertRaises(ValueError):g.safe_relative(path)
        for change in ('duplicate_cell','bad_tree','unknown_sid','out_of_world'):
            bad=copy.deepcopy(inputs)
            if change=='duplicate_cell':bad['snapshot']['cells'][1]=bad['snapshot']['cells'][0]
            elif change=='bad_tree':bad['snapshot']['cells'][0]['tree']='a1'
            elif change=='unknown_sid':bad['snapshot']['cells'][0]['slots'][0]='not-a-biome'
            else:bad['snapshot']['cells'][0]['x']=32
            with self.assertRaises(ValueError):g.build_catalog(bad)

    def test_output_determinism_and_failed_pin_does_not_change_existing_outputs(self):
        g=self.generator();doc=g.build_catalog(self.data())
        with tempfile.TemporaryDirectory(dir=HERE/'build') as td:
            out=Path(td)/'out';g.write_outputs(doc,out);before={p.name:p.read_bytes() for p in out.iterdir() if p.is_file()}
            g.write_outputs(doc,out);self.assertEqual(before,{p.name:p.read_bytes() for p in out.iterdir() if p.is_file()})
            bad=Path(td)/'bad.json';raw=json.loads((HERE/'source-manifest.json').read_text());raw['dds'][0]['sha256']='0'*64;bad.write_text(json.dumps(raw))
            run=subprocess.run([sys.executable,str(HERE/'generate_catalog.py'),'--manifest',str(bad),'--output',str(out)],capture_output=True)
            self.assertNotEqual(run.returncode,0)
            self.assertEqual(before,{p.name:p.read_bytes() for p in out.iterdir() if p.is_file()})

    def test_equivalent_manifest_or_snapshot_changes_still_refused(self):
        g=self.generator()
        with tempfile.TemporaryDirectory(dir=HERE/'build') as td:
            path=Path(td)/'manifest.json';path.write_bytes((HERE/'source-manifest.json').read_bytes()+b'\n')
            with self.assertRaises(ValueError):g.load_pinned_inputs(path,GAME)
            snapshot=Path(td)/'snapshot';snapshot.mkdir()
            (snapshot/'biome-selection-snapshot.json').write_bytes((g.SNAPSHOT/'biome-selection-snapshot.json').read_bytes()+b'\n')
            (snapshot/'active-biomes.json').write_bytes((g.SNAPSHOT/'active-biomes.json').read_bytes())
            with self.assertRaises(ValueError):g.load_pinned_inputs(HERE/'source-manifest.json',GAME,snapshot)

if __name__=='__main__':
    (HERE/'build').mkdir(exist_ok=True)
    unittest.main(verbosity=2)

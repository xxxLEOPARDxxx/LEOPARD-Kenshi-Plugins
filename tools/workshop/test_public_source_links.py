"""Regression for central MCM links without losing upstream credit/licensing."""
import json
from pathlib import Path
import unittest

from publication_sections import public_source_links, OUR_REPOSITORY_URL
from validate_descriptions import player_feature_claims

PLUGINS='https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins'
FORKS='https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks'
ORIGINAL='[b]Original source code:[/b] [url=https://github.com/Basusu0/Basusu-kenshi-plugins]Basusu sources[/url].'


class SourceLinkPolicy(unittest.TestCase):
    def test_original_credit_and_gpl_survive(self):
        for licence in ('GPLv3 licence.','GPL-3.0-or-later.','GPLv3; LICENSE and UPSTREAM.md are included.'):
            with self.subTest(licence=licence):
                source='Thank you, Basusu!\n'+ORIGINAL+'\n[b]Our source code:[/b] [url='+PLUGINS+']Our code[/url]. '+licence
                output=public_source_links('AIFixed',source)
                self.assertIn(ORIGINAL,output);self.assertIn('Thank you, Basusu!',output)
                self.assertIn(licence,output);self.assertNotIn('Our source code:',output)
                self.assertIsNone(OUR_REPOSITORY_URL.search(output))

    def test_link_label_and_empty_heading_are_removed(self):
        output=public_source_links('Formations','[h2]SOURCE CODE[/h2]\n\nThe source of this version is on [url='+FORKS+'/tree/main/Formations]GitHub[/url].')
        self.assertEqual(output.strip(),'')

    def test_russian_credit_and_licence_survive(self):
        source='Спасибо Basusu!\n[b]Наши исходники:[/b] [url='+PLUGINS+']Наш код[/url]. GPLv3; LICENSE включена.'
        output=public_source_links('AIFixed',source)
        self.assertIn('Спасибо Basusu!',output);self.assertIn('GPLv3; LICENSE включена.',output)
        self.assertNotIn('Наши исходники:',output)

    def test_mcm_is_byte_preserved_and_requires_both_repositories(self):
        source='Our repositories: [url='+PLUGINS+']Plugins[/url], [url='+FORKS+']Forks[/url].'
        self.assertEqual(public_source_links('ModConfigMenu',source),source)
        player_feature_claims('ModConfigMenu',source,'fixture')
        with self.assertRaises(ValueError):player_feature_claims('ModConfigMenu',PLUGINS,'fixture')

    def test_other_mod_rejects_our_namespace(self):
        for url in (PLUGINS,FORKS+'/tree/main/Formations',PLUGINS.upper()):
            with self.subTest(url=url):
                with self.assertRaises(ValueError):player_feature_claims('AIFixed',url,'fixture')

    def test_mixed_original_line_is_not_silently_destroyed(self):
        with self.assertRaises(ValueError):
            public_source_links('AIFixed',ORIGINAL+' [url='+PLUGINS+']Our code[/url]')

    def test_canonical_guides_are_clean_and_idempotent(self):
        guides=json.loads((Path(__file__).parent/'desc/qol-20261029.json').read_text('utf-8'))
        for mod,pair in guides.items():
            for lang,text in pair.items():
                with self.subTest(mod=mod,lang=lang):
                    self.assertEqual(public_source_links(mod,text),text)
                    player_feature_claims(mod,text,'canonical fixture')


if __name__=='__main__':unittest.main()

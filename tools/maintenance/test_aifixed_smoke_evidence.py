"""Check evidence kinds without pretending user confirmation is a log trace."""
import copy
import unittest
from verify_aifixed_and_37_20261102 import smoke_acceptance, EXPECTED, EXPECTED_RU, INSTALLED_AI

RU_PATH=INSTALLED_AI.parent/'locale/ru_RU/LC_MESSAGES/ai_fixed.mo'
PERCENT='Ремонт начинается ниже заданного процента. Износ отдельно.'
LABEL='Порог здоровья робочастей (%)'
CATALOG={'':'header','percent':PERCENT,'label':LABEL,**{'message'+str(i):'translation'+str(i) for i in range(40)}}
SMOKE={'candidateSha256':EXPECTED,'startupAndMcm':'PASS','responding':True,'loaded':[str(INSTALLED_AI)],
       'startupHooksVerified':True,'userPercentClarificationAccepted':True,
       'mcmEvidence':{'kind':'user_confirmation','text':'Сделал, проценты норм теперь объясняются'},
       'catalogs':{'ruPath':str(RU_PATH),'ruSha256':EXPECTED_RU,'ruMessageCount':42,
                   'percentageText':PERCENT,'thresholdLabel':LABEL},
       'log':['13 RE_Kenshi: AIFixed -> AIFixed.dll','14 RE_Kenshi: AIFixed: installed']}


def evaluate(smoke,**changes):
    arguments={'actual_dll_sha':EXPECTED,'ru_path':RU_PATH,'actual_ru_sha':EXPECTED_RU,'catalog':CATALOG}
    arguments.update(changes)
    return smoke_acceptance(smoke,**arguments)


class SmokeEvidence(unittest.TestCase):
    def test_user_confirmation_is_not_relabelled_as_logged_mcm(self):
        result=evaluate(copy.deepcopy(SMOKE))
        self.assertEqual(result['evidence_kind'],'startup_user_confirmation_and_catalog_inspection')
        self.assertFalse(result['logged_mcm_page']);self.assertFalse(result['logged_ru_catalog_summary'])
        self.assertEqual(result['user_confirmation'],SMOKE['mcmEvidence']['text'])

    def test_real_log_branch_is_distinct(self):
        smoke=copy.deepcopy(SMOKE);smoke.pop('mcmEvidence');smoke.pop('userPercentClarificationAccepted')
        smoke['log'] += ["20 ai_fixed: locale id='ru_RU', strings loaded=42",'20 MCM page=AIFixed']
        result=evaluate(smoke)
        self.assertEqual(result['evidence_kind'],'startup_and_logged_mcm_ru')

    def test_missing_explicit_user_acceptance_fails(self):
        for field in ('startupHooksVerified','userPercentClarificationAccepted'):
            smoke=copy.deepcopy(SMOKE);smoke[field]=False
            with self.subTest(field=field),self.assertRaises(AssertionError):evaluate(smoke)
        for field,value in (('kind','assumption'),('text','')):
            smoke=copy.deepcopy(SMOKE);smoke['mcmEvidence'][field]=value
            with self.subTest(field=field),self.assertRaises(AssertionError):evaluate(smoke)

    def test_stale_candidate_and_actual_file_fail(self):
        smoke=copy.deepcopy(SMOKE);smoke['candidateSha256']='old'
        with self.assertRaises(AssertionError):evaluate(smoke)
        with self.assertRaises(AssertionError):evaluate(SMOKE,actual_dll_sha='old')

    def test_catalog_hash_and_count_are_not_self_declared(self):
        with self.assertRaises(AssertionError):evaluate(SMOKE,actual_ru_sha='old')
        smoke=copy.deepcopy(SMOKE);smoke['catalogs']['ruSha256']='old'
        with self.assertRaises(AssertionError):evaluate(smoke)
        for count in (41,43):
            smoke=copy.deepcopy(SMOKE);smoke['catalogs']['ruMessageCount']=count
            with self.subTest(count=count),self.assertRaises(AssertionError):evaluate(smoke)
        shorter=dict(CATALOG);shorter.pop('label')
        with self.assertRaises(AssertionError):evaluate(SMOKE,catalog=shorter)

    def test_user_text_must_match_installed_catalog(self):
        smoke=copy.deepcopy(SMOKE);smoke['catalogs']['percentageText']='invented translation'
        with self.assertRaises(AssertionError):evaluate(smoke)

    def test_loaded_module_hooks_and_errors_are_checked(self):
        for key,value in (('loaded',['Other.dll']),('log',['AIFixed: hook incomplete']),('log',[]),('responding',False)):
            smoke=copy.deepcopy(SMOKE);smoke[key]=value
            with self.subTest(key=key,value=value),self.assertRaises(AssertionError):evaluate(smoke)


if __name__=='__main__':unittest.main()

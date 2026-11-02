"""Actual installed-source preprocessing/adapter contracts; no game calls."""
import hashlib
import unittest
import generate_water

class Sources(unittest.TestCase):
    def test_actual_native_conditional_source_and_narrow_adapters(self):
        evidence=generate_water.generate()
        text=(generate_water.OUT/'NativeWaterMaterial.hlsl').read_text()
        native=(generate_water.OUT/'native-preprocessed.hlsl').read_text()
        self.assertEqual(evidence['waterSha256'],'347e9c59ada723e6033cc8a875dd3e724ee8ba5549b4d8bac38ae3cd4265baa5')
        self.assertIn('float clipVal = dist - 4000',native)
        self.assertIn('saturate(tex2D(reflection,refTex).rrr + 1.0)',native)
        self.assertNotIn('depthmap',generate_water.function(native,'void waterFP'))
        self.assertNotIn('scummap',generate_water.function(native,'void waterFP'))
        self.assertNotIn('rainRipple(',generate_water.function(native,'void waterFP'))
        self.assertNotIn('tex2D(',text);self.assertNotIn('texCUBElod(',text)
        self.assertNotIn('clipVal',text);self.assertNotIn('reflection,',text)
        self.assertIn('float3 rf = 1.0f;',text)
        self.assertIn('SampleGrad',text)
        self.assertIn('NativeFlowDisplacement',text)
        self.assertIn('direction = speed > 0.0f ? direction / speed : float2(0,0);',text)
        for name in ['CalcPunctualLight','GetSpecularDominantDir','LazarovEnvironmentBRDF','sampleNormal','CalcEnvironmentLight']:
            self.assertIn(name,text)
        self.assertEqual(evidence['adapterSites'],{'distanceSelector':1,'reflectionFold':1,'stationaryGuard':1,'sampleNormal':1,'waterSamples':2,'cubeSamples':2})
    def test_refuse_native_source_drift(self):
        with self.assertRaises(ValueError):generate_water.verify_hash(b'changed native source','00'*32)

if __name__=='__main__':unittest.main(verbosity=2)

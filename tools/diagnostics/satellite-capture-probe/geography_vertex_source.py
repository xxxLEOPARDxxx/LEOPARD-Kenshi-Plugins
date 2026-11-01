"""Pinned native VS extraction shared by production embedding and WARP fixtures."""
from pathlib import Path
import hashlib
NATIVE=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials/deferred/terrain.hlsl')
SHA='7bde0c5f6aa81d542b5c25925b28508a937d5086effeb959a9b0c5b96f11eb07'
def native_vertex():
    raw=NATIVE.read_bytes();assert hashlib.sha256(raw).hexdigest()==SHA
    text=raw.decode().replace('\r\n','\n');start=text.index('void main_vs (');end=text.index('// ---------------------------------------------------------------------------------------------------------------------',start)
    return text[start:end]
def owned_vertex(helper):
    text=native_vertex()
    assert text.count('out float4 oTexV        : TEXCOORD5,')==1
    text=text.replace('out float4 oTexV        : TEXCOORD5,','out float4 oGeoCliff0 : TEXCOORD5,\n    out float4 oGeoCliff1 : TEXCOORD6,\n    out float4 oGeoCliff2 : TEXCOORD7,\n    out float4 oGeoCliff3 : TEXCOORD8,')
    start=text.index('\tuniform float4 distortion0,');end=text.index('\tuniform float4 biomeData,',start)
    text=text[:start]+'    uniform float4 geoDistortion[15],\n'+text[end:]
    start=text.index('\t// Wavy cliff lines');end=text.index('\n\t#endif',start)
    text=text[:start]+'    GeographyCliffHeights(position.xyz,geoDistortion,oGeoCliff0,oGeoCliff1,oGeoCliff2,oGeoCliff3);\n'+text[end:]
    assert len(text)<32768 and text.isascii()
    return helper+'\n'+text

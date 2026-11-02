struct V {float4 position:SV_Position;};
[maxvertexcount(3)]void main_gs(triangle V input[3],inout TriangleStream<V> outStream){for(uint i=0;i<3;++i){V v;v.position=float4(2,2,2,1);outStream.Append(v);}}
interface IColour {float4 Value();};
class SentinelColour:IColour {float4 Value(){return float4(1,0,1,1);}};
IColour activeColour;
float4 main_ps():SV_Target{return activeColour.Value();}

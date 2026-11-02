cbuffer WaterUniforms : register(b0) {
 float4 tileBounds;       // absolute minX,minZ,spanX,spanZ of selected GST2 cell
 float4 dispatchWindow;   // helper-owned width,height,0,0 of this <=512 block
 float4 sourceWindow;     // full selected side,block originX,block originZ,0
 float4 waterPlane;       // current ABSOLUTE water Y,explicit enable0/1,0,0
 float4 nativeMapBounds;  // actual waterDistant mapBounds minX,minZ,maxX,maxZ
 float4 normalScale;      // actual waterDistant VP scale
 float4 waterSunDirection;
 float4 waterSunColour;
 float4 waterEnvColour;
 float4 materialParams;   // actual invStrength,distortion.x,gloss,glow
 float4 waterTime;        // current native gameTime,0,0,0
 float4 parallelView;     // map surface-to-camera world direction
};

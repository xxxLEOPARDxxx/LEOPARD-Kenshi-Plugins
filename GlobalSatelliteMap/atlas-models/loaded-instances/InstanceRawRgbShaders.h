#pragma once
#include "InstanceRaster.h"
#include "_generated/InstanceShaders.h"
#include "_generated/InstanceRgbShaders.h"
namespace LoadedInstances {
// Private output-only variants; callers still need the complete active-source
// proof. This supplies no permission to assert Snapshot verifiedSources=true.
inline ShaderSet RawRgbShaders(){ShaderSet s;s.vertex[Dual43]=Bytecode(InstanceVP83,sizeof(InstanceVP83));s.vertex[Alpha48]=Bytecode(InstanceVP80,sizeof(InstanceVP80));s.pixel[Dual43]=Bytecode(InstanceFP43Rgb,sizeof(InstanceFP43Rgb));s.pixel[Alpha48]=Bytecode(InstanceFP48Rgb,sizeof(InstanceFP48Rgb));return s;}
}

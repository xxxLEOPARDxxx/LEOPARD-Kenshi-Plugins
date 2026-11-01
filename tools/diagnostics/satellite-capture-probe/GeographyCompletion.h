#pragma once
namespace SatelliteGeography
{
// Allocation, upload and native accounting are one uncertain operation.
template<class Operations> bool CompleteBank(Operations& ops,bool& uncertain)
{
    if(uncertain||!ops.Available())return false;
    uncertain=true;
    ops.Create();
    if(!ops.Preflight()||!ops.Upload())return false;
    ops.Notify();
    ops.SetLoaded();
    if(!ops.Loaded())return false;
    uncertain=false;return true;
}
}

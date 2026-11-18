#pragma once
#include <cfloat>
namespace AIFixedEngineerSurplus
{
    struct Admission
    {
        bool enabled,normalJobCall,exactBuilder,owned,eligible,epochStable;
        Admission():enabled(false),normalJobCall(false),exactBuilder(false),owned(false),eligible(false),epochStable(false){}
    };
    // Native permajob ordering uses its own descending slot key; this only
    // admits an otherwise inactive exact builder with a proven pending return.
    inline float Admit(float nativeScore,const Admission& a)
    {
        if(nativeScore>0 || nativeScore!=nativeScore || nativeScore < -FLT_MAX)return nativeScore;
        return a.enabled&&a.normalJobCall&&a.exactBuilder&&a.owned&&a.eligible&&a.epochStable ? 1.0f : nativeScore;
    }
    inline int Surplus(int proven,int needed)
    {return proven>0&&needed>=0&&proven>needed?proven-needed:0;}
    enum TransferResult { NoTransfer, Delivered, RolledBack, KeptPending };
    template<class Ops> TransferResult TransferOne(Ops& op)
    {
        if(!op.CanStart())return NoTransfer;
        typename Ops::Piece piece=op.Detach();
        if(!piece)return NoTransfer;
        // Reserve a receipt slot BEFORE Detach; Keep records the native-owned
        // detached piece before any further callback can fail or reset a world.
        op.Keep(piece);
        if(!op.SameEpoch())return KeptPending;
        if(op.PutDestination(piece)){op.Commit();return Delivered;}
        if(!op.SameEpoch())return KeptPending;
        if(op.PutBack(piece)){op.Restored();return RolledBack;}
        return KeptPending;
    }
}

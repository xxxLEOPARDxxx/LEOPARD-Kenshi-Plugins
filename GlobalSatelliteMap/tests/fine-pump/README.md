# Ready-only fine pump budget,2026-11-04

GsmFinePump::Budget admits at most8serialized steps within1.5ms in supplied QPC
ticks. An unchanged before/after snapshot stops immediately. It never sleeps,
queries time, waits for a worker or calls an engine function. Individual already
entered work is not preempted; the elapsed budget is checked before the next
step, so this is not a hard limit on one native operation's duration.

FineCoordinator::ObserveProgress exposes fieldwise Main-owned state plus atomic
source/file worker readiness. It does not allocate, mutate state or authenticate
a result. The renderer must also include its own phase/read/pair/cache-revision
changes and defer shade/presentation to once after the batch. No-progress waits
must return to the ordinary next workspace callback, not retry in a busy loop.

`run.py`: actual purepolicy26 and explicit snapshot12controls perVC10/Od/O2,
including step8,1.5ms deadline, zero/low frequency, time reversal and64bit overflow;
each snapshot field independently changes equality. The actual NativeSession
controller getter also compiles/O2/GL. Host checks do not prove a game FPS or
fivefold loading improvement; that requires live measurement.

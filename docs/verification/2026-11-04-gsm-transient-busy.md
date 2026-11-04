# Fine terrain: transient native contention and known joins

Private development repair; no public version or publication.

The live D4598 build stopped fine terrain permanently with ERROR_BUSY (0x800700AA).
Read-only process evidence is in research/gsm-runtime-followup-20261104 outside
this repository. The native source was Failed, but its worker, thread and query
were already gone and it was not quarantined. The outer coordinator incorrectly
treated the historical operation result as an unknown resource lifetime.

Changes:

- Retry exact ERROR_BUSY from native metadata Accept and CommitBank next frame.
  Preserve the current input and source lease; do not spin or accept stale data.
- Retry capture contention only at Idle with zero reserved source GPU memory.
  Keep transient retry state separate from the first fatal operation result.
- Release proven joined, GPU-empty native owners even when their operation failed.
  Keep retaining genuinely unknown, quarantined or borrowed owners.
- Report fatal fine-owner shutdown errors once instead of silently disabling
  refinement. Normal cancellation remains a normal lifecycle completion.

Verification on 2026-11-04:

- Extracted production native busy/join bodies: 32 controls, VC10 /Od and /O2.
- Extracted coordinator queue/shutdown bodies: 53 controls, /Od and /O2;
  retained pre-fix RED evidence.
- Existing prefetch policy/caller: 39 controls, /Od and /O2, three negative mutants.
- VC10 /GL /LTCG DLL: 86 native imports, startPlugin and C MCM exports passed.
- Independent review of coordinator, source retries and Main integration completed
  before installation. No new worker, world scan, synchronous wait or cache growth.

Candidate SHA256: 30dda6d71173ac339b812f89ab4842d510f5d5cc19e35d226d06e916edf34022.
Live acceptance and the separate seam readback remain pending at this checkpoint.

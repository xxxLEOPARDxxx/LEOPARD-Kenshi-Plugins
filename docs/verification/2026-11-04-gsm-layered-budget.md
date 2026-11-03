# Layered viewer startup regression repair

The first loaded-town candidate regressed previously accepted x6/x8 terrain.
Actual game PID32836 reported viewerGpu=34,143,518, sourceGpu=0, fineGpu=0,
phase0/spacings9. The old viewer-only32MiB cap rejected the new2,884,080B city
stages. This was an admission defect, not evidence of insufficient machine VRAM.

The current development profile now gives the viewer/layers40MiB within the
unchanged aggregate512MiB. The startup gate also stops incorrectly charging
retained fine images against that viewer-only cap. It checks actual physical
slot/byte bounds, pending/retired debt and the real K2 producer reservation using
overflow-safe subtraction. Actual MakeBudget and upload/fence checks remain.

After an owner ends, fresh identity creation now retires old-stamp fine images
before any new key is published. A refused retirement blocks identity advance;
retired bytes remain charged until their real completion fence. Ordinary pans
within an owner retain their accepted warm-cache behaviour.

Verification:

- Native VC10 /GL /LTCG build passes86-import/startPlugin/MCM checks.
- Candidate SHA2567f765299e067b1833acbe6aa6f48ef60cd8047ca46cadb364e984860b8729f2a,
  1,226,752B. Log: build/GlobalSatelliteMap/layered-budget-build.log.
- Actual startup header plus extracted actual MakeBudget/native reservation:
  24 controls per/Od,/O2, including actual observed7,880,725B external CPU,
  full256 fine slots, pending/retired/overflow and physical boundaries.
- Actual native CPU reservation45,077,658 +other9,971,365 =55,049,023B,
  below unchanged64MiB. Full fine cache +40MiB viewer +K2 =515,058,740B,
  below unchanged536,870,912B aggregate.
- Actual FineJob planner warm-cache regression20 controls per/Od,/O2,
  including x6/x8 dimensions, observed layered viewer and maximum town reserve.
- Extracted actual FineObserve identity-retirement prefix261 controls per
  optimization, including active-worker refusal and failed-retirement recovery.
- Legacy32 mode reproduces the old rejection; independent review CLEAR.
- No new loop, per-frame allocation or game-world traversal added to admission;
  identity loop is bounded256 and only executes after an owner ends or the
  world/capture identity changes.

Live image acceptance remains pending installation/restart. This does not prove
long-session FPS/memory or support for lower-end GPUs. The512MiB development
profile excludes Kenshi/driver allocations; coherent selectable smaller budgets
and constrained-hardware acceptance are explicitly recorded before public release.
No atlas rerender, personal setting change or public release is part of this repair.

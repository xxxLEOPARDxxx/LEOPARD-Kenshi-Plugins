# Worker-to-GPU terrain-window integration core

Root implemented a one-window diagnostic controller joining the independently
reviewed components. Its current frozen header is
`6cfcfe1680810c28ea5cb692c510c2d6790f8d734ed63e742e95fa53aa14de0f`,
manifest935ba3f7fd8e638aad8ed9d21529b69200bb2c8aa92e6125594eb96e43163019.

Fresh root checks complete two256 frames byte-exact against frozen material,
normal and radial planes, plus a512/1m material observation. One exact TIFF
authentication is reused across those successful jobs. Temporary shader memory
can change after Submit because bytecode is copied. OFF drains the retained
source bank/TIFF/slab, and world replacement cancels before publication.

Independent review found and verified fixes for partial-init fence ordering,
unscoped output access, worker termination/wake errors, asynchronous shader
lifetime, retained-cache OFF handling and destructor quarantine ownership.
Unknown completion now retains a whole heap owner and conservative112,825,820B
GPU reservation, including partial-init retired debt; new worker starts refuse.
Review: `research/gsm-world-stream-review-20261103/report.md`.

The review's isolated fresh run passes12 lifecycle assertions each/Od and/O2,
three real frames and440 focused flow/fault assertions, with destructor-loss and
under-reservation mutants rejected at runtime. Poll-inclusive assertion totals
vary with worker timing; semantic outcomes and pixel comparisons do not.

This is not yet installed or wired to Main. The host test uses fake native
metadata, with real file leases/WIC/WARP afterward. A checked actual native
frame/source adapter, validated radial-to-world-height conversion, full-world
batching and fine-cache scheduling remain required. Raw radial output must not
be written as GST2 height. One transition per frame for4096 windows would be too
slow; grouped/budgeted scheduling remains a production acceptance condition.

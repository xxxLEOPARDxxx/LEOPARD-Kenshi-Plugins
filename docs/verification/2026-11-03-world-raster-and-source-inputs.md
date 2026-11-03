# Reusable world rendering and authenticated source inputs

The reusable raster supports256/512 outputs, ordered SID contributions and the
fixed global mesh under the shared192MiB ledger. Root independently reviewed and
reran the frozen suite; report:
`research/gsm-world-raster-review-20261103/report.md`.

The source receipt library has independently reviewed native FileSystem metadata
selection, worker file leases, sealed acceptance and whole-bank revalidation.
The actual native API/frame adapter remains unexecuted in-game; host tests do not
establish current-world authority. Review:
`research/gsm-source-receipts-review-20261103/report.md`.

The WIC decoder matches all129 actual PNG images against independent Pillow RGBA
pins. Independent review found and verified fixes for translation-unit-local pin
identity and cancellation after the final decoded hash. The final suite passes
660checks plus5 runtime mutants; independent controls cover COM/fault cleanup,
200 cancellation points and copied/forged profile identities. Review:
`research/gsm-png-source-review-20261103/report.md`.

Frozen manifests: raster9d140c2b98189028d32abbec434bd48875fef2e12787b5108a585f992f82a7ae,
receiptsf7e7aa94d38dd7af18d6da29cd8694a42a63d075e6ad4eac91746380070ff2d7,
decoder931297e052082cc0c604a4829f6190cb18b6cd1b54bc7ab395e4ec9e43c1edd4.

World-stream is the separate, still-reviewed integration candidate. Its real
Win32/WIC/WARP tests complete two256 frames matching allthree frozen planes and
one512/1m view, with one TIFF authentication across those jobs. Its test metadata
provider is deliberately fake; this does not replace a native frame acceptance
test. No game installation or release is implied by these commits.

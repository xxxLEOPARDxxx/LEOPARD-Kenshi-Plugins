# Geographic biome trial: native compressed mip correction and runtime proof

The private standalone 512px primary1,1 trial now loads all 15 biome records,
92 DDS inputs, 39 ordered coarse layouts and three compressed array banks with
nine actual mip levels. This is a material/ownership acceptance step, not the
completed global satellite map. Distortion/fade are disabled for isolation;
water, buildings/LODs, local lights and the final map viewer are not accepted.

## Failed request and correction

PID14656, request1509 refused before the first compressed upload because native
Ogre reduced the requested additional mip count by two: request8 produced seven
levels, while our strict uploader required nine. Live read-only inspection of
the quarantined texture and its native SRV confirmed the mismatch. The process
retained one uncertain private set and disabled retries until exit. It did not
silently accept a partial chain or relax the budget. The user closed that game.

Commit f0de24a requests10 additional levels to obtain nine actual levels. The
strict resource/SRV descriptor checks and budgets are unchanged. The actual
native terrain arrays examined had seven levels; these private nine-level
banks are intentional and do not establish native mip parity.

The capture controller now recognizes `retained until process exit`,
`cleanup refused` and geography refusal as terminal errors instead of waiting
for the full timeout. It derives the geographic scope from the success log.

## Installed and measured

Installed DLL334336 bytes, SHA256
`47d2fd8d90218a690068a3168284f43f5a16904be0b5b92085d023708fa0c8c4`.
INI bytes were preserved. Backup/installation record:
`game/_codex_work/global-satellite-20261102/before-bc-request-compensation/installation.json`.

Fresh process4348 startUTC2026-11-01T21:48:41.2128061Z, user-confirmed SAT scene
paused with global map open. File requests1510/1511 completed in3299/2834ms.
Both logged three geographic banks/nine actual levels and DDS GPU385874496B.
Each request joined/closed one native worker, released all six owned RTV
creation references to zero and left scene0/workspaces1. Texture accounting
2256363398B and material accounting6879135B returned exactly before/after each.
Five fixed shader identities were cached on the first request and stayed stable
on the second; privateNames5 are that cache, not five retained texture banks.

Raw GBuffer and linear Sun images are pixel-identical across repeats. Exposed
images differ in1522/262144 pixels, by at most one channel level. Root inspected
the actual saved preview: different terrain/biome regions are present. Correct
blend/cliff boundaries and agreement with native material rendering still need
separate review; a visually varied image alone does not prove them.

Process-private bytes before/after were8444952576/8677457920 and
8463609856/8678621184. The second baseline returned close to the first baseline,
but these short observations do not prove zero leaks or steady-state FPS.

Evidence under
`game/_codex_work/global-satellite-20261101/rtv-lifetime-20261101/`:

- `512-geography-bc-fixed-first-20261102/{capture.json,request.log,memory-samples.json}`
- `512-geography-bc-fixed-second-20261102/{capture.json,request.log,memory-samples.json}`
- `geography-bc-fixed-proof-20261102.json`
- `geography-bc-fixed-preview-20261102.png`

Actual native render ABI verification passed again after installation. Offline
WARP/source/ownership checks are recorded in the probe's geography verification
report. No save files or main minimap assets were changed by these captures.

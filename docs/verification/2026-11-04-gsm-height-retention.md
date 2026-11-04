# Fine terrain: retain the authenticated height handle between completed pairs

Private development repair, not a public release.

The previous ordinary drain called the terminal worker Close command, discarding
the deny-write/delete authenticated fullmap.tif handle. Every next cold pair
therefore read and hashed all 536,980,180 bytes again. At 170 pairs, this implies
91.29 GB of repeated authentication work, not measured physical disk traffic.

The completed-pair drain now trims sample/input/raw buffers and drains the same
GPU owners while retaining only the original authenticated file object. It is
admitted only after successful Idle/WIdle, no borrowed accepted view and no
pending GPU query. Source path/epoch changes reauthenticate through the unchanged
height-source implementation. Soft/terminal/context cancellation uses full Close;
a hard cancellation superseding an in-flight trim sends and awaits full Close.

Independent root review checked source identity, locked handle ownership,
consumer ordering and GPU-zero read/upload gates. No growing cache, extra worker
or world scan is added. Existing source scratch reservation stays conservative.

Fresh verification:

- Extracted actual production worker on a real thread with real tiny locked
  files: ordinary reuse, write/delete protection, epoch/path replacement, empty
  payloads, cancellation/Stop/join, and extracted controller ordering all passed.
- Legacy ordinary Close path failed the retained-authentication regression.
- Existing busy/join 32 and shutdown 53 controls passed in /Od and /O2.
- Native VC10 /GL /LTCG DLL, 86 imports and entry/MCM exports passed.

Debug-only heightAuthentications reports cumulative successful height opens at
Idle/Complete. End-to-end speed and actual count after several cold pairs still
require the installed game measurement. The regional material seam is separate.

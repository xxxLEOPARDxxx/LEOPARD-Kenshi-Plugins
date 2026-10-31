# Publication artwork

Nexus uses landscape `workshop/images/<Mod>/000_LEOPARD_Cover.png`.
Steam uses a separately composed square
`workshop/steam-covers/<Mod>/000_LEOPARD_Steam_Cover.png`.
`build.py` selects the square only for Steam's primary preview; gameplay
screenshots and Nexus artwork retain their original paths. A non-square
override fails preparation instead of silently publishing the wrong format.

The 2026-10-31 series contains 37 published mods. Dynamic Factions is excluded.
Titles are larger for Workshop thumbnails. Dark UI Advanced has no MCM badge.
Vital Sense depicts status icons, never overhead health bars. Mod Config Menu
has a distinctive central command-hub design, including a matching landscape
Nexus companion requested by the user. Its earlier landscape is archived
outside the posting folders.

Images were created with the built-in ImageGen tool. JSON manifests retain
the exact prompts, references, output paths, dimensions and SHA-256 hashes.
The scripts copy original generated pixels; staging only performs the
existing JPEG compression needed for Steam's preview-size limit.

`prepare_steam_squares.py` stages preview files without rebuilding content or
changing item metadata. Publish with `steam_upload.py preview <Mod ...>`;
never use content upload or refresh for artwork-only changes. Run
`verify_steam_squares.py baseline` before publication and `verify` afterwards
to check exact CDN bytes, square dimensions and unchanged non-preview fields.
The staged and remote evidence is saved under `workshop/steam-covers`.

Code updates still follow Nexus first, Steam afterwards. Artwork does not
increment a release version.

The later feature-first revision replaces the shared ochre palette with
distinct palettes and a large, accurate feature illustration per mod. Five
user-approved square concepts are reused; every landscape companion is
separately composed. `2026-10-31-feature-series.json` records the final 74
assets and their actual generation inputs. `finalize_feature_series.py`
validates all assets before copying them, archives the earlier series outside
posting folders, and creates a paired gallery and a 37-image Nexus ZIP under
`workshop/cover-series/2026-10-31-features/`. Nexus publication remains manual.

Visual checks include single coloured equipment-grade stars (no kit grade),
XP percentages, an actual ranged-damage target, the skeleton repair wrench,
the MCM badge on supporting plugins and the no-MCM interface-skin exception.

# Workshop release preparation

Versioned sources for the local release tools. Copy these files, preserving `desc/`, into D:/DEV/Kenshi/workshop before running them there with the bundled Python runtime (Pillow is required). Generated packages/images and personal INI files stay outside this repository.

Run `prepare_release.py ModName ...` in that working folder. It refreshes README, Steam and Nexus descriptions, screenshot folders and CRC-verified ZIPs. Releases contain one runtime catalog per language; compiled-MO plugins ship MO only. Obsolete ZIPs for the same mod are removed only after the replacement succeeds.

Maintain full player descriptions in `desc/qol-20261029.json`: purpose, features, controls, settings and compatibility, with matching Russian and English detail. Keep the existing BBCode headings, lists and bold feature names. Forks credit the original author and link the original project before their changes. Technical implementation details belong in README; MCM's SDK section is the exception.

Write both languages of each current version's changes in `release_notes.json`. Preparation places a visible version section in both descriptions and updates `changelog.json` while retaining older versions. The history is synchronized with its versioned copy. Every mod/fork README also describes current-version changes in English and Russian.

`validate_descriptions.py ModName ...` checks full generated bodies, both languages and every version-note item in Steam/Nexus outputs, balanced BBCode, matching changelog and Steam byte limits. Preparation runs it before replacing ZIPs.

Preparation and validation never upload. `steam_upload.py` is the existing publisher, now versioned here too: it sends the English description, then a separate Russian translation. Upload attaches one bilingual changelog to the first submission; the translation adds no duplicate note. Missing changelogs fail explicitly. Do not run the publisher during preparation. The user uploads Nexus first and supplies links; the corresponding Steam release follows afterwards. Maintain these tracked sources and synchronize the working copy when changing release behavior.

One prepared release increments the published number once. Fixes before publication retain that number. MCM is published as9; all current changes belong to10. Its `published_version` baseline guards this rule. API v6 is independent of the product version.

`fork_catalog.json` separates the original author/project/source, our cumulative changes, and the actual history of our fork. `publication_sections.py` renders explicitly labelled thanks and original-source links plus every verified fork version in both languages. Original functionality remains in the player guide and is not presented as newly created fork work. A first release has only v1; do not invent versions. Source shipped with an original download is labelled honestly when no upstream repository is verified.

`mcm_catalog.json` contains verified registration names, source evidence and support status. Validation scans built plugin exports without loading DLLs and fails if a canonical MCM plugin is absent from this catalogue. Prepared, local-only, disabled and experimental entries must remain distinguishable.

For publication text changes alone, run `refresh_descriptions.py Mod ...`: it regenerates both Steam languages and the combined Nexus file without rebuilding content, replacing DLLs/screens or changing versions. Full `prepare_release.py` uses the same text renderer and checks.

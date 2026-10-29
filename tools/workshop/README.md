# Workshop release preparation

Versioned sources for the local release tools. Copy these files, preserving `desc/`, into D:/DEV/Kenshi/workshop before running them there with the bundled Python runtime (Pillow is required). Generated packages/images and personal INI files stay outside this repository.

Run `prepare_release.py ModName ...` in that working folder. It refreshes README, Steam and Nexus descriptions, screenshot folders and CRC-verified ZIPs. Releases contain one runtime catalog per language; compiled-MO plugins ship MO only. Obsolete ZIPs for the same mod are removed only after the replacement succeeds.

No tool here uploads anything. The user uploads Nexus first and supplies links; the corresponding Steam release follows afterwards. Maintain these tracked sources and synchronize the working copy when changing release behavior.

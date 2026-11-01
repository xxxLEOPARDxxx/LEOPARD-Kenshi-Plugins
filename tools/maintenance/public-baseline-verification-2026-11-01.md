# Missing release baseline verification, 2026-11-01

Read-only official Steam public metadata check. The explicit description version footer was read for all eight packages missing from the local ledger. Consumer app 233860 and creator 76561198098571264 were checked. No local publication metadata, packages, installations or remote content were changed. Nexus versions were not checked.

| Package | Steam description version | Checked UTC | Official item | Description SHA256 |
| --- | --- | --- | --- | --- |
| BuildModeGizmo | 4 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3813761881) | `9a7237ef53d33b021c93bf2e6309ccdb73e2dce143008e372804dee11b559082` |
| GearCompare | 7 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3812150687) | `fb6a04151674210fef40cddb4b1a3e502e44c9e76cfc911b9bb95ce2f891280f` |
| ItemMarkers | 5 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3816608451) | `9482d1833e12a13a112ac5048a1b935a406cf3777a5f51bd71a203828315efdf` |
| LiveXpBars | 11 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3812150449) | `d4993236df30099db00ab1d48a64fe465fe44dc5335663eff76a9975922033c3` |
| QuickStartSelect | 6 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3813761725) | `e9adf46d82683bd47056843b16eec86fe733bbcb47f5a4ce2f436ee4c2cf313d` |
| RangedDamageXp | 2 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3813761935) | `3594ec48f00853adc6131e26817ec1d5506582c978f4addffe21035dcc61fd84` |
| StatColours | 12 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3812150560) | `1db6182106a770e923e0cb609a2dec23c85ae91f6b1cf83c3f0f0238d2db4244` |
| WantedMap | 6 | 2026-11-01T20:49:34.607149+00:00 | [Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=3813761812) | `8cb281479bf63d941935a00677c84d570c077a05efee47e95d5ed06c5e562341` |

Official API: [GetPublishedFileDetails](https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/). Raw response: `D:\DEV\Kenshi\build\ReleaseBaselineAudit\steam-public-file-details.json`; SHA256 `a5f40cf710256e106236a5f087c0339f896049ab45c1306992a4973517688f80`.

These version footers support the recorded release-number baseline; they do not verify the contents of every downloadable binary or establish Nexus synchronization. Preparing new packages still follows the user’s Nexus-first, Steam-after-links publication order.

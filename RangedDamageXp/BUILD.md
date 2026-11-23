# Building Ranged Damage Xp from the v4 source bundle

The DLL uses the Visual C++ 2010 x64 ABI. Use Visual C++ 2010, Windows SDK 7.1,
Boost 1.60.0 and the x64 `KenshiLib.lib` and `OgreMain_x64.lib` import libraries.
The bundle includes the exact KenshiLib interface headers used for the release.
The release links against the KenshiLib 0.5.0 runtime used by RE_Kenshi.
The import libraries are available from the [KenshiLib project and its SDK links](https://github.com/KenshiReclaimer/KenshiLib#developing-re_kenshikenshilib-plugins).

```powershell
.\RangedDamageXp\build.ps1 -VcDir <VC-directory> -WindowsSdkDir <SDK-directory> -BoostDir <Boost-directory> -ImportLibDir <x64-library-directory>
```

Use GNU `msgfmt` on each PO file under `RangedDamageXp/locale` to build the three
runtime catalogues. The bundled `RangedDamageXp.ini` contains the factory values;
install it as `RangedDamageXp.defaults.ini`. See `ORIGIN.md` and `LICENSE` for
attribution and redistribution terms.

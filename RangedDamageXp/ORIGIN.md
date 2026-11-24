# Ranged Damage Xp

Ranged Damage Xp is distributed under GPL-3.0-or-later. See LICENSE.

The bolt trail timing, geometry and opacity mask were adapted from MeitouClient
by VlcekM and the MeitouClient contributors, at commit `c7a5b87e8abcdeb7d448a4274071597306f82eb5`.

- [Bolt streak timing and geometry](https://github.com/VlcekM/MeitouClient/blob/c7a5b87e8abcdeb7d448a4274071597306f82eb5/src/Meitou.Game/Effects/Debris.cs#L479)
- [Tracer texture](https://github.com/VlcekM/MeitouClient/blob/c7a5b87e8abcdeb7d448a4274071597306f82eb5/src/Meitou.Game/Rendering/Particles/ParticleRenderer.cs)
- [Upstream license](https://github.com/VlcekM/MeitouClient/blob/c7a5b87e8abcdeb7d448a4274071597306f82eb5/LICENSE)

Adapted to C++ and Kenshi/Ogre in November 2026. The port samples the native
projectile update and adds a bounded trail history and MCM controls. Meitou's
NVIDIA runtime exception is not used.

The [version 4 plugin source and build instructions](https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins/tree/76189c800a1663098c6e44b019c5899dc448774c/RangedDamageXp)
are available separately. The same revision contains the shared plugin headers.

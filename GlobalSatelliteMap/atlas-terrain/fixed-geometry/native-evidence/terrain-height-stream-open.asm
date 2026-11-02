000141c0 4889542410                 mov       qword ptr [rsp + 0x10], rdx
000141c5 48894c2408                 mov       qword ptr [rsp + 8], rcx
000141ca 4881ec98000000             sub       rsp, 0x98
000141d1 48c7842480000000feffffff   mov       qword ptr [rsp + 0x80], 0xfffffffffffffffe
000141dd 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
000141e5 0f57c0                     xorps     xmm0, xmm0
000141e8 0f2e80cc010000             ucomiss   xmm0, dword ptr [rax + 0x1cc]
000141ef 7567                       jne       0x180014258
000141f1 488d15882d0200             lea       rdx, [rip + 0x22d88]
000141f8 488d4c2430                 lea       rcx, [rsp + 0x30]
000141fd e8ee1c0000                 call      0x180015ef0
00014202 90                         nop
00014203 488d15fe2f0200             lea       rdx, [rip + 0x22ffe]
0001420a 488d4c2458                 lea       rcx, [rsp + 0x58]
0001420f e8dc1c0000                 call      0x180015ef0
00014214 90                         nop
00014215 c744242831010000           mov       dword ptr [rsp + 0x28], 0x131
0001421d 488d05942e0200             lea       rax, [rip + 0x22e94]
00014224 4889442420                 mov       qword ptr [rsp + 0x20], rax
00014229 4c8d4c2430                 lea       r9, [rsp + 0x30]
0001422e 4c8d442458                 lea       r8, [rsp + 0x58]
00014233 ba01000000                 mov       edx, 1
00014238 b901000000                 mov       ecx, 1
0001423d e88ef5ffff                 call      0x1800137d0
00014242 90                         nop
00014243 488d4c2458                 lea       rcx, [rsp + 0x58]
00014248 e8e3d2ffff                 call      0x180011530
0001424d 488d4c2430                 lea       rcx, [rsp + 0x30]
00014252 e8d9d2ffff                 call      0x180011530
00014257 90                         nop
00014258 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
00014260 4883b8b801000000           cmp       qword ptr [rax + 0x1b8], 0
00014268 740d                       je        0x180014277
0001426a 488b8c24a0000000           mov       rcx, qword ptr [rsp + 0xa0]
00014272 e8e9000000                 call      0x180014360
00014277 488b8c24a8000000           mov       rcx, qword ptr [rsp + 0xa8]
0001427f e8dc1c0000                 call      0x180015f60
00014284 ba01000000                 mov       edx, 1
00014289 488bc8                     mov       rcx, rax
0001428c e8efc50000                 call      0x180020880
00014291 488b8c24a0000000           mov       rcx, qword ptr [rsp + 0xa0]
00014299 488981b8010000             mov       qword ptr [rcx + 0x1b8], rax
000142a0 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
000142a8 4883b8b801000000           cmp       qword ptr [rax + 0x1b8], 0
000142b0 7507                       jne       0x1800142b9
000142b2 32c0                       xor       al, al
000142b4 e980000000                 jmp       0x180014339
000142b9 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
000142c1 f30f1080cc010000           movss     xmm0, dword ptr [rax + 0x1cc]
000142c9 f30f5e052f2f0200           divss     xmm0, dword ptr [rip + 0x22f2f]
000142d1 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
000142d9 f30f1180d0010000           movss     dword ptr [rax + 0x1d0], xmm0
000142e1 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
000142e9 488b88b8010000             mov       rcx, qword ptr [rax + 0x1b8]
000142f0 e85b000000                 call      0x180014350
000142f5 83e0fe                     and       eax, 0xfffffffe
000142f8 8bc0                       mov       eax, eax
000142fa 660fefc0                   pxor      xmm0, xmm0
000142fe f3480f2ac0                 cvtsi2ss  xmm0, rax
00014303 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
0001430b f30f5e80c8010000           divss     xmm0, dword ptr [rax + 0x1c8]
00014313 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
0001431b f30f1180d8010000           movss     dword ptr [rax + 0x1d8], xmm0
00014323 488b8424a0000000           mov       rax, qword ptr [rsp + 0xa0]
0001432b 488b88c0010000             mov       rcx, qword ptr [rax + 0x1c0]
00014332 e829e0feff                 call      0x180002360
00014337 b001                       mov       al, 1
00014339 4881c498000000             add       rsp, 0x98

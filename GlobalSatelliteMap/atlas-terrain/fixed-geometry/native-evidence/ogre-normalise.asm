00067070 f30f106104                 movss     xmm4, dword ptr [rcx + 4]
00067075 f30f1019                   movss     xmm3, dword ptr [rcx]
00067079 f30f106908                 movss     xmm5, dword ptr [rcx + 8]
0006707e 0f28d3                     movaps    xmm2, xmm3
00067081 0f28c4                     movaps    xmm0, xmm4
00067084 0f28cd                     movaps    xmm1, xmm5
00067087 f30f59d3                   mulss     xmm2, xmm3
0006708b f30f59c4                   mulss     xmm0, xmm4
0006708f f30f59cd                   mulss     xmm1, xmm5
00067093 f30f58d0                   addss     xmm2, xmm0
00067097 f30f58d1                   addss     xmm2, xmm1
0006709b f30f51c2                   sqrtss    xmm0, xmm2
0006709f 0f2f05da296400             comiss    xmm0, dword ptr [rip + 0x6429da]
000670a6 7626                       jbe       0x1800670ce
000670a8 f30f100da0296400           movss     xmm1, dword ptr [rip + 0x6429a0]
000670b0 f30f5ec8                   divss     xmm1, xmm0
000670b4 f30f59d9                   mulss     xmm3, xmm1
000670b8 f30f59e1                   mulss     xmm4, xmm1
000670bc f30f59e9                   mulss     xmm5, xmm1
000670c0 f30f1119                   movss     dword ptr [rcx], xmm3
000670c4 f30f116104                 movss     dword ptr [rcx + 4], xmm4
000670c9 f30f116908                 movss     dword ptr [rcx + 8], xmm5
000670ce f3c3                       repz ret
000670d0 f30f101598296400           movss     xmm2, dword ptr [rip + 0x642998]
000670d8 f30f1001                   movss     xmm0, dword ptr [rcx]
000670dc f30f104908                 movss     xmm1, dword ptr [rcx + 8]
000670e1 488bc2                     mov       rax, rdx
000670e4 0f57c2                     xorps     xmm0, xmm2
000670e7 0f57ca                     xorps     xmm1, xmm2
000670ea f30f1102                   movss     dword ptr [rdx], xmm0
000670ee f30f104104                 movss     xmm0, dword ptr [rcx + 4]
000670f3 f30f114a08                 movss     dword ptr [rdx + 8], xmm1
000670f8 0f57c2                     xorps     xmm0, xmm2
000670fb f30f114204                 movss     dword ptr [rdx + 4], xmm0
00067100 c3                         ret
00067101 cc                         int3
00067102 cc                         int3
00067103 cc                         int3
00067104 cc                         int3
00067105 cc                         int3
00067106 cc                         int3
00067107 cc                         int3
00067108 cc                         int3
00067109 cc                         int3
0006710a cc                         int3
0006710b cc                         int3
0006710c cc                         int3
0006710d cc                         int3
0006710e cc                         int3
0006710f cc                         int3

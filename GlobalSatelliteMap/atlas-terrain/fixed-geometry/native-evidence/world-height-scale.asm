00014070 f30f11542418               movss     dword ptr [rsp + 0x18], xmm2
00014076 f30f114c2410               movss     dword ptr [rsp + 0x10], xmm1
0001407c 48894c2408                 mov       qword ptr [rsp + 8], rcx
00014081 4883ec58                   sub       rsp, 0x58
00014085 488b442460                 mov       rax, qword ptr [rsp + 0x60]
0001408a f30f10442468               movss     xmm0, dword ptr [rsp + 0x68]
00014090 f30f1180c8010000           movss     dword ptr [rax + 0x1c8], xmm0
00014098 488b442460                 mov       rax, qword ptr [rsp + 0x60]
0001409d f30f10442470               movss     xmm0, dword ptr [rsp + 0x70]
000140a3 f30f1180cc010000           movss     dword ptr [rax + 0x1cc], xmm0
000140ab f30f10442468               movss     xmm0, dword ptr [rsp + 0x68]
000140b1 0f5705781e0200             xorps     xmm0, xmmword ptr [rip + 0x21e78]
000140b8 f30f5e05f02e0200           divss     xmm0, dword ptr [rip + 0x22ef0]
000140c0 488b442460                 mov       rax, qword ptr [rsp + 0x60]
000140c5 f30f1180d4010000           movss     dword ptr [rax + 0x1d4], xmm0
000140cd 488b442460                 mov       rax, qword ptr [rsp + 0x60]
000140d2 f30f1098d4010000           movss     xmm3, dword ptr [rax + 0x1d4]
000140da 0f57d2                     xorps     xmm2, xmm2
000140dd 488b442460                 mov       rax, qword ptr [rsp + 0x60]
000140e2 f30f1088d4010000           movss     xmm1, dword ptr [rax + 0x1d4]
000140ea 488d4c2440                 lea       rcx, [rsp + 0x40]
000140ef ff1573170200               call      qword ptr [rip + 0x21773] ; ??0Vector3@Ogre@@QEAA@MMM@Z
000140f5 4c8bc0                     mov       r8, rax
000140f8 488b442460                 mov       rax, qword ptr [rsp + 0x60]
000140fd f30f1088c8010000           movss     xmm1, dword ptr [rax + 0x1c8]
00014105 488b442460                 mov       rax, qword ptr [rsp + 0x60]
0001410a 488b88c0010000             mov       rcx, qword ptr [rax + 0x1c0]
00014111 e86ad5feff                 call      0x180001680
00014116 488b442460                 mov       rax, qword ptr [rsp + 0x60]
0001411b f30f1080d4010000           movss     xmm0, dword ptr [rax + 0x1d4]
00014123 f30f58442468               addss     xmm0, dword ptr [rsp + 0x68]
00014129 488b442460                 mov       rax, qword ptr [rsp + 0x60]
0001412e f30f1088d4010000           movss     xmm1, dword ptr [rax + 0x1d4]
00014136 f30f584c2468               addss     xmm1, dword ptr [rsp + 0x68]
0001413c 488b442460                 mov       rax, qword ptr [rsp + 0x60]
00014141 480590010000               add       rax, 0x190
00014147 f30f11442430               movss     dword ptr [rsp + 0x30], xmm0
0001414d f30f10442470               movss     xmm0, dword ptr [rsp + 0x70]
00014153 f30f11442428               movss     dword ptr [rsp + 0x28], xmm0
00014159 f30f114c2420               movss     dword ptr [rsp + 0x20], xmm1
0001415f 488b4c2460                 mov       rcx, qword ptr [rsp + 0x60]
00014164 f30f1099d4010000           movss     xmm3, dword ptr [rcx + 0x1d4]
0001416c 0f57d2                     xorps     xmm2, xmm2
0001416f 488b4c2460                 mov       rcx, qword ptr [rsp + 0x60]
00014174 f30f1089d4010000           movss     xmm1, dword ptr [rcx + 0x1d4]
0001417c 488bc8                     mov       rcx, rax
0001417f ff15db110200               call      qword ptr [rip + 0x211db] ; ?setExtents@AxisAlignedBox@Ogre@@QEAAXMMMMMM@Z
00014185 4883c458                   add       rsp, 0x58
00014189 c3                         ret

000047a0 48894c2408                 mov       qword ptr [rsp + 8], rcx
000047a5 56                         push      rsi
000047a6 57                         push      rdi
000047a7 4881ec48030000             sub       rsp, 0x348
000047ae 48c7842498020000feffffff   mov       qword ptr [rsp + 0x298], 0xfffffffffffffffe
000047ba 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000047c2 488b00                     mov       rax, qword ptr [rax]
000047c5 8b80ec000000               mov       eax, dword ptr [rax + 0xec]
000047cb 898424bc000000             mov       dword ptr [rsp + 0xbc], eax
000047d2 660f6e8424bc000000         movd      xmm0, dword ptr [rsp + 0xbc]
000047db 0f5bc0                     cvtdq2ps  xmm0, xmm0
000047de f30f5c05ea140300           subss     xmm0, dword ptr [rip + 0x314ea]
000047e6 f30f118424a0020000         movss     dword ptr [rsp + 0x2a0], xmm0
000047ef 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000047f7 480588000000               add       rax, 0x88
000047fd 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00004805 4881c1ac000000             add       rcx, 0xac
0000480c 4c8bc0                     mov       r8, rax
0000480f 488d942460010000           lea       rdx, [rsp + 0x160]
00004817 ff1533100300               call      qword ptr [rip + 0x31033] ; ??GVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000481d f30f108424a0020000         movss     xmm0, dword ptr [rsp + 0x2a0]
00004826 0f28d0                     movaps    xmm2, xmm0
00004829 488d9424a8000000           lea       rdx, [rsp + 0xa8]
00004831 488bc8                     mov       rcx, rax
00004834 ff156e0f0300               call      qword ptr [rip + 0x30f6e] ; ??KVector3@Ogre@@QEBA?AV01@M@Z
0000483a c78424b80000000a000000     mov       dword ptr [rsp + 0xb8], 0xa
00004845 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
0000484c 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00004854 6bc00a                     imul      eax, eax, 0xa
00004857 4898                       cdqe
00004859 48898424a8020000           mov       qword ptr [rsp + 0x2a8], rax
00004861 b804000000                 mov       eax, 4
00004866 488b8c24a8020000           mov       rcx, qword ptr [rsp + 0x2a8]
0000486e 48f7e1                     mul       rcx
00004871 48c7c1ffffffff             mov       rcx, 0xffffffffffffffff
00004878 480f40c1                   cmovo     rax, rcx
0000487c 488bc8                     mov       rcx, rax
0000487f e81cd80100                 call      0x1800220a0
00004884 4889842470010000           mov       qword ptr [rsp + 0x170], rax
0000488c 488b842470010000           mov       rax, qword ptr [rsp + 0x170]
00004894 4889442430                 mov       qword ptr [rsp + 0x30], rax
00004899 f30f108424a8000000         movss     xmm0, dword ptr [rsp + 0xa8]
000048a2 f30f590596160300           mulss     xmm0, dword ptr [rip + 0x31696]
000048aa 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000048b2 f30f118084000000           movss     dword ptr [rax + 0x84], xmm0
000048ba 488d4c2420                 lea       rcx, [rsp + 0x20]
000048bf ff154b0f0300               call      qword ptr [rip + 0x30f4b] ; ??0Vector3@Ogre@@QEAA@XZ
000048c5 c78424a400000000000000     mov       dword ptr [rsp + 0xa4], 0
000048d0 48c744243800000000         mov       qword ptr [rsp + 0x38], 0
000048d9 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000048e1 4883787000                 cmp       qword ptr [rax + 0x70], 0
000048e6 0f849e000000               je        0x18000498a
000048ec 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000048f4 488b4070                   mov       rax, qword ptr [rax + 0x70]
000048f8 4883b8d800000000           cmp       qword ptr [rax + 0xd8], 0
00004900 0f8484000000               je        0x18000498a
00004906 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
0000490e 488b00                     mov       rax, qword ptr [rax]
00004911 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00004919 488b4970                   mov       rcx, qword ptr [rcx + 0x70]
0000491d 48898890020000             mov       qword ptr [rax + 0x290], rcx
00004924 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
0000492c 488b4070                   mov       rax, qword ptr [rax + 0x70]
00004930 488b80d8000000             mov       rax, qword ptr [rax + 0xd8]
00004937 4889442438                 mov       qword ptr [rsp + 0x38], rax
0000493c c78424a400000000000000     mov       dword ptr [rsp + 0xa4], 0
00004947 eb10                       jmp       0x180004959
00004949 8b8424a4000000             mov       eax, dword ptr [rsp + 0xa4]
00004950 ffc0                       inc       eax
00004952 898424a4000000             mov       dword ptr [rsp + 0xa4], eax
00004959 83bc24a400000004           cmp       dword ptr [rsp + 0xa4], 4
00004961 7d27                       jge       0x18000498a
00004963 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
0000496b 488b4070                   mov       rax, qword ptr [rax + 0x70]
0000496f 48638c24a4000000           movsxd    rcx, dword ptr [rsp + 0xa4]
00004977 488b942460030000           mov       rdx, qword ptr [rsp + 0x360]
0000497f 483954c850                 cmp       qword ptr [rax + rcx*8 + 0x50], rdx
00004984 7502                       jne       0x180004988
00004986 eb02                       jmp       0x18000498a
00004988 ebbf                       jmp       0x180004949
0000498a 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00004992 488b00                     mov       rax, qword ptr [rax]
00004995 4805b8000000               add       rax, 0xb8
0000499b 488bc8                     mov       rcx, rax
0000499e e88d530000                 call      0x180009d30
000049a3 4885c0                     test      rax, rax
000049a6 7426                       je        0x1800049ce
000049a8 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
000049b0 e8ab490000                 call      0x180009360
000049b5 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
000049bd 488b09                     mov       rcx, qword ptr [rcx]
000049c0 4881c1b8000000             add       rcx, 0xb8
000049c7 8bd0                       mov       edx, eax
000049c9 e8c2520000                 call      0x180009c90
000049ce 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000049d6 488b00                     mov       rax, qword ptr [rax]
000049d9 4883c018                   add       rax, 0x18
000049dd 488bd0                     mov       rdx, rax
000049e0 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
000049e8 e8034c0000                 call      0x1800095f0
000049ed 90                         nop
000049ee 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000049f6 488b00                     mov       rax, qword ptr [rax]
000049f9 480540020000               add       rax, 0x240
000049ff 41b001                     mov       r8b, 1
00004a02 488bd0                     mov       rdx, rax
00004a05 488d4c2440                 lea       rcx, [rsp + 0x40]
00004a0a e801d3ffff                 call      0x180001d10
00004a0f 90                         nop
00004a10 c784240001000000000000     mov       dword ptr [rsp + 0x100], 0
00004a1b eb10                       jmp       0x180004a2d
00004a1d 8b842400010000             mov       eax, dword ptr [rsp + 0x100]
00004a24 ffc0                       inc       eax
00004a26 89842400010000             mov       dword ptr [rsp + 0x100], eax
00004a2d 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004a34 39842400010000             cmp       dword ptr [rsp + 0x100], eax
00004a3b 0f8d44030000               jge       0x180004d85
00004a41 660f6e842400010000         movd      xmm0, dword ptr [rsp + 0x100]
00004a4a 0f5bc0                     cvtdq2ps  xmm0, xmm0
00004a4d f30f598424a8000000         mulss     xmm0, dword ptr [rsp + 0xa8]
00004a56 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00004a5e f30f108888000000           movss     xmm1, dword ptr [rax + 0x88]
00004a66 f30f58c8                   addss     xmm1, xmm0
00004a6a 0f28c1                     movaps    xmm0, xmm1
00004a6d f30f11442420               movss     dword ptr [rsp + 0x20], xmm0
00004a73 c784240401000000000000     mov       dword ptr [rsp + 0x104], 0
00004a7e eb10                       jmp       0x180004a90
00004a80 8b842404010000             mov       eax, dword ptr [rsp + 0x104]
00004a87 ffc0                       inc       eax
00004a89 89842404010000             mov       dword ptr [rsp + 0x104], eax
00004a90 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004a97 39842404010000             cmp       dword ptr [rsp + 0x104], eax
00004a9e 0f8ddc020000               jge       0x180004d80
00004aa4 8b842404010000             mov       eax, dword ptr [rsp + 0x104]
00004aab 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00004ab3 8b8c2400010000             mov       ecx, dword ptr [rsp + 0x100]
00004aba 03c8                       add       ecx, eax
00004abc 8bc1                       mov       eax, ecx
00004abe 6bc00a                     imul      eax, eax, 0xa
00004ac1 4898                       cdqe
00004ac3 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00004ac8 488d0481                   lea       rax, [rcx + rax*4]
00004acc 4889842408010000           mov       qword ptr [rsp + 0x108], rax
00004ad4 660f6e842404010000         movd      xmm0, dword ptr [rsp + 0x104]
00004add 0f5bc0                     cvtdq2ps  xmm0, xmm0
00004ae0 f30f598424b0000000         mulss     xmm0, dword ptr [rsp + 0xb0]
00004ae9 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00004af1 f30f108890000000           movss     xmm1, dword ptr [rax + 0x90]
00004af9 f30f58c8                   addss     xmm1, xmm0
00004afd 0f28c1                     movaps    xmm0, xmm1
00004b00 f30f11442428               movss     dword ptr [rsp + 0x28], xmm0
00004b06 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004b0e f30f10442420               movss     xmm0, dword ptr [rsp + 0x20]
00004b14 f30f1100                   movss     dword ptr [rax], xmm0
00004b18 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004b20 f30f10442428               movss     xmm0, dword ptr [rsp + 0x28]
00004b26 f30f114008                 movss     dword ptr [rax + 8], xmm0
00004b2b 48837c243800               cmp       qword ptr [rsp + 0x38], 0
00004b31 0f8433010000               je        0x180004c6a
00004b37 8b842400010000             mov       eax, dword ptr [rsp + 0x100]
00004b3e 83e001                     and       eax, 1
00004b41 85c0                       test      eax, eax
00004b43 0f8521010000               jne       0x180004c6a
00004b49 8b842404010000             mov       eax, dword ptr [rsp + 0x104]
00004b50 83e001                     and       eax, 1
00004b53 85c0                       test      eax, eax
00004b55 0f850f010000               jne       0x180004c6a
00004b5b 8b8424a4000000             mov       eax, dword ptr [rsp + 0xa4]
00004b62 83e001                     and       eax, 1
00004b65 85c0                       test      eax, eax
00004b67 7415                       je        0x180004b7e
00004b69 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004b70 99                         cdq
00004b71 2bc2                       sub       eax, edx
00004b73 d1f8                       sar       eax, 1
00004b75 898424b0020000             mov       dword ptr [rsp + 0x2b0], eax
00004b7c eb0b                       jmp       0x180004b89
00004b7e c78424b002000000000000     mov       dword ptr [rsp + 0x2b0], 0
00004b89 8b8424a4000000             mov       eax, dword ptr [rsp + 0xa4]
00004b90 83e002                     and       eax, 2
00004b93 85c0                       test      eax, eax
00004b95 7415                       je        0x180004bac
00004b97 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004b9e 99                         cdq
00004b9f 2bc2                       sub       eax, edx
00004ba1 d1f8                       sar       eax, 1
00004ba3 898424b4020000             mov       dword ptr [rsp + 0x2b4], eax
00004baa eb0b                       jmp       0x180004bb7
00004bac c78424b402000000000000     mov       dword ptr [rsp + 0x2b4], 0
00004bb7 8b842400010000             mov       eax, dword ptr [rsp + 0x100]
00004bbe 99                         cdq
00004bbf 2bc2                       sub       eax, edx
00004bc1 d1f8                       sar       eax, 1
00004bc3 038424b0020000             add       eax, dword ptr [rsp + 0x2b0]
00004bca 898424b8020000             mov       dword ptr [rsp + 0x2b8], eax
00004bd1 8b842404010000             mov       eax, dword ptr [rsp + 0x104]
00004bd8 99                         cdq
00004bd9 2bc2                       sub       eax, edx
00004bdb d1f8                       sar       eax, 1
00004bdd 038424b4020000             add       eax, dword ptr [rsp + 0x2b4]
00004be4 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00004bec 8b8c24b8020000             mov       ecx, dword ptr [rsp + 0x2b8]
00004bf3 03c8                       add       ecx, eax
00004bf5 8bc1                       mov       eax, ecx
00004bf7 89842410010000             mov       dword ptr [rsp + 0x110], eax
00004bfe 8b842410010000             mov       eax, dword ptr [rsp + 0x110]
00004c05 6bc00a                     imul      eax, eax, 0xa
00004c08 ffc0                       inc       eax
00004c0a 4898                       cdqe
00004c0c 488b8c2408010000           mov       rcx, qword ptr [rsp + 0x108]
00004c14 488b542438                 mov       rdx, qword ptr [rsp + 0x38]
00004c19 f30f100482                 movss     xmm0, dword ptr [rdx + rax*4]
00004c1e f30f114124                 movss     dword ptr [rcx + 0x24], xmm0
00004c23 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004c2b 488b8c2408010000           mov       rcx, qword ptr [rsp + 0x108]
00004c33 f30f104124                 movss     xmm0, dword ptr [rcx + 0x24]
00004c38 f30f114004                 movss     dword ptr [rax + 4], xmm0
00004c3d 8b842410010000             mov       eax, dword ptr [rsp + 0x110]
00004c44 6bc00a                     imul      eax, eax, 0xa
00004c47 83c003                     add       eax, 3
00004c4a 4898                       cdqe
00004c4c 488b8c2408010000           mov       rcx, qword ptr [rsp + 0x108]
00004c54 488b542438                 mov       rdx, qword ptr [rsp + 0x38]
00004c59 488d7918                   lea       rdi, [rcx + 0x18]
00004c5d 488d3482                   lea       rsi, [rdx + rax*4]
00004c61 b90c000000                 mov       ecx, 0xc
00004c66 f3a4                       rep movsb byte ptr [rdi], byte ptr [rsi]
00004c68 eb53                       jmp       0x180004cbd
00004c6a 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00004c72 448b4078                   mov       r8d, dword ptr [rax + 0x78]
00004c76 488d542420                 lea       rdx, [rsp + 0x20]
00004c7b 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00004c83 e8184a0000                 call      0x1800096a0
00004c88 f30f118424bc020000         movss     dword ptr [rsp + 0x2bc], xmm0
00004c91 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004c99 f30f108424bc020000         movss     xmm0, dword ptr [rsp + 0x2bc]
00004ca2 f30f114024                 movss     dword ptr [rax + 0x24], xmm0
00004ca7 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004caf f30f108424bc020000         movss     xmm0, dword ptr [rsp + 0x2bc]
00004cb8 f30f114004                 movss     dword ptr [rax + 4], xmm0
00004cbd 8b842404010000             mov       eax, dword ptr [rsp + 0x104]
00004cc4 8b8c2400010000             mov       ecx, dword ptr [rsp + 0x100]
00004ccb 03c8                       add       ecx, eax
00004ccd 8bc1                       mov       eax, ecx
00004ccf 85c0                       test      eax, eax
00004cd1 743e                       je        0x180004d11
00004cd3 f30f105c2428               movss     xmm3, dword ptr [rsp + 0x28]
00004cd9 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004ce1 f30f105004                 movss     xmm2, dword ptr [rax + 4]
00004ce6 f30f104c2420               movss     xmm1, dword ptr [rsp + 0x20]
00004cec 488d8c2478010000           lea       rcx, [rsp + 0x178]
00004cf4 ff156e0b0300               call      qword ptr [rip + 0x30b6e] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004cfa 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00004d02 4883c108                   add       rcx, 8
00004d06 488bd0                     mov       rdx, rax
00004d09 ff15a10a0300               call      qword ptr [rip + 0x30aa1] ; ?merge@AxisAlignedBox@Ogre@@QEAAXAEBVVector3@2@@Z
00004d0f eb6a                       jmp       0x180004d7b
00004d11 f30f105c2428               movss     xmm3, dword ptr [rsp + 0x28]
00004d17 488b842408010000           mov       rax, qword ptr [rsp + 0x108]
00004d1f f30f105004                 movss     xmm2, dword ptr [rax + 4]
00004d24 f30f104c2420               movss     xmm1, dword ptr [rsp + 0x20]
00004d2a 488d8c2484010000           lea       rcx, [rsp + 0x184]
00004d32 ff15300b0300               call      qword ptr [rip + 0x30b30] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004d38 48898424c0020000           mov       qword ptr [rsp + 0x2c0], rax
00004d40 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00004d48 4883c108                   add       rcx, 8
00004d4c ff15660a0300               call      qword ptr [rip + 0x30a66] ; ?getMaximum@AxisAlignedBox@Ogre@@QEAAAEAVVector3@2@XZ
00004d52 488b8c24c0020000           mov       rcx, qword ptr [rsp + 0x2c0]
00004d5a 488bd1                     mov       rdx, rcx
00004d5d 488bc8                     mov       rcx, rax
00004d60 ff15120b0300               call      qword ptr [rip + 0x30b12] ; ??4Vector3@Ogre@@QEAAAEAV01@AEBV01@@Z
00004d66 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00004d6e 4883c108                   add       rcx, 8
00004d72 488bd0                     mov       rdx, rax
00004d75 ff15450a0300               call      qword ptr [rip + 0x30a45] ; ?setMinimum@AxisAlignedBox@Ogre@@QEAAXAEBVVector3@2@@Z
00004d7b e900fdffff                 jmp       0x180004a80
00004d80 e998fcffff                 jmp       0x180004a1d
00004d85 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00004d8d 488b00                     mov       rax, qword ptr [rax]
00004d90 48c7809002000000000000     mov       qword ptr [rax + 0x290], 0
00004d9b 4c8b0d6e0a0300             mov       r9, qword ptr [rip + 0x30a6e] ; ??0Vector3@Ogre@@QEAA@XZ
00004da2 41b806000000               mov       r8d, 6
00004da8 ba0c000000                 mov       edx, 0xc
00004dad 488d4c2450                 lea       rcx, [rsp + 0x50]
00004db2 e849c2ffff                 call      0x180001000
00004db7 488d8c24c0000000           lea       rcx, [rsp + 0xc0]
00004dbf ff154b0a0300               call      qword ptr [rip + 0x30a4b] ; ??0Vector3@Ogre@@QEAA@XZ
00004dc5 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004dcc ffc8                       dec       eax
00004dce 898424a0000000             mov       dword ptr [rsp + 0xa0], eax
00004dd5 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004ddc 6bc00a                     imul      eax, eax, 0xa
00004ddf 4898                       cdqe
00004de1 48898424d0000000           mov       qword ptr [rsp + 0xd0], rax
00004de9 8b05096a0400               mov       eax, dword ptr [rip + 0x46a09]
00004def 83e001                     and       eax, 1
00004df2 85c0                       test      eax, eax
00004df4 0f85bc000000               jne       0x180004eb6
00004dfa 8b05f8690400               mov       eax, dword ptr [rip + 0x469f8]
00004e00 83c801                     or        eax, 1
00004e03 8905ef690400               mov       dword ptr [rip + 0x469ef], eax
00004e09 f30f101d8b0f0300           movss     xmm3, dword ptr [rip + 0x30f8b]
00004e11 0f57d2                     xorps     xmm2, xmm2
00004e14 0f57c9                     xorps     xmm1, xmm1
00004e17 488d0d92690400             lea       rcx, [rip + 0x46992]
00004e1e ff15440a0300               call      qword ptr [rip + 0x30a44] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004e24 f30f101d700f0300           movss     xmm3, dword ptr [rip + 0x30f70]
00004e2c 0f57d2                     xorps     xmm2, xmm2
00004e2f f30f100d990e0300           movss     xmm1, dword ptr [rip + 0x30e99]
00004e37 488d0d7e690400             lea       rcx, [rip + 0x4697e]
00004e3e ff15240a0300               call      qword ptr [rip + 0x30a24] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004e44 0f57db                     xorps     xmm3, xmm3
00004e47 0f57d2                     xorps     xmm2, xmm2
00004e4a f30f100d7e0e0300           movss     xmm1, dword ptr [rip + 0x30e7e]
00004e52 488d0d6f690400             lea       rcx, [rip + 0x4696f]
00004e59 ff15090a0300               call      qword ptr [rip + 0x30a09] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004e5f f30f101d690e0300           movss     xmm3, dword ptr [rip + 0x30e69]
00004e67 0f57d2                     xorps     xmm2, xmm2
00004e6a 0f57c9                     xorps     xmm1, xmm1
00004e6d 488d0d60690400             lea       rcx, [rip + 0x46960]
00004e74 ff15ee090300               call      qword ptr [rip + 0x309ee] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004e7a f30f101d4e0e0300           movss     xmm3, dword ptr [rip + 0x30e4e]
00004e82 0f57d2                     xorps     xmm2, xmm2
00004e85 f30f100d0f0f0300           movss     xmm1, dword ptr [rip + 0x30f0f]
00004e8d 488d0d4c690400             lea       rcx, [rip + 0x4694c]
00004e94 ff15ce090300               call      qword ptr [rip + 0x309ce] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004e9a 0f57db                     xorps     xmm3, xmm3
00004e9d 0f57d2                     xorps     xmm2, xmm2
00004ea0 f30f100df40e0300           movss     xmm1, dword ptr [rip + 0x30ef4]
00004ea8 488d0d3d690400             lea       rcx, [rip + 0x4693d]
00004eaf ff15b3090300               call      qword ptr [rip + 0x309b3] ; ??0Vector3@Ogre@@QEAA@MMM@Z
00004eb5 90                         nop
00004eb6 c784241401000000000000     mov       dword ptr [rsp + 0x114], 0
00004ec1 eb10                       jmp       0x180004ed3
00004ec3 8b842414010000             mov       eax, dword ptr [rsp + 0x114]
00004eca ffc0                       inc       eax
00004ecc 89842414010000             mov       dword ptr [rsp + 0x114], eax
00004ed3 83bc241401000006           cmp       dword ptr [rsp + 0x114], 6
00004edb 7d4e                       jge       0x180004f2b
00004edd 4863842414010000           movsxd    rax, dword ptr [rsp + 0x114]
00004ee5 486bc00c                   imul      rax, rax, 0xc
00004ee9 488d0dc0680400             lea       rcx, [rip + 0x468c0]
00004ef0 4803c8                     add       rcx, rax
00004ef3 488bc1                     mov       rax, rcx
00004ef6 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
00004efe 488d942490010000           lea       rdx, [rsp + 0x190]
00004f06 488bc8                     mov       rcx, rax
00004f09 ff15b9080300               call      qword ptr [rip + 0x308b9] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
00004f0f 48638c2414010000           movsxd    rcx, dword ptr [rsp + 0x114]
00004f17 486bc90c                   imul      rcx, rcx, 0xc
00004f1b 488d4c0c50                 lea       rcx, [rsp + rcx + 0x50]
00004f20 488bd0                     mov       rdx, rax
00004f23 ff154f090300               call      qword ptr [rip + 0x3094f] ; ??4Vector3@Ogre@@QEAAAEAV01@AEBV01@@Z
00004f29 eb98                       jmp       0x180004ec3
00004f2b c784241801000000000000     mov       dword ptr [rsp + 0x118], 0
00004f36 eb10                       jmp       0x180004f48
00004f38 8b842418010000             mov       eax, dword ptr [rsp + 0x118]
00004f3f ffc0                       inc       eax
00004f41 89842418010000             mov       dword ptr [rsp + 0x118], eax
00004f48 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004f4f 39842418010000             cmp       dword ptr [rsp + 0x118], eax
00004f56 0f8dea060000               jge       0x180005646
00004f5c c784241c01000000000000     mov       dword ptr [rsp + 0x11c], 0
00004f67 eb10                       jmp       0x180004f79
00004f69 8b84241c010000             mov       eax, dword ptr [rsp + 0x11c]
00004f70 ffc0                       inc       eax
00004f72 8984241c010000             mov       dword ptr [rsp + 0x11c], eax
00004f79 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00004f80 3984241c010000             cmp       dword ptr [rsp + 0x11c], eax
00004f87 0f8db4060000               jge       0x180005641
00004f8d 8b84241c010000             mov       eax, dword ptr [rsp + 0x11c]
00004f94 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00004f9c 8b8c2418010000             mov       ecx, dword ptr [rsp + 0x118]
00004fa3 03c8                       add       ecx, eax
00004fa5 8bc1                       mov       eax, ecx
00004fa7 6bc00a                     imul      eax, eax, 0xa
00004faa ffc0                       inc       eax
00004fac 89842424010000             mov       dword ptr [rsp + 0x124], eax
00004fb3 4863842424010000           movsxd    rax, dword ptr [rsp + 0x124]
00004fbb 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00004fc0 f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
00004fc5 f30f11842420010000         movss     dword ptr [rsp + 0x120], xmm0
00004fce 83bc241c01000000           cmp       dword ptr [rsp + 0x11c], 0
00004fd6 7e28                       jle       0x180005000
00004fd8 4863842424010000           movsxd    rax, dword ptr [rsp + 0x124]
00004fe0 482b8424d0000000           sub       rax, qword ptr [rsp + 0xd0]
00004fe8 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00004fed f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
00004ff2 f30f118424c8020000         movss     dword ptr [rsp + 0x2c8], xmm0
00004ffb e989000000                 jmp       0x180005089
00005000 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
00005008 488d9424a8010000           lea       rdx, [rsp + 0x1a8]
00005010 488d0d99670400             lea       rcx, [rip + 0x46799]
00005017 ff15ab070300               call      qword ptr [rip + 0x307ab] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000501d 48898424d0020000           mov       qword ptr [rsp + 0x2d0], rax
00005025 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
0000502d 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
00005032 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
00005037 488bd1                     mov       rdx, rcx
0000503a 488d8c249c010000           lea       rcx, [rsp + 0x19c]
00005042 ff1588070300               call      qword ptr [rip + 0x30788] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
00005048 488b8c24d0020000           mov       rcx, qword ptr [rsp + 0x2d0]
00005050 4c8bc1                     mov       r8, rcx
00005053 488d9424b4010000           lea       rdx, [rsp + 0x1b4]
0000505b 488bc8                     mov       rcx, rax
0000505e ff15fc070300               call      qword ptr [rip + 0x307fc] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
00005064 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
0000506c 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
00005070 488bd0                     mov       rdx, rax
00005073 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
0000507b e820460000                 call      0x1800096a0
00005080 f30f118424c8020000         movss     dword ptr [rsp + 0x2c8], xmm0
00005089 f30f108424c8020000         movss     xmm0, dword ptr [rsp + 0x2c8]
00005092 f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
0000509b f30f11442454               movss     dword ptr [rsp + 0x54], xmm0
000050a1 83bc241c01000000           cmp       dword ptr [rsp + 0x11c], 0
000050a9 7e3c                       jle       0x1800050e7
000050ab 8b8424a0000000             mov       eax, dword ptr [rsp + 0xa0]
000050b2 39842418010000             cmp       dword ptr [rsp + 0x118], eax
000050b9 7d2c                       jge       0x1800050e7
000050bb 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
000050c2 83c00a                     add       eax, 0xa
000050c5 4898                       cdqe
000050c7 482b8424d0000000           sub       rax, qword ptr [rsp + 0xd0]
000050cf 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000050d4 f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
000050d9 f30f118424d8020000         movss     dword ptr [rsp + 0x2d8], xmm0
000050e2 e990000000                 jmp       0x180005177
000050e7 488d05c2660400             lea       rax, [rip + 0x466c2]
000050ee 4883c00c                   add       rax, 0xc
000050f2 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
000050fa 488d9424cc010000           lea       rdx, [rsp + 0x1cc]
00005102 488bc8                     mov       rcx, rax
00005105 ff15bd060300               call      qword ptr [rip + 0x306bd] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000510b 48898424e0020000           mov       qword ptr [rsp + 0x2e0], rax
00005113 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
0000511b 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
00005120 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
00005125 488bd1                     mov       rdx, rcx
00005128 488d8c24c0010000           lea       rcx, [rsp + 0x1c0]
00005130 ff159a060300               call      qword ptr [rip + 0x3069a] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
00005136 488b8c24e0020000           mov       rcx, qword ptr [rsp + 0x2e0]
0000513e 4c8bc1                     mov       r8, rcx
00005141 488d9424d8010000           lea       rdx, [rsp + 0x1d8]
00005149 488bc8                     mov       rcx, rax
0000514c ff150e070300               call      qword ptr [rip + 0x3070e] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
00005152 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
0000515a 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
0000515e 488bd0                     mov       rdx, rax
00005161 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00005169 e832450000                 call      0x1800096a0
0000516e f30f118424d8020000         movss     dword ptr [rsp + 0x2d8], xmm0
00005177 f30f108424d8020000         movss     xmm0, dword ptr [rsp + 0x2d8]
00005180 f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
00005189 f30f11442460               movss     dword ptr [rsp + 0x60], xmm0
0000518f 8b8424a0000000             mov       eax, dword ptr [rsp + 0xa0]
00005196 39842418010000             cmp       dword ptr [rsp + 0x118], eax
0000519d 7d24                       jge       0x1800051c3
0000519f 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
000051a6 83c00a                     add       eax, 0xa
000051a9 4898                       cdqe
000051ab 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000051b0 f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
000051b5 f30f118424e8020000         movss     dword ptr [rsp + 0x2e8], xmm0
000051be e990000000                 jmp       0x180005253
000051c3 488d05e6650400             lea       rax, [rip + 0x465e6]
000051ca 4883c018                   add       rax, 0x18
000051ce 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
000051d6 488d9424f0010000           lea       rdx, [rsp + 0x1f0]
000051de 488bc8                     mov       rcx, rax
000051e1 ff15e1050300               call      qword ptr [rip + 0x305e1] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000051e7 48898424f0020000           mov       qword ptr [rsp + 0x2f0], rax
000051ef 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
000051f7 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
000051fc 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
00005201 488bd1                     mov       rdx, rcx
00005204 488d8c24e4010000           lea       rcx, [rsp + 0x1e4]
0000520c ff15be050300               call      qword ptr [rip + 0x305be] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
00005212 488b8c24f0020000           mov       rcx, qword ptr [rsp + 0x2f0]
0000521a 4c8bc1                     mov       r8, rcx
0000521d 488d9424fc010000           lea       rdx, [rsp + 0x1fc]
00005225 488bc8                     mov       rcx, rax
00005228 ff1532060300               call      qword ptr [rip + 0x30632] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000522e 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00005236 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
0000523a 488bd0                     mov       rdx, rax
0000523d 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00005245 e856440000                 call      0x1800096a0
0000524a f30f118424e8020000         movss     dword ptr [rsp + 0x2e8], xmm0
00005253 f30f108424e8020000         movss     xmm0, dword ptr [rsp + 0x2e8]
0000525c f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
00005265 f30f1144246c               movss     dword ptr [rsp + 0x6c], xmm0
0000526b 8b8424a0000000             mov       eax, dword ptr [rsp + 0xa0]
00005272 3984241c010000             cmp       dword ptr [rsp + 0x11c], eax
00005279 7d28                       jge       0x1800052a3
0000527b 4863842424010000           movsxd    rax, dword ptr [rsp + 0x124]
00005283 48038424d0000000           add       rax, qword ptr [rsp + 0xd0]
0000528b 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005290 f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
00005295 f30f118424f8020000         movss     dword ptr [rsp + 0x2f8], xmm0
0000529e e990000000                 jmp       0x180005333
000052a3 488d0506650400             lea       rax, [rip + 0x46506]
000052aa 4883c024                   add       rax, 0x24
000052ae 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
000052b6 488d942414020000           lea       rdx, [rsp + 0x214]
000052be 488bc8                     mov       rcx, rax
000052c1 ff1501050300               call      qword ptr [rip + 0x30501] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000052c7 4889842400030000           mov       qword ptr [rsp + 0x300], rax
000052cf 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
000052d7 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
000052dc 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
000052e1 488bd1                     mov       rdx, rcx
000052e4 488d8c2408020000           lea       rcx, [rsp + 0x208]
000052ec ff15de040300               call      qword ptr [rip + 0x304de] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
000052f2 488b8c2400030000           mov       rcx, qword ptr [rsp + 0x300]
000052fa 4c8bc1                     mov       r8, rcx
000052fd 488d942420020000           lea       rdx, [rsp + 0x220]
00005305 488bc8                     mov       rcx, rax
00005308 ff1552050300               call      qword ptr [rip + 0x30552] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000530e 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00005316 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
0000531a 488bd0                     mov       rdx, rax
0000531d 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00005325 e876430000                 call      0x1800096a0
0000532a f30f118424f8020000         movss     dword ptr [rsp + 0x2f8], xmm0
00005333 f30f108424f8020000         movss     xmm0, dword ptr [rsp + 0x2f8]
0000533c f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
00005345 f30f11442478               movss     dword ptr [rsp + 0x78], xmm0
0000534b 83bc241801000000           cmp       dword ptr [rsp + 0x118], 0
00005353 7e39                       jle       0x18000538e
00005355 8b8424a0000000             mov       eax, dword ptr [rsp + 0xa0]
0000535c 3984241c010000             cmp       dword ptr [rsp + 0x11c], eax
00005363 7d29                       jge       0x18000538e
00005365 4863842424010000           movsxd    rax, dword ptr [rsp + 0x124]
0000536d 48038424d0000000           add       rax, qword ptr [rsp + 0xd0]
00005375 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0000537a f30f104481d8               movss     xmm0, dword ptr [rcx + rax*4 - 0x28]
00005380 f30f11842408030000         movss     dword ptr [rsp + 0x308], xmm0
00005389 e990000000                 jmp       0x18000541e
0000538e 488d051b640400             lea       rax, [rip + 0x4641b]
00005395 4883c030                   add       rax, 0x30
00005399 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
000053a1 488d942438020000           lea       rdx, [rsp + 0x238]
000053a9 488bc8                     mov       rcx, rax
000053ac ff1516040300               call      qword ptr [rip + 0x30416] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000053b2 4889842410030000           mov       qword ptr [rsp + 0x310], rax
000053ba 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
000053c2 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
000053c7 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
000053cc 488bd1                     mov       rdx, rcx
000053cf 488d8c242c020000           lea       rcx, [rsp + 0x22c]
000053d7 ff15f3030300               call      qword ptr [rip + 0x303f3] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
000053dd 488b8c2410030000           mov       rcx, qword ptr [rsp + 0x310]
000053e5 4c8bc1                     mov       r8, rcx
000053e8 488d942444020000           lea       rdx, [rsp + 0x244]
000053f0 488bc8                     mov       rcx, rax
000053f3 ff1567040300               call      qword ptr [rip + 0x30467] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000053f9 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00005401 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
00005405 488bd0                     mov       rdx, rax
00005408 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00005410 e88b420000                 call      0x1800096a0
00005415 f30f11842408030000         movss     dword ptr [rsp + 0x308], xmm0
0000541e f30f10842408030000         movss     xmm0, dword ptr [rsp + 0x308]
00005427 f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
00005430 f30f11842484000000         movss     dword ptr [rsp + 0x84], xmm0
00005439 83bc241801000000           cmp       dword ptr [rsp + 0x118], 0
00005441 7e24                       jle       0x180005467
00005443 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
0000544a 83e80a                     sub       eax, 0xa
0000544d 4898                       cdqe
0000544f 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005454 f30f100481                 movss     xmm0, dword ptr [rcx + rax*4]
00005459 f30f11842418030000         movss     dword ptr [rsp + 0x318], xmm0
00005462 e990000000                 jmp       0x1800054f7
00005467 488d0542630400             lea       rax, [rip + 0x46342]
0000546e 4883c03c                   add       rax, 0x3c
00005472 4c8d8424a8000000           lea       r8, [rsp + 0xa8]
0000547a 488d94245c020000           lea       rdx, [rsp + 0x25c]
00005482 488bc8                     mov       rcx, rax
00005485 ff153d030300               call      qword ptr [rip + 0x3033d] ; ??DVector3@Ogre@@QEBA?AV01@AEBV01@@Z
0000548b 4889842420030000           mov       qword ptr [rsp + 0x320], rax
00005493 48638c2424010000           movsxd    rcx, dword ptr [rsp + 0x124]
0000549b 488b542430                 mov       rdx, qword ptr [rsp + 0x30]
000054a0 488d4c8afc                 lea       rcx, [rdx + rcx*4 - 4]
000054a5 488bd1                     mov       rdx, rcx
000054a8 488d8c2450020000           lea       rcx, [rsp + 0x250]
000054b0 ff151a030300               call      qword ptr [rip + 0x3031a] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
000054b6 488b8c2420030000           mov       rcx, qword ptr [rsp + 0x320]
000054be 4c8bc1                     mov       r8, rcx
000054c1 488d942468020000           lea       rdx, [rsp + 0x268]
000054c9 488bc8                     mov       rcx, rax
000054cc ff158e030300               call      qword ptr [rip + 0x3038e] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000054d2 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
000054da 448b4178                   mov       r8d, dword ptr [rcx + 0x78]
000054de 488bd0                     mov       rdx, rax
000054e1 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
000054e9 e8b2410000                 call      0x1800096a0
000054ee f30f11842418030000         movss     dword ptr [rsp + 0x318], xmm0
000054f7 f30f10842418030000         movss     xmm0, dword ptr [rsp + 0x318]
00005500 f30f5c842420010000         subss     xmm0, dword ptr [rsp + 0x120]
00005509 f30f11842490000000         movss     dword ptr [rsp + 0x90], xmm0
00005512 0f57c0                     xorps     xmm0, xmm0
00005515 f30f118424c8000000         movss     dword ptr [rsp + 0xc8], xmm0
0000551e f30f108424c8000000         movss     xmm0, dword ptr [rsp + 0xc8]
00005527 f30f118424c4000000         movss     dword ptr [rsp + 0xc4], xmm0
00005530 f30f108424c4000000         movss     xmm0, dword ptr [rsp + 0xc4]
00005539 f30f118424c0000000         movss     dword ptr [rsp + 0xc0], xmm0
00005542 c784242801000000000000     mov       dword ptr [rsp + 0x128], 0
0000554d eb10                       jmp       0x18000555f
0000554f 8b842428010000             mov       eax, dword ptr [rsp + 0x128]
00005556 ffc0                       inc       eax
00005558 89842428010000             mov       dword ptr [rsp + 0x128], eax
0000555f 83bc242801000006           cmp       dword ptr [rsp + 0x128], 6
00005567 7d53                       jge       0x1800055bc
00005569 8b842428010000             mov       eax, dword ptr [rsp + 0x128]
00005570 ffc0                       inc       eax
00005572 99                         cdq
00005573 b906000000                 mov       ecx, 6
00005578 f7f9                       idiv      ecx
0000557a 8bc2                       mov       eax, edx
0000557c 4898                       cdqe
0000557e 486bc00c                   imul      rax, rax, 0xc
00005582 488d440450                 lea       rax, [rsp + rax + 0x50]
00005587 48638c2428010000           movsxd    rcx, dword ptr [rsp + 0x128]
0000558f 486bc90c                   imul      rcx, rcx, 0xc
00005593 488d4c0c50                 lea       rcx, [rsp + rcx + 0x50]
00005598 4c8bc0                     mov       r8, rax
0000559b 488d942474020000           lea       rdx, [rsp + 0x274]
000055a3 ff152f020300               call      qword ptr [rip + 0x3022f] ; ?crossProduct@Vector3@Ogre@@QEBA?AV12@AEBV12@@Z
000055a9 488bd0                     mov       rdx, rax
000055ac 488d8c24c0000000           lea       rcx, [rsp + 0xc0]
000055b4 ff1526020300               call      qword ptr [rip + 0x30226] ; ??YVector3@Ogre@@QEAAAEAV01@AEBV01@@Z
000055ba eb93                       jmp       0x18000554f
000055bc 488d8c24c0000000           lea       rcx, [rsp + 0xc0]
000055c4 ff151e020300               call      qword ptr [rip + 0x3021e] ; ?normalise@Vector3@Ogre@@QEAAMXZ
000055ca f30f108424c0000000         movss     xmm0, dword ptr [rsp + 0xc0]
000055d3 0f570556090300             xorps     xmm0, xmmword ptr [rip + 0x30956]
000055da 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
000055e1 83c002                     add       eax, 2
000055e4 4898                       cdqe
000055e6 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000055eb f30f110481                 movss     dword ptr [rcx + rax*4], xmm0
000055f0 f30f108424c4000000         movss     xmm0, dword ptr [rsp + 0xc4]
000055f9 0f570530090300             xorps     xmm0, xmmword ptr [rip + 0x30930]
00005600 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
00005607 83c003                     add       eax, 3
0000560a 4898                       cdqe
0000560c 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005611 f30f110481                 movss     dword ptr [rcx + rax*4], xmm0
00005616 f30f108424c8000000         movss     xmm0, dword ptr [rsp + 0xc8]
0000561f 0f57050a090300             xorps     xmm0, xmmword ptr [rip + 0x3090a]
00005626 8b842424010000             mov       eax, dword ptr [rsp + 0x124]
0000562d 83c004                     add       eax, 4
00005630 4898                       cdqe
00005632 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005637 f30f110481                 movss     dword ptr [rcx + rax*4], xmm0
0000563c e928f9ffff                 jmp       0x180004f69
00005641 e9f2f8ffff                 jmp       0x180004f38
00005646 c784242c01000000000000     mov       dword ptr [rsp + 0x12c], 0
00005651 eb10                       jmp       0x180005663
00005653 8b84242c010000             mov       eax, dword ptr [rsp + 0x12c]
0000565a ffc0                       inc       eax
0000565c 8984242c010000             mov       dword ptr [rsp + 0x12c], eax
00005663 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
0000566a 3984242c010000             cmp       dword ptr [rsp + 0x12c], eax
00005671 0f8d92030000               jge       0x180005a09
00005677 c784243001000000000000     mov       dword ptr [rsp + 0x130], 0
00005682 eb10                       jmp       0x180005694
00005684 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
0000568b ffc0                       inc       eax
0000568d 89842430010000             mov       dword ptr [rsp + 0x130], eax
00005694 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
0000569b 39842430010000             cmp       dword ptr [rsp + 0x130], eax
000056a2 0f8d5c030000               jge       0x180005a04
000056a8 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
000056af 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
000056b7 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
000056be 03c8                       add       ecx, eax
000056c0 8bc1                       mov       eax, ecx
000056c2 6bc00a                     imul      eax, eax, 0xa
000056c5 4898                       cdqe
000056c7 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000056cc 488d0481                   lea       rax, [rcx + rax*4]
000056d0 4889842438010000           mov       qword ptr [rsp + 0x138], rax
000056d8 48c784244001000000000000   mov       qword ptr [rsp + 0x140], 0
000056e4 48c784244801000000000000   mov       qword ptr [rsp + 0x148], 0
000056f0 8b84242c010000             mov       eax, dword ptr [rsp + 0x12c]
000056f7 83e001                     and       eax, 1
000056fa 85c0                       test      eax, eax
000056fc 7477                       je        0x180005775
000056fe 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
00005705 83e001                     and       eax, 1
00005708 85c0                       test      eax, eax
0000570a 7469                       je        0x180005775
0000570c 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
00005713 ffc8                       dec       eax
00005715 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
0000571d 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
00005724 8d4401ff                   lea       eax, [rcx + rax - 1]
00005728 6bc00a                     imul      eax, eax, 0xa
0000572b 4898                       cdqe
0000572d 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005732 488d0481                   lea       rax, [rcx + rax*4]
00005736 4889842440010000           mov       qword ptr [rsp + 0x140], rax
0000573e 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
00005745 ffc0                       inc       eax
00005747 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
0000574f 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
00005756 8d440101                   lea       eax, [rcx + rax + 1]
0000575a 6bc00a                     imul      eax, eax, 0xa
0000575d 4898                       cdqe
0000575f 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005764 488d0481                   lea       rax, [rcx + rax*4]
00005768 4889842448010000           mov       qword ptr [rsp + 0x148], rax
00005770 e9e2000000                 jmp       0x180005857
00005775 8b84242c010000             mov       eax, dword ptr [rsp + 0x12c]
0000577c 83e001                     and       eax, 1
0000577f 85c0                       test      eax, eax
00005781 7462                       je        0x1800057e5
00005783 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
0000578a 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00005792 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
00005799 8d4401ff                   lea       eax, [rcx + rax - 1]
0000579d 6bc00a                     imul      eax, eax, 0xa
000057a0 4898                       cdqe
000057a2 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000057a7 488d0481                   lea       rax, [rcx + rax*4]
000057ab 4889842440010000           mov       qword ptr [rsp + 0x140], rax
000057b3 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
000057ba 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
000057c2 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
000057c9 8d440101                   lea       eax, [rcx + rax + 1]
000057cd 6bc00a                     imul      eax, eax, 0xa
000057d0 4898                       cdqe
000057d2 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
000057d7 488d0481                   lea       rax, [rcx + rax*4]
000057db 4889842448010000           mov       qword ptr [rsp + 0x148], rax
000057e3 eb72                       jmp       0x180005857
000057e5 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
000057ec 83e001                     and       eax, 1
000057ef 85c0                       test      eax, eax
000057f1 7464                       je        0x180005857
000057f3 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
000057fa ffc8                       dec       eax
000057fc 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00005804 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
0000580b 03c8                       add       ecx, eax
0000580d 8bc1                       mov       eax, ecx
0000580f 6bc00a                     imul      eax, eax, 0xa
00005812 4898                       cdqe
00005814 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005819 488d0481                   lea       rax, [rcx + rax*4]
0000581d 4889842440010000           mov       qword ptr [rsp + 0x140], rax
00005825 8b842430010000             mov       eax, dword ptr [rsp + 0x130]
0000582c ffc0                       inc       eax
0000582e 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00005836 8b8c242c010000             mov       ecx, dword ptr [rsp + 0x12c]
0000583d 03c8                       add       ecx, eax
0000583f 8bc1                       mov       eax, ecx
00005841 6bc00a                     imul      eax, eax, 0xa
00005844 4898                       cdqe
00005846 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0000584b 488d0481                   lea       rax, [rcx + rax*4]
0000584f 4889842448010000           mov       qword ptr [rsp + 0x148], rax
00005857 4883bc244001000000         cmp       qword ptr [rsp + 0x140], 0
00005860 0f847a010000               je        0x1800059e0
00005866 4883bc244801000000         cmp       qword ptr [rsp + 0x148], 0
0000586f 0f846b010000               je        0x1800059e0
00005875 488b842448010000           mov       rax, qword ptr [rsp + 0x148]
0000587d 4883c00c                   add       rax, 0xc
00005881 488bd0                     mov       rdx, rax
00005884 488d8c248c020000           lea       rcx, [rsp + 0x28c]
0000588c ff153eff0200               call      qword ptr [rip + 0x2ff3e] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
00005892 4889842428030000           mov       qword ptr [rsp + 0x328], rax
0000589a 488b8c2440010000           mov       rcx, qword ptr [rsp + 0x140]
000058a2 4883c10c                   add       rcx, 0xc
000058a6 488bd1                     mov       rdx, rcx
000058a9 488d8c2480020000           lea       rcx, [rsp + 0x280]
000058b1 ff1519ff0200               call      qword ptr [rip + 0x2ff19] ; ??0Vector3@Ogre@@QEAA@QEAM@Z
000058b7 488b8c2428030000           mov       rcx, qword ptr [rsp + 0x328]
000058bf 4c8bc1                     mov       r8, rcx
000058c2 488d942450010000           lea       rdx, [rsp + 0x150]
000058ca 488bc8                     mov       rcx, rax
000058cd ff158dff0200               call      qword ptr [rip + 0x2ff8d] ; ??HVector3@Ogre@@QEBA?AV01@AEBV01@@Z
000058d3 488d8c2450010000           lea       rcx, [rsp + 0x150]
000058db ff1507ff0200               call      qword ptr [rip + 0x2ff07] ; ?normalise@Vector3@Ogre@@QEAAMXZ
000058e1 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
000058e9 f30f10842450010000         movss     xmm0, dword ptr [rsp + 0x150]
000058f2 f30f114018                 movss     dword ptr [rax + 0x18], xmm0
000058f7 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
000058ff f30f10842454010000         movss     xmm0, dword ptr [rsp + 0x154]
00005908 f30f11401c                 movss     dword ptr [rax + 0x1c], xmm0
0000590d 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
00005915 f30f10842458010000         movss     xmm0, dword ptr [rsp + 0x158]
0000591e f30f114020                 movss     dword ptr [rax + 0x20], xmm0
00005923 488b842440010000           mov       rax, qword ptr [rsp + 0x140]
0000592b 488b8c2448010000           mov       rcx, qword ptr [rsp + 0x148]
00005933 f30f104004                 movss     xmm0, dword ptr [rax + 4]
00005938 f30f584104                 addss     xmm0, dword ptr [rcx + 4]
0000593d f30f5905e7040300           mulss     xmm0, dword ptr [rip + 0x304e7]
00005945 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
0000594d f30f114024                 movss     dword ptr [rax + 0x24], xmm0
00005952 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
0000595a 488b8c2438010000           mov       rcx, qword ptr [rsp + 0x138]
00005962 f30f104024                 movss     xmm0, dword ptr [rax + 0x24]
00005967 f30f5c4104                 subss     xmm0, dword ptr [rcx + 4]
0000596c e8ef000000                 call      0x180005a60
00005971 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00005979 0f2f8084000000             comiss    xmm0, dword ptr [rax + 0x84]
00005980 762a                       jbe       0x1800059ac
00005982 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
0000598a 488b8c2438010000           mov       rcx, qword ptr [rsp + 0x138]
00005992 f30f104024                 movss     xmm0, dword ptr [rax + 0x24]
00005997 f30f5c4104                 subss     xmm0, dword ptr [rcx + 4]
0000599c e8bf000000                 call      0x180005a60
000059a1 f30f11842430030000         movss     dword ptr [rsp + 0x330], xmm0
000059aa eb19                       jmp       0x1800059c5
000059ac 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000059b4 f30f108084000000           movss     xmm0, dword ptr [rax + 0x84]
000059bc f30f11842430030000         movss     dword ptr [rsp + 0x330], xmm0
000059c5 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
000059cd f30f10842430030000         movss     xmm0, dword ptr [rsp + 0x330]
000059d6 f30f118084000000           movss     dword ptr [rax + 0x84], xmm0
000059de eb1f                       jmp       0x1800059ff
000059e0 488b842438010000           mov       rax, qword ptr [rsp + 0x138]
000059e8 488b8c2438010000           mov       rcx, qword ptr [rsp + 0x138]
000059f0 488d7818                   lea       rdi, [rax + 0x18]
000059f4 488d710c                   lea       rsi, [rcx + 0xc]
000059f8 b90c000000                 mov       ecx, 0xc
000059fd f3a4                       rep movsb byte ptr [rdi], byte ptr [rsi]
000059ff e980fcffff                 jmp       0x180005684
00005a04 e94afcffff                 jmp       0x180005653
00005a09 488b842460030000           mov       rax, qword ptr [rsp + 0x360]
00005a11 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005a16 488988d8000000             mov       qword ptr [rax + 0xd8], rcx
00005a1d 8b8424bc000000             mov       eax, dword ptr [rsp + 0xbc]
00005a24 0faf8424bc000000           imul      eax, dword ptr [rsp + 0xbc]
00005a2c 4898                       cdqe
00005a2e 488b8c2460030000           mov       rcx, qword ptr [rsp + 0x360]
00005a36 488981c8000000             mov       qword ptr [rcx + 0xc8], rax
00005a3d 488d4c2440                 lea       rcx, [rsp + 0x40]
00005a42 e889c3ffff                 call      0x180001dd0
00005a47 90                         nop
00005a48 488d8c24d8000000           lea       rcx, [rsp + 0xd8]
00005a50 e86bb9ffff                 call      0x1800013c0
00005a55 4881c448030000             add       rsp, 0x348
00005a5c 5f                         pop       rdi
00005a5d 5e                         pop       rsi
00005a5e c3                         ret
00005a5f cc                         int3

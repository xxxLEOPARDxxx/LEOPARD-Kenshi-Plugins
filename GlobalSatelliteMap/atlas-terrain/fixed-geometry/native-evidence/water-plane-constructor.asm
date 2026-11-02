009d6000 64ff4533                   inc       dword ptr fs:[rbp + 0x33]
009d6004 ed                         in        eax, dx
009d6005 458be5                     mov       r12d, r13d
009d6008 4585f6                     test      r14d, r14d
009d600b 0f8e64010000               jle       0x1409d6175
009d6011 f30f103dbf78ca00           movss     xmm7, dword ptr [rip + 0xca78bf]
009d6019 450f57c0                   xorps     xmm8, xmm8
009d601d 0f1f00                     nop       dword ptr [rax]
009d6020 418bfd                     mov       edi, r13d
009d6023 66410f6ef4                 movd      xmm6, r12d
009d6028 0f5bf6                     cvtdq2ps  xmm6, xmm6
009d602b f30f58f7                   addss     xmm6, xmm7
009d602f 90                         nop
009d6030 498b4738                   mov       rax, qword ptr [r15 + 0x38]
009d6034 660f6edf                   movd      xmm3, edi
009d6038 0f5bdb                     cvtdq2ps  xmm3, xmm3
009d603b f30f58df                   addss     xmm3, xmm7
009d603f f30f5998cc000000           mulss     xmm3, dword ptr [rax + 0xcc]
009d6047 660f6e400c                 movd      xmm0, dword ptr [rax + 0xc]
009d604c 0f5bc0                     cvtdq2ps  xmm0, xmm0
009d604f f30f5ed8                   divss     xmm3, xmm0
009d6053 f30f5898c4000000           addss     xmm3, dword ptr [rax + 0xc4]
009d605b 0f28ce                     movaps    xmm1, xmm6
009d605e f30f5988c8000000           mulss     xmm1, dword ptr [rax + 0xc8]
009d6066 660f6e4008                 movd      xmm0, dword ptr [rax + 8]
009d606b 0f5bc0                     cvtdq2ps  xmm0, xmm0
009d606e f30f5ec8                   divss     xmm1, xmm0
009d6072 f30f5888c0000000           addss     xmm1, dword ptr [rax + 0xc0]
009d607a 410f28d0                   movaps    xmm2, xmm8
009d607e 488d4c2430                 lea       rcx, [rsp + 0x30]
009d6083 ff154f018701               call      qword ptr [rip + 0x187014f] ; ??0Vector3@Ogre@@QEAA@MMM@Z
009d6089 488d542430                 lea       rdx, [rsp + 0x30]
009d608e 488b0dcbd87501             mov       rcx, qword ptr [rip + 0x175d8cb]
009d6095 488b8920821600             mov       rcx, qword ptr [rcx + 0x168220]
009d609c e8177f67ff                 call      0x14004dfb8
009d60a1 488bd8                     mov       rbx, rax
009d60a4 4885c0                     test      rax, rax
009d60a7 0f84b1000000               je        0x1409d615e
009d60ad 48c74424600f000000         mov       qword ptr [rsp + 0x60], 0xf
009d60b6 4c896c2458                 mov       qword ptr [rsp + 0x58], r13
009d60bb c644244800                 mov       byte ptr [rsp + 0x48], 0
009d60c0 41b80b000000               mov       r8d, 0xb
009d60c6 488d1503e0d600             lea       rdx, [rip + 0xd6e003]
009d60cd 488d4c2448                 lea       rcx, [rsp + 0x48]
009d60d2 e8ee7667ff                 call      0x14004d7c5
009d60d7 90                         nop
009d60d8 488d8b78010000             lea       rcx, [rbx + 0x178]
009d60df 488d542448                 lea       rdx, [rsp + 0x48]
009d60e4 e8152067ff                 call      0x1400480fe
009d60e9 8b7028                     mov       esi, dword ptr [rax + 0x28]
009d60ec 498b6f38                   mov       rbp, qword ptr [r15 + 0x38]
009d60f0 3b7d08                     cmp       edi, dword ptr [rbp + 8]
009d60f3 7d57                       jge       0x1409d614c
009d60f5 443b650c                   cmp       r12d, dword ptr [rbp + 0xc]
009d60f9 7d51                       jge       0x1409d614c
009d60fb 85ff                       test      edi, edi
009d60fd 784d                       js        0x1409d614c
009d60ff 4585e4                     test      r12d, r12d
009d6102 7848                       js        0x1409d614c
009d6104 8b9db0000000               mov       ebx, dword ptr [rbp + 0xb0]
009d610a 488b8db8000000             mov       rcx, qword ptr [rbp + 0xb8]
009d6111 ff1579008701               call      qword ptr [rip + 0x1870079] ; ?getData@Image@Ogre@@QEAAPEAEXZ
009d6117 448b5d08                   mov       r11d, dword ptr [rbp + 8]
009d611b 440fafdf                   imul      r11d, edi
009d611f 4503dc                     add       r11d, r12d
009d6122 440fafdb                   imul      r11d, ebx
009d6126 4c03d8                     add       r11, rax
009d6129 83fb04                     cmp       ebx, 4
009d612c 7509                       jne       0x1409d6137
009d612e 8bce                       mov       ecx, esi
009d6130 c1e918                     shr       ecx, 0x18
009d6133 41884b03                   mov       byte ptr [r11 + 3], cl
009d6137 8bc6                       mov       eax, esi
009d6139 c1e810                     shr       eax, 0x10
009d613c 41884302                   mov       byte ptr [r11 + 2], al
009d6140 8bc6                       mov       eax, esi
009d6142 c1e808                     shr       eax, 8
009d6145 41884301                   mov       byte ptr [r11 + 1], al
009d6149 418833                     mov       byte ptr [r11], sil
009d614c 48837c246010               cmp       qword ptr [rsp + 0x60], 0x10
009d6152 720a                       jb        0x1409d615e
009d6154 488b4c2448                 mov       rcx, qword ptr [rsp + 0x48]
009d6159 e8caf44f00                 call      0x140ed5628
009d615e ffc7                       inc       edi
009d6160 413bfe                     cmp       edi, r14d
009d6163 0f8cc7feffff               jl        0x1409d6030
009d6169 41ffc4                     inc       r12d
009d616c 453be6                     cmp       r12d, r14d
009d616f 0f8cabfeffff               jl        0x1409d6020
009d6175 498b7f38                   mov       rdi, qword ptr [r15 + 0x38]
009d6179 83bfb000000000             cmp       dword ptr [rdi + 0xb0], 0
009d6180 7643                       jbe       0x1409d61c5
009d6182 4c896c2430                 mov       qword ptr [rsp + 0x30], r13
009d6187 498bdd                     mov       rbx, r13
009d618a f30f10353adfd600           movss     xmm6, dword ptr [rip + 0xd6df3a]
009d6192 8b4708                     mov       eax, dword ptr [rdi + 8]
009d6195 89442430                   mov       dword ptr [rsp + 0x30], eax
009d6199 8b470c                     mov       eax, dword ptr [rdi + 0xc]
009d619c 89442434                   mov       dword ptr [rsp + 0x34], eax
009d61a0 f30f11742420               movss     dword ptr [rsp + 0x20], xmm6
009d61a6 458bcd                     mov       r9d, r13d
009d61a9 4c8b442430                 mov       r8, qword ptr [rsp + 0x30]
009d61ae 488bd3                     mov       rdx, rbx
009d61b1 488bcf                     mov       rcx, rdi
009d61b4 e8a55265ff                 call      0x14002b45e
009d61b9 41ffc5                     inc       r13d
009d61bc 443bafb0000000             cmp       r13d, dword ptr [rdi + 0xb0]
009d61c3 72cd                       jb        0x1409d6192
009d61c5 498b4f38                   mov       rcx, qword ptr [r15 + 0x38]
009d61c9 e8857967ff                 call      0x14004db53
009d61ce 498b4f38                   mov       rcx, qword ptr [r15 + 0x38]
009d61d2 e8849165ff                 call      0x14002f35b
009d61d7 488b4c2470                 mov       rcx, qword ptr [rsp + 0x70]
009d61dc 4833cc                     xor       rcx, rsp
009d61df e8ccfb4f00                 call      0x140ed5db0
009d61e4 4c8d9c24b0000000           lea       r11, [rsp + 0xb0]
009d61ec 498b5b50                   mov       rbx, qword ptr [r11 + 0x50]
009d61f0 410f2873f0                 movaps    xmm6, xmmword ptr [r11 - 0x10]
009d61f5 410f287be0                 movaps    xmm7, xmmword ptr [r11 - 0x20]
009d61fa 450f2843d0                 movaps    xmm8, xmmword ptr [r11 - 0x30]
009d61ff 498be3                     mov       rsp, r11
009d6202 415f                       pop       r15
009d6204 415e                       pop       r14
009d6206 415d                       pop       r13
009d6208 415c                       pop       r12
009d620a 5f                         pop       rdi
009d620b 5e                         pop       rsi
009d620c 5d                         pop       rbp
009d620d c3                         ret
009d620e cc                         int3
009d620f cc                         int3
009d6210 cc                         int3
009d6211 cc                         int3
009d6212 cc                         int3
009d6213 cc                         int3
009d6214 cc                         int3
009d6215 cc                         int3
009d6216 cc                         int3
009d6217 cc                         int3
009d6218 cc                         int3
009d6219 cc                         int3
009d621a cc                         int3
009d621b cc                         int3
009d621c cc                         int3
009d621d cc                         int3
009d621e cc                         int3
009d621f cc                         int3
009d6220 488bc4                     mov       rax, rsp
009d6223 55                         push      rbp
009d6224 57                         push      rdi
009d6225 4154                       push      r12
009d6227 488da858feffff             lea       rbp, [rax - 0x1a8]
009d622e 4881eca0020000             sub       rsp, 0x2a0
009d6235 48c745f0feffffff           mov       qword ptr [rbp - 0x10], 0xfffffffffffffffe
009d623d 48895810                   mov       qword ptr [rax + 0x10], rbx
009d6241 48897018                   mov       qword ptr [rax + 0x18], rsi
009d6245 0f2970d8                   movaps    xmmword ptr [rax - 0x28], xmm6
009d6249 0f2978c8                   movaps    xmmword ptr [rax - 0x38], xmm7
009d624d 488b0564017401             mov       rax, qword ptr [rip + 0x1740164]
009d6254 4833c4                     xor       rax, rsp
009d6257 48898560010000             mov       qword ptr [rbp + 0x160], rax
009d625e 488bf1                     mov       rsi, rcx
009d6261 4533e4                     xor       r12d, r12d
009d6264 418bdc                     mov       ebx, r12d
009d6267 895d80                     mov       dword ptr [rbp - 0x80], ebx
009d626a b9d0000000                 mov       ecx, 0xd0
009d626f ff1553ff8601               call      qword ptr [rip + 0x186ff53] ; ??2?$AllocatedObject@V?$CategorisedAllocPolicy@$04@Ogre@@@Ogre@@SAPEAX_K@Z
009d6275 488bf8                     mov       rdi, rax
009d6278 48894588                   mov       qword ptr [rbp - 0x78], rax
009d627c f30f103578cccb00           movss     xmm6, dword ptr [rip + 0xcbcc78]
009d6284 f30f103df42ccf00           movss     xmm7, dword ptr [rip + 0xcf2cf4]
009d628c 4885c0                     test      rax, rax
009d628f 0f84a6000000               je        0x1409d633b
009d6295 48c785280100000f000000     mov       qword ptr [rbp + 0x128], 0xf
009d62a0 4c89a520010000             mov       qword ptr [rbp + 0x120], r12
009d62a7 4488a510010000             mov       byte ptr [rbp + 0x110], r12b
009d62ae bb03000000                 mov       ebx, 3
009d62b3 448bc3                     mov       r8d, ebx
009d62b6 488d15e79bcf00             lea       rdx, [rip + 0xcf9be7]
009d62bd 488d8d10010000             lea       rcx, [rbp + 0x110]
009d62c4 e8fc7467ff                 call      0x14004d7c5
009d62c9 90                         nop
009d62ca c7458001000000             mov       dword ptr [rbp - 0x80], 1
009d62d1 48c785880000000f000000     mov       qword ptr [rbp + 0x88], 0xf
009d62dc 4c89a580000000             mov       qword ptr [rbp + 0x80], r12
009d62e3 44886570                   mov       byte ptr [rbp + 0x70], r12b
009d62e7 458d44240e                 lea       r8d, [r12 + 0xe]
009d62ec 488d155dded600             lea       rdx, [rip + 0xd6de5d]
009d62f3 488d4d70                   lea       rcx, [rbp + 0x70]
009d62f7 e8c97467ff                 call      0x14004d7c5
009d62fc 90                         nop
009d62fd 895d80                     mov       dword ptr [rbp - 0x80], ebx
009d6300 f30f11742420               movss     dword ptr [rsp + 0x20], xmm6
009d6306 0f28de                     movaps    xmm3, xmm6
009d6309 0f28d7                     movaps    xmm2, xmm7
009d630c 0f28cf                     movaps    xmm1, xmm7
009d630f 488d4d00                   lea       rcx, [rbp]
009d6313 ff15170a8701               call      qword ptr [rip + 0x1870a17] ; ??0Vector4@Ogre@@QEAA@MMMM@Z
009d6319 0f286d00                   movaps    xmm5, xmmword ptr [rbp]
009d631d 660f7f6de0                 movdqa    xmmword ptr [rbp - 0x20], xmm5
009d6322 4c8d4de0                   lea       r9, [rbp - 0x20]
009d6326 4c8d8510010000             lea       r8, [rbp + 0x110]
009d632d 488d5570                   lea       rdx, [rbp + 0x70]
009d6331 488bcf                     mov       rcx, rdi
009d6334 e8f09f64ff                 call      0x140020329
009d6339 eb03                       jmp       0x1409d633e
009d633b 498bc4                     mov       rax, r12
009d633e 48894638                   mov       qword ptr [rsi + 0x38], rax
009d6342 f6c302                     test      bl, 2
009d6345 742f                       je        0x1409d6376
009d6347 83e3fd                     and       ebx, 0xfffffffd
009d634a 895d80                     mov       dword ptr [rbp - 0x80], ebx
009d634d 4883bd8800000010           cmp       qword ptr [rbp + 0x88], 0x10
009d6355 7209                       jb        0x1409d6360
009d6357 488b4d70                   mov       rcx, qword ptr [rbp + 0x70]
009d635b e8c8f24f00                 call      0x140ed5628
009d6360 48c785880000000f000000     mov       qword ptr [rbp + 0x88], 0xf
009d636b 4c89a580000000             mov       qword ptr [rbp + 0x80], r12
009d6372 c6457000                   mov       byte ptr [rbp + 0x70], 0
009d6376 f6c301                     test      bl, 1
009d6379 7416                       je        0x1409d6391
009d637b 4883bd2801000010           cmp       qword ptr [rbp + 0x128], 0x10
009d6383 720c                       jb        0x1409d6391
009d6385 488b8d10010000             mov       rcx, qword ptr [rbp + 0x110]
009d638c e897f24f00                 call      0x140ed5628
009d6391 0f57d2                     xorps     xmm2, xmm2
009d6394 488b154d0e8701             mov       rdx, qword ptr [rip + 0x1870e4d] ; ?UNIT_Y@Vector3@Ogre@@2V12@B
009d639b 488d4dd0                   lea       rcx, [rbp - 0x30]
009d639f ff1533128701               call      qword ptr [rip + 0x1871233] ; ??0Plane@Ogre@@QEAA@AEBVVector3@1@M@Z
009d63a5 48c745600f000000           mov       qword ptr [rbp + 0x60], 0xf
009d63ad 4c896558                   mov       qword ptr [rbp + 0x58], r12
009d63b1 c6454800                   mov       byte ptr [rbp + 0x48], 0
009d63b5 41b80c000000               mov       r8d, 0xc
009d63bb 488d157eddd600             lea       rdx, [rip + 0xd6dd7e]
009d63c2 488d4d48                   lea       rcx, [rbp + 0x48]
009d63c6 e8fa7367ff                 call      0x14004d7c5
009d63cb 90                         nop
009d63cc ff159e098701               call      qword ptr [rip + 0x187099e] ; ?getSingleton@MaterialManager@Ogre@@SAAEAV12@XZ
009d63d2 488bc8                     mov       rcx, rax
009d63d5 4c8b0d0cfe8601             mov       r9, qword ptr [rip + 0x186fe0c] ; ?AUTODETECT_RESOURCE_GROUP_NAME@ResourceGroupManager@Ogre@@2V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@A
009d63dc 4c8d4548                   lea       r8, [rbp + 0x48]
009d63e0 488d5590                   lea       rdx, [rbp - 0x70]
009d63e4 ff157e098701               call      qword ptr [rip + 0x187097e] ; ?getByName@MaterialManager@Ogre@@QEAA?AV?$SharedPtr@VMaterial@Ogre@@@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0@Z
009d63ea 90                         nop
009d63eb 48837d6010                 cmp       qword ptr [rbp + 0x60], 0x10
009d63f0 7209                       jb        0x1409d63fb
009d63f2 488b4d48                   mov       rcx, qword ptr [rbp + 0x48]
009d63f6 e82df24f00                 call      0x140ed5628
009d63fb 48c745600f000000           mov       qword ptr [rbp + 0x60], 0xf
009d6403 4c896558                   mov       qword ptr [rbp + 0x58], r12
009d6407 c6454800                   mov       byte ptr [rbp + 0x48], 0
009d640b 48837d9000                 cmp       qword ptr [rbp - 0x70], 0
009d6410 757b                       jne       0x1409d648d
009d6412 48c785b00000000f000000     mov       qword ptr [rbp + 0xb0], 0xf
009d641d 4c89a5a8000000             mov       qword ptr [rbp + 0xa8], r12
009d6424 c6859800000000             mov       byte ptr [rbp + 0x98], 0
009d642b 41b824000000               mov       r8d, 0x24
009d6431 488d15d8dcd600             lea       rdx, [rip + 0xd6dcd8]
009d6438 488d8d98000000             lea       rcx, [rbp + 0x98]
009d643f e8817367ff                 call      0x14004d7c5
009d6444 90                         nop
009d6445 ff155dfd8601               call      qword ptr [rip + 0x186fd5d] ; ?getSingleton@LogManager@Ogre@@SAAEAV12@XZ
009d644b 488bc8                     mov       rcx, rax
009d644e 4533c9                     xor       r9d, r9d
009d6451 458d4102                   lea       r8d, [r9 + 2]
009d6455 488d9598000000             lea       rdx, [rbp + 0x98]
009d645c ff153efd8601               call      qword ptr [rip + 0x186fd3e] ; ?logMessage@LogManager@Ogre@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@W4LogMessageLevel@2@_N@Z
009d6462 90                         nop
009d6463 4883bdb000000010           cmp       qword ptr [rbp + 0xb0], 0x10
009d646b 720d                       jb        0x1409d647a
009d646d 488b8d98000000             mov       rcx, qword ptr [rbp + 0x98]
009d6474 e8aff14f00                 call      0x140ed5628
009d6479 90                         nop
009d647a 48837d9000                 cmp       qword ptr [rbp - 0x70], 0
009d647f 0f84e7040000               je        0x1409d696c
009d6485 83cbff                     or        ebx, 0xffffffff
009d6488 e9b8040000                 jmp       0x1409d6945
009d648d 48c785d80000000f000000     mov       qword ptr [rbp + 0xd8], 0xf
009d6498 4c89a5d0000000             mov       qword ptr [rbp + 0xd0], r12
009d649f c685c000000000             mov       byte ptr [rbp + 0xc0], 0
009d64a6 41b809000000               mov       r8d, 9
009d64ac 488d154ddcd600             lea       rdx, [rip + 0xd6dc4d]
009d64b3 488d8dc0000000             lea       rcx, [rbp + 0xc0]
009d64ba e8067367ff                 call      0x14004d7c5
009d64bf 90                         nop
009d64c0 33d2                       xor       edx, edx
009d64c2 488b4d90                   mov       rcx, qword ptr [rbp - 0x70]
009d64c6 ff158c088701               call      qword ptr [rip + 0x187088c] ; ?getTechnique@Material@Ogre@@QEAAPEAVTechnique@2@G@Z
009d64cc 488bc8                     mov       rcx, rax
009d64cf 410fb7d4                   movzx     edx, r12w
009d64d3 ff1577088701               call      qword ptr [rip + 0x1870877] ; ?getPass@Technique@Ogre@@QEAAPEAVPass@2@G@Z
009d64d9 488bc8                     mov       rcx, rax
009d64dc 488d55a0                   lea       rdx, [rbp - 0x60]
009d64e0 ff157a0b8701               call      qword ptr [rip + 0x1870b7a] ; ?getVertexProgramParameters@Pass@Ogre@@QEBA?AV?$SharedPtr@VGpuProgramParameters@Ogre@@@2@XZ
009d64e6 488bd8                     mov       rbx, rax
009d64e9 f30f11742420               movss     dword ptr [rsp + 0x20], xmm6
009d64ef 0f28de                     movaps    xmm3, xmm6
009d64f2 0f28d7                     movaps    xmm2, xmm7
009d64f5 0f28cf                     movaps    xmm1, xmm7
009d64f8 488d4d10                   lea       rcx, [rbp + 0x10]
009d64fc ff152e088701               call      qword ptr [rip + 0x187082e] ; ??0Vector4@Ogre@@QEAA@MMMM@Z
009d6502 4c8d4510                   lea       r8, [rbp + 0x10]
009d6506 488d95c0000000             lea       rdx, [rbp + 0xc0]
009d650d 488b0b                     mov       rcx, qword ptr [rbx]
009d6510 ff15621b8701               call      qword ptr [rip + 0x1871b62] ; ?setNamedConstant@GpuProgramParameters@Ogre@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@AEBVVector4@2@@Z
009d6516 90                         nop
009d6517 83cbff                     or        ebx, 0xffffffff
009d651a 48837da000                 cmp       qword ptr [rbp - 0x60], 0
009d651f 7429                       je        0x1409d654a
009d6521 488b4da8                   mov       rcx, qword ptr [rbp - 0x58]
009d6525 8bc3                       mov       eax, ebx
009d6527 f00fc14120                 lock xadd dword ptr [rcx + 0x20], eax
009d652c ffc8                       dec       eax
009d652e 751a                       jne       0x1409d654a
009d6530 488b4da8                   mov       rcx, qword ptr [rbp - 0x58]
009d6534 4885c9                     test      rcx, rcx
009d6537 7411                       je        0x1409d654a
009d6539 488b01                     mov       rax, qword ptr [rcx]
009d653c 33d2                       xor       edx, edx
009d653e ff10                       call      qword ptr [rax]
009d6540 488b4da8                   mov       rcx, qword ptr [rbp - 0x58]
009d6544 ff15be128701               call      qword ptr [rip + 0x18712be] ; ?deallocateBytes@NedPoolingPolicy@Ogre@@SAXPEAX@Z
009d654a 4c8965a0                   mov       qword ptr [rbp - 0x60], r12
009d654e 4c8965a8                   mov       qword ptr [rbp - 0x58], r12
009d6552 4883bdd800000010           cmp       qword ptr [rbp + 0xd8], 0x10
009d655a 720c                       jb        0x1409d6568
009d655c 488b8dc0000000             mov       rcx, qword ptr [rbp + 0xc0]
009d6563 e8c0f04f00                 call      0x140ed5628
009d6568 48c785000100000f000000     mov       qword ptr [rbp + 0x100], 0xf
009d6573 4c89a5f8000000             mov       qword ptr [rbp + 0xf8], r12
009d657a c685e800000000             mov       byte ptr [rbp + 0xe8], 0
009d6581 41b80a000000               mov       r8d, 0xa
009d6587 488d1562dbd600             lea       rdx, [rip + 0xd6db62]
009d658e 488d8de8000000             lea       rcx, [rbp + 0xe8]
009d6595 e82b7267ff                 call      0x14004d7c5
009d659a 90                         nop
009d659b ff1507078701               call      qword ptr [rip + 0x1870707] ; ?getSingleton@MeshManager@Ogre@@SAAEAV12@XZ
009d65a1 488bc8                     mov       rcx, rax
009d65a4 c684248800000001           mov       byte ptr [rsp + 0x88], 1
009d65ac c684248000000001           mov       byte ptr [rsp + 0x80], 1
009d65b4 c744247805000000           mov       dword ptr [rsp + 0x78], 5
009d65bc c744247005000000           mov       dword ptr [rsp + 0x70], 5
009d65c4 488b05ed0a8701             mov       rax, qword ptr [rip + 0x1870aed] ; ?UNIT_Z@Vector3@Ogre@@2V12@B
009d65cb 4889442468                 mov       qword ptr [rsp + 0x68], rax
009d65d0 f30f1035304dca00           movss     xmm6, dword ptr [rip + 0xca4d30]
009d65d8 f30f11742460               movss     dword ptr [rsp + 0x60], xmm6
009d65de f30f11742458               movss     dword ptr [rsp + 0x58], xmm6
009d65e4 664489642450               mov       word ptr [rsp + 0x50], r12w
009d65ea c644244801                 mov       byte ptr [rsp + 0x48], 1
009d65ef c744244064000000           mov       dword ptr [rsp + 0x40], 0x64
009d65f7 c744243864000000           mov       dword ptr [rsp + 0x38], 0x64
009d65ff f30f1005e5e2cb00           movss     xmm0, dword ptr [rip + 0xcbe2e5]
009d6607 f30f11442430               movss     dword ptr [rsp + 0x30], xmm0
009d660d f30f11442428               movss     dword ptr [rsp + 0x28], xmm0
009d6613 488d45d0                   lea       rax, [rbp - 0x30]
009d6617 4889442420                 mov       qword ptr [rsp + 0x20], rax
009d661c 4c8b0ddd078701             mov       r9, qword ptr [rip + 0x18707dd] ; ?DEFAULT_RESOURCE_GROUP_NAME@ResourceGroupManager@Ogre@@2V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@A
009d6623 4c8d85e8000000             lea       r8, [rbp + 0xe8]
009d662a 488d55c0                   lea       rdx, [rbp - 0x40]
009d662e ff1544fc8601               call      qword ptr [rip + 0x186fc44] ; ?createPlane@MeshManager@Ogre@@QEAA?AV?$SharedPtr@VMesh@Ogre@@@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@0AEBVPlane@2@MMHH_NGMMAEBVVector3@2@W4Usage@HardwareBuffer@2@422@Z
009d6634 48837dc000                 cmp       qword ptr [rbp - 0x40], 0
009d6639 7429                       je        0x1409d6664
009d663b 488b4dc8                   mov       rcx, qword ptr [rbp - 0x38]
009d663f 8bc3                       mov       eax, ebx
009d6641 f00fc14120                 lock xadd dword ptr [rcx + 0x20], eax
009d6646 ffc8                       dec       eax
009d6648 751a                       jne       0x1409d6664
009d664a 488b4dc8                   mov       rcx, qword ptr [rbp - 0x38]
009d664e 4885c9                     test      rcx, rcx
009d6651 7411                       je        0x1409d6664
009d6653 488b01                     mov       rax, qword ptr [rcx]
009d6656 33d2                       xor       edx, edx
009d6658 ff10                       call      qword ptr [rax]
009d665a 488b4dc8                   mov       rcx, qword ptr [rbp - 0x38]
009d665e ff15a4118701               call      qword ptr [rip + 0x18711a4] ; ?deallocateBytes@NedPoolingPolicy@Ogre@@SAXPEAX@Z
009d6664 4c8965c0                   mov       qword ptr [rbp - 0x40], r12
009d6668 4c8965c8                   mov       qword ptr [rbp - 0x38], r12
009d666c 4883bd0001000010           cmp       qword ptr [rbp + 0x100], 0x10
009d6674 720c                       jb        0x1409d6682
009d6676 488b8de8000000             mov       rcx, qword ptr [rbp + 0xe8]
009d667d e8a6ef4f00                 call      0x140ed5628
009d6682 48c745380f000000           mov       qword ptr [rbp + 0x38], 0xf
009d668a 4c896530                   mov       qword ptr [rbp + 0x30], r12
009d668e c6452000                   mov       byte ptr [rbp + 0x20], 0
009d6692 41b80a000000               mov       r8d, 0xa
009d6698 488d1551dad600             lea       rdx, [rip + 0xd6da51]
009d669f 488d4d20                   lea       rcx, [rbp + 0x20]
009d66a3 e81d7167ff                 call      0x14004d7c5
009d66a8 90                         nop
009d66a9 488b4e10                   mov       rcx, qword ptr [rsi + 0x10]
009d66ad 488b01                     mov       rax, qword ptr [rcx]
009d66b0 41b901000000               mov       r9d, 1
009d66b6 4c8b0543078701             mov       r8, qword ptr [rip + 0x1870743] ; ?DEFAULT_RESOURCE_GROUP_NAME@ResourceGroupManager@Ogre@@2V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@A
009d66bd 488d5520                   lea       rdx, [rbp + 0x20]
009d66c1 ff90b8010000               call      qword ptr [rax + 0x1b8]
009d66c7 488bf8                     mov       rdi, rax
009d66ca 48837d3810                 cmp       qword ptr [rbp + 0x38], 0x10
009d66cf 7209                       jb        0x1409d66da
009d66d1 488b4d20                   mov       rcx, qword ptr [rbp + 0x20]
009d66d5 e84eef4f00                 call      0x140ed5628
009d66da 48c745380f000000           mov       qword ptr [rbp + 0x38], 0xf
009d66e2 4c896530                   mov       qword ptr [rbp + 0x30], r12
009d66e6 c6452000                   mov       byte ptr [rbp + 0x20], 0
009d66ea 33d2                       xor       edx, edx
009d66ec 488bcf                     mov       rcx, rdi
009d66ef ff15eb0e8701               call      qword ptr [rip + 0x1870eeb] ; ?setCastShadows@MovableObject@Ogre@@QEAAX_N@Z
009d66f5 488d5590                   lea       rdx, [rbp - 0x70]
009d66f9 488bcf                     mov       rcx, rdi
009d66fc ff15de058701               call      qword ptr [rip + 0x18705de] ; ?setMaterial@Entity@Ogre@@QEAAXAEBV?$SharedPtr@VMaterial@Ogre@@@2@@Z
009d6702 b201                       mov       dl, 1
009d6704 488bcf                     mov       rcx, rdi
009d6707 ff15fb058701               call      qword ptr [rip + 0x18705fb] ; ?setVisible@MovableObject@Ogre@@QEAAX_N@Z
009d670d 4c8b1f                     mov       r11, qword ptr [rdi]
009d6710 41b801000000               mov       r8d, 1
009d6716 b251                       mov       dl, 0x51
009d6718 488bcf                     mov       rcx, rdi
009d671b 41ff5358                   call      qword ptr [r11 + 0x58]
009d671f ba01000000                 mov       edx, 1
009d6724 488b4e10                   mov       rcx, qword ptr [rsi + 0x10]
009d6728 ff1582078701               call      qword ptr [rip + 0x1870782] ; ?getRootSceneNode@SceneManager@Ogre@@QEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z
009d672e 4c8b10                     mov       r10, qword ptr [rax]
009d6731 4c8b0d40068701             mov       r9, qword ptr [rip + 0x1870640] ; ?IDENTITY@Quaternion@Ogre@@2V12@B
009d6738 4c8b0529058701             mov       r8, qword ptr [rip + 0x1870529] ; ?ZERO@Vector3@Ogre@@2V12@B
009d673f ba01000000                 mov       edx, 1
009d6744 488bc8                     mov       rcx, rax
009d6747 41ff9298000000             call      qword ptr [r10 + 0x98]
009d674e 48894618                   mov       qword ptr [rsi + 0x18], rax
009d6752 488bd7                     mov       rdx, rdi
009d6755 488bc8                     mov       rcx, rax
009d6758 ff1522f98601               call      qword ptr [rip + 0x186f922] ; ?attachObject@SceneNode@Ogre@@QEAAXPEAVMovableObject@2@@Z
009d675e ba10000000                 mov       edx, 0x10
009d6763 488bcf                     mov       rcx, rdi
009d6766 ff156c088701               call      qword ptr [rip + 0x187086c] ; ?setVisibilityFlags@MovableObject@Ogre@@QEAAXI@Z
009d676c f30f100ddcbaca00           movss     xmm1, dword ptr [rip + 0xcabadc]
009d6774 488bce                     mov       rcx, rsi
009d6777 e8057164ff                 call      0x14001d881
009d677c b938000000                 mov       ecx, 0x38
009d6781 e8b4ee4f00                 call      0x140ed563a
009d6786 48894588                   mov       qword ptr [rbp - 0x78], rax
009d678a 4885c0                     test      rax, rax
009d678d 740d                       je        0x1409d679c
009d678f 488bc8                     mov       rcx, rax
009d6792 e8840c66ff                 call      0x14003741b
009d6797 488bd0                     mov       rdx, rax
009d679a eb03                       jmp       0x1409d679f
009d679c 498bd4                     mov       rdx, r12
009d679f 4889153ae47501             mov       qword ptr [rip + 0x175e43a], rdx
009d67a6 488b050bbb7501             mov       rax, qword ptr [rip + 0x175bb0b]
009d67ad 488b4860                   mov       rcx, qword ptr [rax + 0x60]
009d67b1 488b01                     mov       rax, qword ptr [rcx]
009d67b4 ff9018040000               call      qword ptr [rax + 0x418]

00005e10 89542410                   mov       dword ptr [rsp + 0x10], edx
00005e14 48894c2408                 mov       qword ptr [rsp + 8], rcx
00005e19 4881ecb8000000             sub       rsp, 0xb8
00005e20 488b8424c0000000           mov       rax, qword ptr [rsp + 0xc0]
00005e28 488b00                     mov       rax, qword ptr [rax]
00005e2b 8b80ec000000               mov       eax, dword ptr [rax + 0xec]
00005e31 89442428                   mov       dword ptr [rsp + 0x28], eax
00005e35 8b442428                   mov       eax, dword ptr [rsp + 0x28]
00005e39 8d440004                   lea       eax, [rax + rax + 4]
00005e3d 8944242c                   mov       dword ptr [rsp + 0x2c], eax
00005e41 8b9424c8000000             mov       edx, dword ptr [rsp + 0xc8]
00005e48 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
00005e50 e81b050000                 call      0x180006370
00005e55 89442424                   mov       dword ptr [rsp + 0x24], eax
00005e59 837c242400                 cmp       dword ptr [rsp + 0x24], 0
00005e5e 7e1c                       jle       0x180005e7c
00005e60 8b442424                   mov       eax, dword ptr [rsp + 0x24]
00005e64 b901000000                 mov       ecx, 1
00005e69 894c2470                   mov       dword ptr [rsp + 0x70], ecx
00005e6d 0fb6c8                     movzx     ecx, al
00005e70 8b442470                   mov       eax, dword ptr [rsp + 0x70]
00005e74 d3e0                       shl       eax, cl
00005e76 89442474                   mov       dword ptr [rsp + 0x74], eax
00005e7a eb08                       jmp       0x180005e84
00005e7c c744247401000000           mov       dword ptr [rsp + 0x74], 1
00005e84 8b442474                   mov       eax, dword ptr [rsp + 0x74]
00005e88 89442420                   mov       dword ptr [rsp + 0x20], eax
00005e8c 8b8424c8000000             mov       eax, dword ptr [rsp + 0xc8]
00005e93 89442478                   mov       dword ptr [rsp + 0x78], eax
00005e97 837c247800                 cmp       dword ptr [rsp + 0x78], 0
00005e9c 7426                       je        0x180005ec4
00005e9e 837c247801                 cmp       dword ptr [rsp + 0x78], 1
00005ea3 0f8416010000               je        0x180005fbf
00005ea9 837c247802                 cmp       dword ptr [rsp + 0x78], 2
00005eae 0f8470020000               je        0x180006124
00005eb4 837c247803                 cmp       dword ptr [rsp + 0x78], 3
00005eb9 0f841f030000               je        0x1800061de
00005ebf e90f040000                 jmp       0x1800062d3
00005ec4 c744243001000000           mov       dword ptr [rsp + 0x30], 1
00005ecc eb0a                       jmp       0x180005ed8
00005ece 8b442430                   mov       eax, dword ptr [rsp + 0x30]
00005ed2 ffc0                       inc       eax
00005ed4 89442430                   mov       dword ptr [rsp + 0x30], eax
00005ed8 8b442428                   mov       eax, dword ptr [rsp + 0x28]
00005edc ffc8                       dec       eax
00005ede 39442430                   cmp       dword ptr [rsp + 0x30], eax
00005ee2 0f8dd2000000               jge       0x180005fba
00005ee8 8b442430                   mov       eax, dword ptr [rsp + 0x30]
00005eec 99                         cdq
00005eed f77c2420                   idiv      dword ptr [rsp + 0x20]
00005ef1 8bc2                       mov       eax, edx
00005ef3 8944247c                   mov       dword ptr [rsp + 0x7c], eax
00005ef7 8b442420                   mov       eax, dword ptr [rsp + 0x20]
00005efb 99                         cdq
00005efc 2bc2                       sub       eax, edx
00005efe d1f8                       sar       eax, 1
00005f00 8b4c247c                   mov       ecx, dword ptr [rsp + 0x7c]
00005f04 3bc8                       cmp       ecx, eax
00005f06 7f17                       jg        0x180005f1f
00005f08 8b442430                   mov       eax, dword ptr [rsp + 0x30]
00005f0c 99                         cdq
00005f0d f77c2420                   idiv      dword ptr [rsp + 0x20]
00005f11 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
00005f16 89842480000000             mov       dword ptr [rsp + 0x80], eax
00005f1d eb17                       jmp       0x180005f36
00005f1f 8b442430                   mov       eax, dword ptr [rsp + 0x30]
00005f23 99                         cdq
00005f24 f77c2420                   idiv      dword ptr [rsp + 0x20]
00005f28 ffc0                       inc       eax
00005f2a 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
00005f2f 89842480000000             mov       dword ptr [rsp + 0x80], eax
00005f36 8b842480000000             mov       eax, dword ptr [rsp + 0x80]
00005f3d 89442434                   mov       dword ptr [rsp + 0x34], eax
00005f41 8b442430                   mov       eax, dword ptr [rsp + 0x30]
00005f45 0faf44242c                 imul      eax, dword ptr [rsp + 0x2c]
00005f4a 4898                       cdqe
00005f4c 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
00005f54 488b89e0000000             mov       rcx, qword ptr [rcx + 0xe0]
00005f5b 488d0441                   lea       rax, [rcx + rax*2]
00005f5f 4889442438                 mov       qword ptr [rsp + 0x38], rax
00005f64 8b442434                   mov       eax, dword ptr [rsp + 0x34]
00005f68 0faf442428                 imul      eax, dword ptr [rsp + 0x28]
00005f6d 488b4c2438                 mov       rcx, qword ptr [rsp + 0x38]
00005f72 668901                     mov       word ptr [rcx], ax
00005f75 488b442438                 mov       rax, qword ptr [rsp + 0x38]
00005f7a 488b4c2438                 mov       rcx, qword ptr [rsp + 0x38]
00005f7f 0fb709                     movzx     ecx, word ptr [rcx]
00005f82 668948fc                   mov       word ptr [rax - 4], cx
00005f86 488b442438                 mov       rax, qword ptr [rsp + 0x38]
00005f8b 488b4c2438                 mov       rcx, qword ptr [rsp + 0x38]
00005f90 0fb749fc                   movzx     ecx, word ptr [rcx - 4]
00005f94 668948fe                   mov       word ptr [rax - 2], cx
00005f98 b801000000                 mov       eax, 1
00005f9d 2b44242c                   sub       eax, dword ptr [rsp + 0x2c]
00005fa1 4898                       cdqe
00005fa3 488b4c2438                 mov       rcx, qword ptr [rsp + 0x38]
00005fa8 488b542438                 mov       rdx, qword ptr [rsp + 0x38]
00005fad 0fb752fe                   movzx     edx, word ptr [rdx - 2]
00005fb1 66891441                   mov       word ptr [rcx + rax*2], dx
00005fb5 e914ffffff                 jmp       0x180005ece
00005fba e914030000                 jmp       0x1800062d3
00005fbf c744244001000000           mov       dword ptr [rsp + 0x40], 1
00005fc7 eb0a                       jmp       0x180005fd3
00005fc9 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00005fcd ffc0                       inc       eax
00005fcf 89442440                   mov       dword ptr [rsp + 0x40], eax
00005fd3 8b442428                   mov       eax, dword ptr [rsp + 0x28]
00005fd7 ffc8                       dec       eax
00005fd9 39442440                   cmp       dword ptr [rsp + 0x40], eax
00005fdd 0f8d3c010000               jge       0x18000611f
00005fe3 837c242001                 cmp       dword ptr [rsp + 0x20], 1
00005fe8 750d                       jne       0x180005ff7
00005fea 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00005fee 89842484000000             mov       dword ptr [rsp + 0x84], eax
00005ff5 eb62                       jmp       0x180006059
00005ff7 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00005ffb 99                         cdq
00005ffc f77c2420                   idiv      dword ptr [rsp + 0x20]
00006000 8bc2                       mov       eax, edx
00006002 89842488000000             mov       dword ptr [rsp + 0x88], eax
00006009 8b442420                   mov       eax, dword ptr [rsp + 0x20]
0000600d 99                         cdq
0000600e 2bc2                       sub       eax, edx
00006010 d1f8                       sar       eax, 1
00006012 8b8c2488000000             mov       ecx, dword ptr [rsp + 0x88]
00006019 3bc8                       cmp       ecx, eax
0000601b 7d17                       jge       0x180006034
0000601d 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00006021 99                         cdq
00006022 f77c2420                   idiv      dword ptr [rsp + 0x20]
00006026 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
0000602b 8984248c000000             mov       dword ptr [rsp + 0x8c], eax
00006032 eb17                       jmp       0x18000604b
00006034 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00006038 99                         cdq
00006039 f77c2420                   idiv      dword ptr [rsp + 0x20]
0000603d ffc0                       inc       eax
0000603f 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
00006044 8984248c000000             mov       dword ptr [rsp + 0x8c], eax
0000604b 8b84248c000000             mov       eax, dword ptr [rsp + 0x8c]
00006052 89842484000000             mov       dword ptr [rsp + 0x84], eax
00006059 8b842484000000             mov       eax, dword ptr [rsp + 0x84]
00006060 89442444                   mov       dword ptr [rsp + 0x44], eax
00006064 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00006068 0faf44242c                 imul      eax, dword ptr [rsp + 0x2c]
0000606d 4898                       cdqe
0000606f 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
00006077 488b89e0000000             mov       rcx, qword ptr [rcx + 0xe0]
0000607e 488d0441                   lea       rax, [rcx + rax*2]
00006082 48634c242c                 movsxd    rcx, dword ptr [rsp + 0x2c]
00006087 488d4448f4                 lea       rax, [rax + rcx*2 - 0xc]
0000608c 4889442448                 mov       qword ptr [rsp + 0x48], rax
00006091 8b442444                   mov       eax, dword ptr [rsp + 0x44]
00006095 0faf442428                 imul      eax, dword ptr [rsp + 0x28]
0000609a 8b4c2428                   mov       ecx, dword ptr [rsp + 0x28]
0000609e 8d4401ff                   lea       eax, [rcx + rax - 1]
000060a2 488b4c2448                 mov       rcx, qword ptr [rsp + 0x48]
000060a7 668901                     mov       word ptr [rcx], ax
000060aa b803000000                 mov       eax, 3
000060af 2b44242c                   sub       eax, dword ptr [rsp + 0x2c]
000060b3 4898                       cdqe
000060b5 488b4c2448                 mov       rcx, qword ptr [rsp + 0x48]
000060ba 488b542448                 mov       rdx, qword ptr [rsp + 0x48]
000060bf 0fb712                     movzx     edx, word ptr [rdx]
000060c2 66891441                   mov       word ptr [rcx + rax*2], dx
000060c6 b803000000                 mov       eax, 3
000060cb 2b44242c                   sub       eax, dword ptr [rsp + 0x2c]
000060cf 4898                       cdqe
000060d1 b902000000                 mov       ecx, 2
000060d6 2b4c242c                   sub       ecx, dword ptr [rsp + 0x2c]
000060da 4863c9                     movsxd    rcx, ecx
000060dd 488b542448                 mov       rdx, qword ptr [rsp + 0x48]
000060e2 4c8b442448                 mov       r8, qword ptr [rsp + 0x48]
000060e7 410fb70440                 movzx     eax, word ptr [r8 + rax*2]
000060ec 6689044a                   mov       word ptr [rdx + rcx*2], ax
000060f0 b802000000                 mov       eax, 2
000060f5 2b44242c                   sub       eax, dword ptr [rsp + 0x2c]
000060f9 4898                       cdqe
000060fb b901000000                 mov       ecx, 1
00006100 2b4c242c                   sub       ecx, dword ptr [rsp + 0x2c]
00006104 4863c9                     movsxd    rcx, ecx
00006107 488b542448                 mov       rdx, qword ptr [rsp + 0x48]
0000610c 4c8b442448                 mov       r8, qword ptr [rsp + 0x48]
00006111 410fb70440                 movzx     eax, word ptr [r8 + rax*2]
00006116 6689044a                   mov       word ptr [rdx + rcx*2], ax
0000611a e9aafeffff                 jmp       0x180005fc9
0000611f e9af010000                 jmp       0x1800062d3
00006124 c744245001000000           mov       dword ptr [rsp + 0x50], 1
0000612c eb0a                       jmp       0x180006138
0000612e 8b442450                   mov       eax, dword ptr [rsp + 0x50]
00006132 ffc0                       inc       eax
00006134 89442450                   mov       dword ptr [rsp + 0x50], eax
00006138 8b442428                   mov       eax, dword ptr [rsp + 0x28]
0000613c ffc8                       dec       eax
0000613e 39442450                   cmp       dword ptr [rsp + 0x50], eax
00006142 0f8d91000000               jge       0x1800061d9
00006148 8b442450                   mov       eax, dword ptr [rsp + 0x50]
0000614c 99                         cdq
0000614d f77c2420                   idiv      dword ptr [rsp + 0x20]
00006151 8bc2                       mov       eax, edx
00006153 89842490000000             mov       dword ptr [rsp + 0x90], eax
0000615a 8b442420                   mov       eax, dword ptr [rsp + 0x20]
0000615e 99                         cdq
0000615f 2bc2                       sub       eax, edx
00006161 d1f8                       sar       eax, 1
00006163 8b8c2490000000             mov       ecx, dword ptr [rsp + 0x90]
0000616a 3bc8                       cmp       ecx, eax
0000616c 7f17                       jg        0x180006185
0000616e 8b442450                   mov       eax, dword ptr [rsp + 0x50]
00006172 99                         cdq
00006173 f77c2420                   idiv      dword ptr [rsp + 0x20]
00006177 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
0000617c 89842494000000             mov       dword ptr [rsp + 0x94], eax
00006183 eb17                       jmp       0x18000619c
00006185 8b442450                   mov       eax, dword ptr [rsp + 0x50]
00006189 99                         cdq
0000618a f77c2420                   idiv      dword ptr [rsp + 0x20]
0000618e ffc0                       inc       eax
00006190 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
00006195 89842494000000             mov       dword ptr [rsp + 0x94], eax
0000619c 8b842494000000             mov       eax, dword ptr [rsp + 0x94]
000061a3 89442454                   mov       dword ptr [rsp + 0x54], eax
000061a7 8b442450                   mov       eax, dword ptr [rsp + 0x50]
000061ab d1e0                       shl       eax, 1
000061ad 4898                       cdqe
000061af 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
000061b7 488b89e0000000             mov       rcx, qword ptr [rcx + 0xe0]
000061be 488d0441                   lea       rax, [rcx + rax*2]
000061c2 4889442458                 mov       qword ptr [rsp + 0x58], rax
000061c7 488b442458                 mov       rax, qword ptr [rsp + 0x58]
000061cc 0fb74c2454                 movzx     ecx, word ptr [rsp + 0x54]
000061d1 668908                     mov       word ptr [rax], cx
000061d4 e955ffffff                 jmp       0x18000612e
000061d9 e9f5000000                 jmp       0x1800062d3
000061de c744246001000000           mov       dword ptr [rsp + 0x60], 1
000061e6 eb0a                       jmp       0x1800061f2
000061e8 8b442460                   mov       eax, dword ptr [rsp + 0x60]
000061ec ffc0                       inc       eax
000061ee 89442460                   mov       dword ptr [rsp + 0x60], eax
000061f2 8b442428                   mov       eax, dword ptr [rsp + 0x28]
000061f6 ffc8                       dec       eax
000061f8 39442460                   cmp       dword ptr [rsp + 0x60], eax
000061fc 0f8dd1000000               jge       0x1800062d3
00006202 837c242001                 cmp       dword ptr [rsp + 0x20], 1
00006207 750d                       jne       0x180006216
00006209 8b442460                   mov       eax, dword ptr [rsp + 0x60]
0000620d 89842498000000             mov       dword ptr [rsp + 0x98], eax
00006214 eb62                       jmp       0x180006278
00006216 8b442460                   mov       eax, dword ptr [rsp + 0x60]
0000621a 99                         cdq
0000621b f77c2420                   idiv      dword ptr [rsp + 0x20]
0000621f 8bc2                       mov       eax, edx
00006221 8984249c000000             mov       dword ptr [rsp + 0x9c], eax
00006228 8b442420                   mov       eax, dword ptr [rsp + 0x20]
0000622c 99                         cdq
0000622d 2bc2                       sub       eax, edx
0000622f d1f8                       sar       eax, 1
00006231 8b8c249c000000             mov       ecx, dword ptr [rsp + 0x9c]
00006238 3bc8                       cmp       ecx, eax
0000623a 7d17                       jge       0x180006253
0000623c 8b442460                   mov       eax, dword ptr [rsp + 0x60]
00006240 99                         cdq
00006241 f77c2420                   idiv      dword ptr [rsp + 0x20]
00006245 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
0000624a 898424a0000000             mov       dword ptr [rsp + 0xa0], eax
00006251 eb17                       jmp       0x18000626a
00006253 8b442460                   mov       eax, dword ptr [rsp + 0x60]
00006257 99                         cdq
00006258 f77c2420                   idiv      dword ptr [rsp + 0x20]
0000625c ffc0                       inc       eax
0000625e 0faf442420                 imul      eax, dword ptr [rsp + 0x20]
00006263 898424a0000000             mov       dword ptr [rsp + 0xa0], eax
0000626a 8b8424a0000000             mov       eax, dword ptr [rsp + 0xa0]
00006271 89842498000000             mov       dword ptr [rsp + 0x98], eax
00006278 8b842498000000             mov       eax, dword ptr [rsp + 0x98]
0000627f 89442464                   mov       dword ptr [rsp + 0x64], eax
00006283 8b442428                   mov       eax, dword ptr [rsp + 0x28]
00006287 83e802                     sub       eax, 2
0000628a 0faf44242c                 imul      eax, dword ptr [rsp + 0x2c]
0000628f 4898                       cdqe
00006291 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
00006299 488b89e0000000             mov       rcx, qword ptr [rcx + 0xe0]
000062a0 488d0441                   lea       rax, [rcx + rax*2]
000062a4 8b4c2460                   mov       ecx, dword ptr [rsp + 0x60]
000062a8 d1e1                       shl       ecx, 1
000062aa 4863c9                     movsxd    rcx, ecx
000062ad 488d444802                 lea       rax, [rax + rcx*2 + 2]
000062b2 4889442468                 mov       qword ptr [rsp + 0x68], rax
000062b7 8b442428                   mov       eax, dword ptr [rsp + 0x28]
000062bb ffc8                       dec       eax
000062bd 0faf442428                 imul      eax, dword ptr [rsp + 0x28]
000062c2 03442464                   add       eax, dword ptr [rsp + 0x64]
000062c6 488b4c2468                 mov       rcx, qword ptr [rsp + 0x68]
000062cb 668901                     mov       word ptr [rcx], ax
000062ce e915ffffff                 jmp       0x1800061e8
000062d3 8b8424c8000000             mov       eax, dword ptr [rsp + 0xc8]
000062da b910000000                 mov       ecx, 0x10
000062df 898c24a4000000             mov       dword ptr [rsp + 0xa4], ecx
000062e6 0fb6c8                     movzx     ecx, al
000062e9 8b8424a4000000             mov       eax, dword ptr [rsp + 0xa4]
000062f0 d3e0                       shl       eax, cl
000062f2 f7d0                       not       eax
000062f4 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
000062fc 0fb689b8000000             movzx     ecx, byte ptr [rcx + 0xb8]
00006303 23c8                       and       ecx, eax
00006305 8bc1                       mov       eax, ecx
00006307 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
0000630f 8881b8000000               mov       byte ptr [rcx + 0xb8], al
00006315 488b8424c0000000           mov       rax, qword ptr [rsp + 0xc0]
0000631d 0fb780be000000             movzx     eax, word ptr [rax + 0xbe]
00006324 8b8c24c8000000             mov       ecx, dword ptr [rsp + 0xc8]
0000632b c1e102                     shl       ecx, 2
0000632e ba0f000000                 mov       edx, 0xf
00006333 d3e2                       shl       edx, cl
00006335 8bca                       mov       ecx, edx
00006337 f7d1                       not       ecx
00006339 23c1                       and       eax, ecx
0000633b 8b8c24c8000000             mov       ecx, dword ptr [rsp + 0xc8]
00006342 c1e102                     shl       ecx, 2
00006345 8b542424                   mov       edx, dword ptr [rsp + 0x24]
00006349 d3e2                       shl       edx, cl
0000634b 8bca                       mov       ecx, edx
0000634d 0bc1                       or        eax, ecx
0000634f 488b8c24c0000000           mov       rcx, qword ptr [rsp + 0xc0]
00006357 668981be000000             mov       word ptr [rcx + 0xbe], ax
0000635e 4881c4b8000000             add       rsp, 0xb8
00006365 c3                         ret
00006366 cc                         int3
00006367 cc                         int3
00006368 cc                         int3
00006369 cc                         int3
0000636a cc                         int3
0000636b cc                         int3
0000636c cc                         int3
0000636d cc                         int3
0000636e cc                         int3
0000636f cc                         int3

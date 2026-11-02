00003960 44894c2420                 mov       dword ptr [rsp + 0x20], r9d
00003965 4c89442418                 mov       qword ptr [rsp + 0x18], r8
0000396a 4889542410                 mov       qword ptr [rsp + 0x10], rdx
0000396f 48894c2408                 mov       qword ptr [rsp + 8], rcx
00003974 4883ec38                   sub       rsp, 0x38
00003978 488b442440                 mov       rax, qword ptr [rsp + 0x40]
0000397d 4883c008                   add       rax, 8
00003981 448b442458                 mov       r8d, dword ptr [rsp + 0x58]
00003986 488bd0                     mov       rdx, rax
00003989 488b4c2448                 mov       rcx, qword ptr [rsp + 0x48]
0000398e e8bde20000                 call      0x180011c50
00003993 89442420                   mov       dword ptr [rsp + 0x20], eax
00003997 837c242000                 cmp       dword ptr [rsp + 0x20], 0
0000399c 7502                       jne       0x1800039a0
0000399e eb73                       jmp       0x180003a13
000039a0 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000039a5 8b8080000000               mov       eax, dword ptr [rax + 0x80]
000039ab 83e001                     and       eax, 1
000039ae 85c0                       test      eax, eax
000039b0 7442                       je        0x1800039f4
000039b2 c744242400000000           mov       dword ptr [rsp + 0x24], 0
000039ba eb0a                       jmp       0x1800039c6
000039bc 8b442424                   mov       eax, dword ptr [rsp + 0x24]
000039c0 ffc0                       inc       eax
000039c2 89442424                   mov       dword ptr [rsp + 0x24], eax
000039c6 837c242404                 cmp       dword ptr [rsp + 0x24], 4
000039cb 7d25                       jge       0x1800039f2
000039cd 4863442424                 movsxd    rax, dword ptr [rsp + 0x24]
000039d2 448b4c2420                 mov       r9d, dword ptr [rsp + 0x20]
000039d7 4c8b442450                 mov       r8, qword ptr [rsp + 0x50]
000039dc 488b542448                 mov       rdx, qword ptr [rsp + 0x48]
000039e1 488b4c2440                 mov       rcx, qword ptr [rsp + 0x40]
000039e6 488b4cc150                 mov       rcx, qword ptr [rcx + rax*8 + 0x50]
000039eb e870ffffff                 call      0x180003960
000039f0 ebca                       jmp       0x1800039bc
000039f2 eb1f                       jmp       0x180003a13
000039f4 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000039f9 4805c8000000               add       rax, 0xc8
000039ff 4889442428                 mov       qword ptr [rsp + 0x28], rax
00003a04 488d542428                 lea       rdx, [rsp + 0x28]
00003a09 488b4c2450                 mov       rcx, qword ptr [rsp + 0x50]
00003a0e e89d630000                 call      0x180009db0
00003a13 4883c438                   add       rsp, 0x38
00003a17 c3                         ret
00003a18 cc                         int3
00003a19 cc                         int3
00003a1a cc                         int3
00003a1b cc                         int3
00003a1c cc                         int3
00003a1d cc                         int3
00003a1e cc                         int3
00003a1f cc                         int3

00005aa0 48894c2408                 mov       qword ptr [rsp + 8], rcx
00005aa5 4883ec78                   sub       rsp, 0x78
00005aa9 488b842480000000           mov       rax, qword ptr [rsp + 0x80]
00005ab1 488b00                     mov       rax, qword ptr [rax]
00005ab4 8b80ec000000               mov       eax, dword ptr [rax + 0xec]
00005aba 89442438                   mov       dword ptr [rsp + 0x38], eax
00005abe 8b442438                   mov       eax, dword ptr [rsp + 0x38]
00005ac2 8d440004                   lea       eax, [rax + rax + 4]
00005ac6 8b4c2438                   mov       ecx, dword ptr [rsp + 0x38]
00005aca ffc9                       dec       ecx
00005acc 0fafc1                     imul      eax, ecx
00005acf 83e804                     sub       eax, 4
00005ad2 89442420                   mov       dword ptr [rsp + 0x20], eax
00005ad6 488b842480000000           mov       rax, qword ptr [rsp + 0x80]
00005ade 4883b8e000000000           cmp       qword ptr [rax + 0xe0], 0
00005ae6 741e                       je        0x180005b06
00005ae8 488b842480000000           mov       rax, qword ptr [rsp + 0x80]
00005af0 488b80e0000000             mov       rax, qword ptr [rax + 0xe0]
00005af7 4889442448                 mov       qword ptr [rsp + 0x48], rax
00005afc 488b4c2448                 mov       rcx, qword ptr [rsp + 0x48]
00005b01 e8e8c50100                 call      0x1800220ee
00005b06 4863442420                 movsxd    rax, dword ptr [rsp + 0x20]
00005b0b 4889442458                 mov       qword ptr [rsp + 0x58], rax
00005b10 b802000000                 mov       eax, 2
00005b15 488b4c2458                 mov       rcx, qword ptr [rsp + 0x58]
00005b1a 48f7e1                     mul       rcx
00005b1d 48c7c1ffffffff             mov       rcx, 0xffffffffffffffff
00005b24 480f40c1                   cmovo     rax, rcx
00005b28 488bc8                     mov       rcx, rax
00005b2b e870c50100                 call      0x1800220a0
00005b30 4889442450                 mov       qword ptr [rsp + 0x50], rax
00005b35 488b442450                 mov       rax, qword ptr [rsp + 0x50]
00005b3a 4889442430                 mov       qword ptr [rsp + 0x30], rax
00005b3f 488b442430                 mov       rax, qword ptr [rsp + 0x30]
00005b44 4889442428                 mov       qword ptr [rsp + 0x28], rax
00005b49 c744243c00000000           mov       dword ptr [rsp + 0x3c], 0
00005b51 eb0a                       jmp       0x180005b5d
00005b53 8b44243c                   mov       eax, dword ptr [rsp + 0x3c]
00005b57 ffc0                       inc       eax
00005b59 8944243c                   mov       dword ptr [rsp + 0x3c], eax
00005b5d 8b442438                   mov       eax, dword ptr [rsp + 0x38]
00005b61 ffc8                       dec       eax
00005b63 3944243c                   cmp       dword ptr [rsp + 0x3c], eax
00005b67 0f8dc9000000               jge       0x180005c36
00005b6d 837c243c00                 cmp       dword ptr [rsp + 0x3c], 0
00005b72 7e5a                       jle       0x180005bce
00005b74 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005b79 488b4c2428                 mov       rcx, qword ptr [rsp + 0x28]
00005b7e 0fb749fe                   movzx     ecx, word ptr [rcx - 2]
00005b82 66894802                   mov       word ptr [rax + 2], cx
00005b86 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005b8b 488b4c2428                 mov       rcx, qword ptr [rsp + 0x28]
00005b90 0fb74902                   movzx     ecx, word ptr [rcx + 2]
00005b94 668908                     mov       word ptr [rax], cx
00005b97 8b44243c                   mov       eax, dword ptr [rsp + 0x3c]
00005b9b 0faf442438                 imul      eax, dword ptr [rsp + 0x38]
00005ba0 89442460                   mov       dword ptr [rsp + 0x60], eax
00005ba4 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005ba9 0fb74c2460                 movzx     ecx, word ptr [rsp + 0x60]
00005bae 66894806                   mov       word ptr [rax + 6], cx
00005bb2 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005bb7 0fb74c2460                 movzx     ecx, word ptr [rsp + 0x60]
00005bbc 66894804                   mov       word ptr [rax + 4], cx
00005bc0 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005bc5 4883c008                   add       rax, 8
00005bc9 4889442428                 mov       qword ptr [rsp + 0x28], rax
00005bce c744244000000000           mov       dword ptr [rsp + 0x40], 0
00005bd6 eb18                       jmp       0x180005bf0
00005bd8 8b442440                   mov       eax, dword ptr [rsp + 0x40]
00005bdc ffc0                       inc       eax
00005bde 89442440                   mov       dword ptr [rsp + 0x40], eax
00005be2 488b442428                 mov       rax, qword ptr [rsp + 0x28]
00005be7 4883c004                   add       rax, 4
00005beb 4889442428                 mov       qword ptr [rsp + 0x28], rax
00005bf0 8b442438                   mov       eax, dword ptr [rsp + 0x38]
00005bf4 39442440                   cmp       dword ptr [rsp + 0x40], eax
00005bf8 7d37                       jge       0x180005c31
00005bfa 8b44243c                   mov       eax, dword ptr [rsp + 0x3c]
00005bfe 0faf442438                 imul      eax, dword ptr [rsp + 0x38]
00005c03 8b4c2440                   mov       ecx, dword ptr [rsp + 0x40]
00005c07 03c8                       add       ecx, eax
00005c09 8bc1                       mov       eax, ecx
00005c0b 488b4c2428                 mov       rcx, qword ptr [rsp + 0x28]
00005c10 668901                     mov       word ptr [rcx], ax
00005c13 8b44243c                   mov       eax, dword ptr [rsp + 0x3c]
00005c17 ffc0                       inc       eax
00005c19 0faf442438                 imul      eax, dword ptr [rsp + 0x38]
00005c1e 8b4c2440                   mov       ecx, dword ptr [rsp + 0x40]
00005c22 03c8                       add       ecx, eax
00005c24 8bc1                       mov       eax, ecx
00005c26 488b4c2428                 mov       rcx, qword ptr [rsp + 0x28]
00005c2b 66894102                   mov       word ptr [rcx + 2], ax
00005c2f eba7                       jmp       0x180005bd8
00005c31 e91dffffff                 jmp       0x180005b53
00005c36 33c0                       xor       eax, eax
00005c38 488b8c2480000000           mov       rcx, qword ptr [rsp + 0x80]
00005c40 668981be000000             mov       word ptr [rcx + 0xbe], ax
00005c47 488b842480000000           mov       rax, qword ptr [rsp + 0x80]
00005c4f 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
00005c54 488988e0000000             mov       qword ptr [rax + 0xe0], rcx
00005c5b 4863442420                 movsxd    rax, dword ptr [rsp + 0x20]
00005c60 488b8c2480000000           mov       rcx, qword ptr [rsp + 0x80]
00005c68 488981d0000000             mov       qword ptr [rcx + 0xd0], rax
00005c6f 4883c478                   add       rsp, 0x78
00005c73 c3                         ret

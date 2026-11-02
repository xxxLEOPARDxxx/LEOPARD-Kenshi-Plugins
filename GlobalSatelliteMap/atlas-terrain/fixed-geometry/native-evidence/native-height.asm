00014430 4489442418                 mov       dword ptr [rsp + 0x18], r8d
00014435 4889542410                 mov       qword ptr [rsp + 0x10], rdx
0001443a 48894c2408                 mov       qword ptr [rsp + 8], rcx
0001443f 4883ec38                   sub       rsp, 0x38
00014443 488b442448                 mov       rax, qword ptr [rsp + 0x48]
00014448 488b4c2440                 mov       rcx, qword ptr [rsp + 0x40]
0001444d f30f1000                   movss     xmm0, dword ptr [rax]
00014451 f30f5c81d4010000           subss     xmm0, dword ptr [rcx + 0x1d4]
00014459 488b442440                 mov       rax, qword ptr [rsp + 0x40]
0001445e f30f5980d8010000           mulss     xmm0, dword ptr [rax + 0x1d8]
00014466 f30f2cc0                   cvttss2si eax, xmm0
0001446a 89442428                   mov       dword ptr [rsp + 0x28], eax
0001446e 488b442448                 mov       rax, qword ptr [rsp + 0x48]
00014473 488b4c2440                 mov       rcx, qword ptr [rsp + 0x40]
00014478 f30f104008                 movss     xmm0, dword ptr [rax + 8]
0001447d f30f5c81d4010000           subss     xmm0, dword ptr [rcx + 0x1d4]
00014485 488b442440                 mov       rax, qword ptr [rsp + 0x40]
0001448a f30f5980d8010000           mulss     xmm0, dword ptr [rax + 0x1d8]
00014492 f30f2cc0                   cvttss2si eax, xmm0
00014496 89442424                   mov       dword ptr [rsp + 0x24], eax
0001449a 837c242800                 cmp       dword ptr [rsp + 0x28], 0
0001449f 7c35                       jl        0x1800144d6
000144a1 837c242400                 cmp       dword ptr [rsp + 0x24], 0
000144a6 7c2e                       jl        0x1800144d6
000144a8 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000144ad 488b88b8010000             mov       rcx, qword ptr [rax + 0x1b8]
000144b4 e897feffff                 call      0x180014350
000144b9 39442428                   cmp       dword ptr [rsp + 0x28], eax
000144bd 7d17                       jge       0x1800144d6
000144bf 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000144c4 488b88b8010000             mov       rcx, qword ptr [rax + 0x1b8]
000144cb e880010000                 call      0x180014650
000144d0 39442424                   cmp       dword ptr [rsp + 0x24], eax
000144d4 7c08                       jl        0x1800144de
000144d6 0f57c0                     xorps     xmm0, xmm0
000144d9 e95f010000                 jmp       0x18001463d
000144de 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000144e3 8b8060020000               mov       eax, dword ptr [rax + 0x260]
000144e9 39442428                   cmp       dword ptr [rsp + 0x28], eax
000144ed 0f8c12010000               jl        0x180014605
000144f3 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000144f8 8b8064020000               mov       eax, dword ptr [rax + 0x264]
000144fe 39442428                   cmp       dword ptr [rsp + 0x28], eax
00014502 0f8dfd000000               jge       0x180014605
00014508 488b442440                 mov       rax, qword ptr [rsp + 0x40]
0001450d 8b805c020000               mov       eax, dword ptr [rax + 0x25c]
00014513 39442424                   cmp       dword ptr [rsp + 0x24], eax
00014517 0f8ce8000000               jl        0x180014605
0001451d 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014522 8b8068020000               mov       eax, dword ptr [rax + 0x268]
00014528 39442424                   cmp       dword ptr [rsp + 0x24], eax
0001452c 0f8dd3000000               jge       0x180014605
00014532 c744242c00000000           mov       dword ptr [rsp + 0x2c], 0
0001453a 488b442440                 mov       rax, qword ptr [rsp + 0x40]
0001453f 8b806c020000               mov       eax, dword ptr [rax + 0x26c]
00014545 39442428                   cmp       dword ptr [rsp + 0x28], eax
00014549 7d19                       jge       0x180014564
0001454b 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014550 8b8060020000               mov       eax, dword ptr [rax + 0x260]
00014556 8b4c2428                   mov       ecx, dword ptr [rsp + 0x28]
0001455a 2bc8                       sub       ecx, eax
0001455c 8bc1                       mov       eax, ecx
0001455e 89442428                   mov       dword ptr [rsp + 0x28], eax
00014562 eb1f                       jmp       0x180014583
00014564 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014569 8b806c020000               mov       eax, dword ptr [rax + 0x26c]
0001456f 8b4c2428                   mov       ecx, dword ptr [rsp + 0x28]
00014573 2bc8                       sub       ecx, eax
00014575 8bc1                       mov       eax, ecx
00014577 89442428                   mov       dword ptr [rsp + 0x28], eax
0001457b c744242c01000000           mov       dword ptr [rsp + 0x2c], 1
00014583 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014588 8b8070020000               mov       eax, dword ptr [rax + 0x270]
0001458e 39442424                   cmp       dword ptr [rsp + 0x24], eax
00014592 7d19                       jge       0x1800145ad
00014594 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014599 8b805c020000               mov       eax, dword ptr [rax + 0x25c]
0001459f 8b4c2424                   mov       ecx, dword ptr [rsp + 0x24]
000145a3 2bc8                       sub       ecx, eax
000145a5 8bc1                       mov       eax, ecx
000145a7 89442424                   mov       dword ptr [rsp + 0x24], eax
000145ab eb22                       jmp       0x1800145cf
000145ad 488b442440                 mov       rax, qword ptr [rsp + 0x40]
000145b2 8b8070020000               mov       eax, dword ptr [rax + 0x270]
000145b8 8b4c2424                   mov       ecx, dword ptr [rsp + 0x24]
000145bc 2bc8                       sub       ecx, eax
000145be 8bc1                       mov       eax, ecx
000145c0 89442424                   mov       dword ptr [rsp + 0x24], eax
000145c4 8b44242c                   mov       eax, dword ptr [rsp + 0x2c]
000145c8 83c802                     or        eax, 2
000145cb 8944242c                   mov       dword ptr [rsp + 0x2c], eax
000145cf 486344242c                 movsxd    rax, dword ptr [rsp + 0x2c]
000145d4 488b4c2440                 mov       rcx, qword ptr [rsp + 0x40]
000145d9 8b542424                   mov       edx, dword ptr [rsp + 0x24]
000145dd 0faf9158020000             imul      edx, dword ptr [rcx + 0x258]
000145e4 8bca                       mov       ecx, edx
000145e6 8b542428                   mov       edx, dword ptr [rsp + 0x28]
000145ea 03d1                       add       edx, ecx
000145ec 8bca                       mov       ecx, edx
000145ee 4863c9                     movsxd    rcx, ecx
000145f1 488b542440                 mov       rdx, qword ptr [rsp + 0x40]
000145f6 488b84c280020000           mov       rax, qword ptr [rdx + rax*8 + 0x280]
000145fe f30f100488                 movss     xmm0, dword ptr [rax + rcx*4]
00014603 eb38                       jmp       0x18001463d
00014605 4c8d4c2420                 lea       r9, [rsp + 0x20]
0001460a 448b442424                 mov       r8d, dword ptr [rsp + 0x24]
0001460f 8b542428                   mov       edx, dword ptr [rsp + 0x28]
00014613 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014618 488b88b8010000             mov       rcx, qword ptr [rax + 0x1b8]
0001461f e81cd00000                 call      0x180021640
00014624 0fb7442420                 movzx     eax, word ptr [rsp + 0x20]
00014629 660f6ec0                   movd      xmm0, eax
0001462d 0f5bc0                     cvtdq2ps  xmm0, xmm0
00014630 488b442440                 mov       rax, qword ptr [rsp + 0x40]
00014635 f30f5980d0010000           mulss     xmm0, dword ptr [rax + 0x1d0]
0001463d 4883c438                   add       rsp, 0x38
00014641 c3                         ret

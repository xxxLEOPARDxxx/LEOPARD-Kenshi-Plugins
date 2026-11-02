0001f5c0 4889542410                 mov       qword ptr [rsp + 0x10], rdx
0001f5c5 48894c2408                 mov       qword ptr [rsp + 8], rcx
0001f5ca 56                         push      rsi
0001f5cb 57                         push      rdi
0001f5cc 4881ecc8000000             sub       rsp, 0xc8
0001f5d3 48c7442468feffffff         mov       qword ptr [rsp + 0x68], 0xfffffffffffffffe
0001f5dc 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f5e4 488b80a8000000             mov       rax, qword ptr [rax + 0xa8]
0001f5eb 4889442430                 mov       qword ptr [rsp + 0x30], rax
0001f5f0 488b442430                 mov       rax, qword ptr [rsp + 0x30]
0001f5f5 488b00                     mov       rax, qword ptr [rax]
0001f5f8 66c74424280000             mov       word ptr [rsp + 0x28], 0
0001f5ff c744242001000000           mov       dword ptr [rsp + 0x20], 1
0001f607 41b903000000               mov       r9d, 3
0001f60d 4533c0                     xor       r8d, r8d
0001f610 33d2                       xor       edx, edx
0001f612 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0001f617 ff5008                     call      qword ptr [rax + 8]
0001f61a 488b442430                 mov       rax, qword ptr [rsp + 0x30]
0001f61f 488b00                     mov       rax, qword ptr [rax]
0001f622 66c74424280000             mov       word ptr [rsp + 0x28], 0
0001f629 c744242004000000           mov       dword ptr [rsp + 0x20], 4
0001f631 41b902000000               mov       r9d, 2
0001f637 41b810000000               mov       r8d, 0x10
0001f63d 33d2                       xor       edx, edx
0001f63f 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0001f644 ff5008                     call      qword ptr [rax + 8]
0001f647 488b442430                 mov       rax, qword ptr [rsp + 0x30]
0001f64c 488b00                     mov       rax, qword ptr [rax]
0001f64f 66c74424280000             mov       word ptr [rsp + 0x28], 0
0001f656 c744242008000000           mov       dword ptr [rsp + 0x20], 8
0001f65e 41b902000000               mov       r9d, 2
0001f664 41b81c000000               mov       r8d, 0x1c
0001f66a 33d2                       xor       edx, edx
0001f66c 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0001f671 ff5008                     call      qword ptr [rax + 8]
0001f674 ff15665e0100               call      qword ptr [rip + 0x15e66] ; ?getSingleton@HardwareBufferManager@Ogre@@SAAEAV12@XZ
0001f67a 4889442470                 mov       qword ptr [rsp + 0x70], rax
0001f67f 488b442430                 mov       rax, qword ptr [rsp + 0x30]
0001f684 488b00                     mov       rax, qword ptr [rax]
0001f687 33d2                       xor       edx, edx
0001f689 488b4c2430                 mov       rcx, qword ptr [rsp + 0x30]
0001f68e ff5048                     call      qword ptr [rax + 0x48]
0001f691 488b4c2470                 mov       rcx, qword ptr [rsp + 0x70]
0001f696 488b09                     mov       rcx, qword ptr [rcx]
0001f699 48894c2478                 mov       qword ptr [rsp + 0x78], rcx
0001f69e c644242800                 mov       byte ptr [rsp + 0x28], 0
0001f6a3 c744242005000000           mov       dword ptr [rsp + 0x20], 5
0001f6ab 488b9424e8000000           mov       rdx, qword ptr [rsp + 0xe8]
0001f6b3 4c8b0a                     mov       r9, qword ptr [rdx]
0001f6b6 4c8bc0                     mov       r8, rax
0001f6b9 488d542448                 lea       rdx, [rsp + 0x48]
0001f6be 488b4c2470                 mov       rcx, qword ptr [rsp + 0x70]
0001f6c3 488b442478                 mov       rax, qword ptr [rsp + 0x78]
0001f6c8 ff5040                     call      qword ptr [rax + 0x40]
0001f6cb 90                         nop
0001f6cc 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f6d4 488b80b0000000             mov       rax, qword ptr [rax + 0xb0]
0001f6db 488b8c24e0000000           mov       rcx, qword ptr [rsp + 0xe0]
0001f6e3 488b89b0000000             mov       rcx, qword ptr [rcx + 0xb0]
0001f6ea 488b00                     mov       rax, qword ptr [rax]
0001f6ed 4c8d442448                 lea       r8, [rsp + 0x48]
0001f6f2 33d2                       xor       edx, edx
0001f6f4 ff5008                     call      qword ptr [rax + 8]
0001f6f7 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f6ff 488b8c24e8000000           mov       rcx, qword ptr [rsp + 0xe8]
0001f707 488b09                     mov       rcx, qword ptr [rcx]
0001f70a 488988c8000000             mov       qword ptr [rax + 0xc8], rcx
0001f711 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f719 48c780c000000000000000     mov       qword ptr [rax + 0xc0], 0
0001f724 488d4c2448                 lea       rcx, [rsp + 0x48]
0001f729 ff15a95d0100               call      qword ptr [rip + 0x15da9] ; ??C?$SharedPtr@VHardwareVertexBuffer@Ogre@@@Ogre@@QEBAPEAVHardwareVertexBuffer@1@XZ
0001f72f 4533c0                     xor       r8d, r8d
0001f732 ba01000000                 mov       edx, 1
0001f737 488bc8                     mov       rcx, rax
0001f73a ff15905d0100               call      qword ptr [rip + 0x15d90] ; ?lock@HardwareBuffer@Ogre@@QEAAPEAXW4LockOptions@12@W4UploadOptions@12@@Z
0001f740 4889442440                 mov       qword ptr [rsp + 0x40], rax
0001f745 488b8424e8000000           mov       rax, qword ptr [rsp + 0xe8]
0001f74d 488b00                     mov       rax, qword ptr [rax]
0001f750 486bc00a                   imul      rax, rax, 0xa
0001f754 48c1e002                   shl       rax, 2
0001f758 488b8c24e8000000           mov       rcx, qword ptr [rsp + 0xe8]
0001f760 488b7c2440                 mov       rdi, qword ptr [rsp + 0x40]
0001f765 488b7110                   mov       rsi, qword ptr [rcx + 0x10]
0001f769 488bc8                     mov       rcx, rax
0001f76c f3a4                       rep movsb byte ptr [rdi], byte ptr [rsi]
0001f76e 488d4c2448                 lea       rcx, [rsp + 0x48]
0001f773 ff155f5d0100               call      qword ptr [rip + 0x15d5f] ; ??C?$SharedPtr@VHardwareVertexBuffer@Ogre@@@Ogre@@QEBAPEAVHardwareVertexBuffer@1@XZ
0001f779 4889842480000000           mov       qword ptr [rsp + 0x80], rax
0001f781 488b842480000000           mov       rax, qword ptr [rsp + 0x80]
0001f789 488b00                     mov       rax, qword ptr [rax]
0001f78c 488b8c2480000000           mov       rcx, qword ptr [rsp + 0x80]
0001f794 ff5020                     call      qword ptr [rax + 0x20]
0001f797 ff15435d0100               call      qword ptr [rip + 0x15d43] ; ?getSingleton@HardwareBufferManager@Ogre@@SAAEAV12@XZ
0001f79d 4889842488000000           mov       qword ptr [rsp + 0x88], rax
0001f7a5 488b842488000000           mov       rax, qword ptr [rsp + 0x88]
0001f7ad 488b00                     mov       rax, qword ptr [rax]
0001f7b0 c644242800                 mov       byte ptr [rsp + 0x28], 0
0001f7b5 c744242005000000           mov       dword ptr [rsp + 0x20], 5
0001f7bd 488b8c24e8000000           mov       rcx, qword ptr [rsp + 0xe8]
0001f7c5 4c8b4908                   mov       r9, qword ptr [rcx + 8]
0001f7c9 4533c0                     xor       r8d, r8d
0001f7cc 488d542458                 lea       rdx, [rsp + 0x58]
0001f7d1 488b8c2488000000           mov       rcx, qword ptr [rsp + 0x88]
0001f7d9 ff5048                     call      qword ptr [rax + 0x48]
0001f7dc 4889842490000000           mov       qword ptr [rsp + 0x90], rax
0001f7e4 488b842490000000           mov       rax, qword ptr [rsp + 0x90]
0001f7ec 4889842498000000           mov       qword ptr [rsp + 0x98], rax
0001f7f4 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f7fc 4883c078                   add       rax, 0x78
0001f800 488b942498000000           mov       rdx, qword ptr [rsp + 0x98]
0001f808 488bc8                     mov       rcx, rax
0001f80b ff15b75c0100               call      qword ptr [rip + 0x15cb7] ; ??4HardwareIndexBufferSharedPtr@Ogre@@QEAAAEAV01@AEBV01@@Z
0001f811 90                         nop
0001f812 488d4c2458                 lea       rcx, [rsp + 0x58]
0001f817 ff15a35c0100               call      qword ptr [rip + 0x15ca3] ; ??1HardwareIndexBufferSharedPtr@Ogre@@QEAA@XZ
0001f81d 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f825 4883c078                   add       rax, 0x78
0001f829 488bc8                     mov       rcx, rax
0001f82c ff15865c0100               call      qword ptr [rip + 0x15c86] ; ??C?$SharedPtr@VHardwareIndexBuffer@Ogre@@@Ogre@@QEBAPEAVHardwareIndexBuffer@1@XZ
0001f832 48898424a0000000           mov       qword ptr [rsp + 0xa0], rax
0001f83a 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f842 4883c078                   add       rax, 0x78
0001f846 488bc8                     mov       rcx, rax
0001f849 ff15695c0100               call      qword ptr [rip + 0x15c69] ; ??C?$SharedPtr@VHardwareIndexBuffer@Ogre@@@Ogre@@QEBAPEAVHardwareIndexBuffer@1@XZ
0001f84f 488bc8                     mov       rcx, rax
0001f852 ff15585c0100               call      qword ptr [rip + 0x15c58] ; ?getSizeInBytes@HardwareBuffer@Ogre@@QEBA_KXZ
0001f858 488b8c24a0000000           mov       rcx, qword ptr [rsp + 0xa0]
0001f860 488b09                     mov       rcx, qword ptr [rcx]
0001f863 48898c24a8000000           mov       qword ptr [rsp + 0xa8], rcx
0001f86b c744242000000000           mov       dword ptr [rsp + 0x20], 0
0001f873 41b901000000               mov       r9d, 1
0001f879 4c8bc0                     mov       r8, rax
0001f87c 33d2                       xor       edx, edx
0001f87e 488b8c24a0000000           mov       rcx, qword ptr [rsp + 0xa0]
0001f886 488b8424a8000000           mov       rax, qword ptr [rsp + 0xa8]
0001f88e ff5018                     call      qword ptr [rax + 0x18]
0001f891 4889442438                 mov       qword ptr [rsp + 0x38], rax
0001f896 488b8424e8000000           mov       rax, qword ptr [rsp + 0xe8]
0001f89e 488b4008                   mov       rax, qword ptr [rax + 8]
0001f8a2 48d1e0                     shl       rax, 1
0001f8a5 488b8c24e8000000           mov       rcx, qword ptr [rsp + 0xe8]
0001f8ad 488b7c2438                 mov       rdi, qword ptr [rsp + 0x38]
0001f8b2 488b7118                   mov       rsi, qword ptr [rcx + 0x18]
0001f8b6 488bc8                     mov       rcx, rax
0001f8b9 f3a4                       rep movsb byte ptr [rdi], byte ptr [rsi]
0001f8bb 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f8c3 488b8c24e8000000           mov       rcx, qword ptr [rsp + 0xe8]
0001f8cb 488b4908                   mov       rcx, qword ptr [rcx + 8]
0001f8cf 48898890000000             mov       qword ptr [rax + 0x90], rcx
0001f8d6 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f8de 4883c078                   add       rax, 0x78
0001f8e2 488bc8                     mov       rcx, rax
0001f8e5 ff15cd5b0100               call      qword ptr [rip + 0x15bcd] ; ??C?$SharedPtr@VHardwareIndexBuffer@Ogre@@@Ogre@@QEBAPEAVHardwareIndexBuffer@1@XZ
0001f8eb 48898424b0000000           mov       qword ptr [rsp + 0xb0], rax
0001f8f3 488b8424b0000000           mov       rax, qword ptr [rsp + 0xb0]
0001f8fb 488b00                     mov       rax, qword ptr [rax]
0001f8fe 488b8c24b0000000           mov       rcx, qword ptr [rsp + 0xb0]
0001f906 ff5020                     call      qword ptr [rax + 0x20]
0001f909 488b8424e0000000           mov       rax, qword ptr [rsp + 0xe0]
0001f911 c6801401000000             mov       byte ptr [rax + 0x114], 0
0001f918 488d4c2448                 lea       rcx, [rsp + 0x48]
0001f91d ff15855b0100               call      qword ptr [rip + 0x15b85] ; ??1HardwareVertexBufferSharedPtr@Ogre@@QEAA@XZ
0001f923 4881c4c8000000             add       rsp, 0xc8
0001f92a 5f                         pop       rdi
0001f92b 5e                         pop       rsi
0001f92c c3                         ret
0001f92d cc                         int3
0001f92e cc                         int3
0001f92f cc                         int3

#ifndef SYSCALL0
#define SYSCALL0(name)
#endif

#ifndef SYSCALL1
#define SYSCALL1(name, type1)
#endif

#ifndef SYSCALL2
#define SYSCALL2(name, type1, type2)
#endif

#ifndef SYSCALL3
#define SYSCALL3(name, type1, type2, type3)
#endif

#ifndef SYSCALL4
#define SYSCALL4(name, type1, type2, type3, type4)
#endif

#ifndef SYSCALL5
#define SYSCALL5(name, type1, type2, type3, type4, type5)
#endif

SYSCALL0(hello)

#undef SYSCALL0
#undef SYSCALL1
#undef SYSCALL2
#undef SYSCALL3
#undef SYSCALL4
#undef SYSCALL5

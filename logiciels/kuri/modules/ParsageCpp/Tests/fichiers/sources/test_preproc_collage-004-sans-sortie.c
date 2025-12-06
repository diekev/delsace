#define MAKE64(a, b) a##b

#if MAKE64(6, 4) != 64
#error "64 devrait être égal à 64"
#endif
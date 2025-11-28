#define BITS_PER_LONG 64
#define uintBPL_t uint(BITS_PER_LONG)
#define uint(x) xuint(x)
#define xuint(x) __le ## x
uintBPL_t *p = ... ;
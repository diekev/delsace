#define COMMAND(NAME)  { #NAME, NAME ## _command }

struct command commands[] =
{
    COMMAND (quit),
    COMMAND (help),
};

// #include <stdio.h>

#define combine(a, b, c, d, e, f, g) c ## d ## e ## f
#define start combine(m, a, i, n, p, r, o)

int start()
{
    printf("Did you figure it out?\n");
    return 0;
}

// ----------------

#define __gcc_header(x) #x
#define _gcc_header(x) __gcc_header(linux/compiler-gcc##x.h)
#define gcc_header(x) _gcc_header(x)

#define __GNUC__ 0

// #include gcc_header(__GNUC__)
const char *path = gcc_header(__GNUC__);

// --------------------

#define BITS_PER_LONG 64
#define uintBPL_t uint(BITS_PER_LONG)
#define uint(x) xuint(x)
#define xuint(x) __le ## x
uintBPL_t *p = ... ;

// --------------------

#define MAKE64(a, b) a##b

#if MAKE64(6, 4) != 64
#error "64 devrait être égal à 64"
#endif

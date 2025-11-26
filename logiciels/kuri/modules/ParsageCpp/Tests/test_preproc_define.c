#define PARAMETRES(a, b, c) a + b + c

#if PARAMETRES(0, 0, 1)
#  define OK
#else
#  error "Error"
#endif

#define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))

#define __GLIBC_USE(F)	__GLIBC_USE_ ## F

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

min (min (a, b), c)
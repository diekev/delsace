// point d'entrée pour les programmes Kuri
// Nous avons besoin de ceci afin de pouvoir initialiser la bibliothèque
// C, et lié proprement avec le C runtime. Autrememt nous ne pourrons
// pas utiliser des fonctions comme malloc.

#include "r16_tables.h"

typedef float  r32;
typedef double r64;

typedef unsigned short n16;
typedef unsigned int   n32;

typedef short z16;
typedef int   z32;

/* ************* conversions ************* */

#ifdef __cplusplus
extern "C" {
#endif
// À FAIRE : il serait bien de bouger ces fonctions dans r16.kuri
r32 DLS_vers_r32(n16 h)
{
	union { n32 i; float f; } u;
	u.i = table_r16_r32[h];
	return u.f;
}

r64 DLS_vers_r64(n16 a)
{
	return (r64)(DLS_vers_r32(a));
}

r32 DLS_fabs(r32 f)
{
	return (f < 0.0f) ? -f : f;
}

n16 DLS_depuis_r32_except(n32 i)
{
	n32 s = ((i>>16) & 0x8000);
	z32 e = ((i>>13) & 0x3fc00) - 0x1c000;

	if (e <= 0) {
		// denormalized
		union { n32 i; r32 f; } u;
		u.i = i;
		return (n16)(s | (int)(DLS_fabs(u.f) * 1.6777216e7f + 0.5f));
	}

	if (e == 0x23c00) {
		// inf/nan, preserve msb bits of m for nan code
		return (n16)(s|0x7c00|((i&0x7fffff)>>13));
	}

	// overflow - convert to inf
	return (n16)(s | 0x7c00);
}

n16 DLS_depuis_r32(r32 val)
{
	if (val == 0.0f) {
		return 0;
	}

	union { n32 i; float f; } u;
	u.f = val;

	int e = table_r32_r16[(u.i >> 23) & 0x1ff];

	if (e) {
		return (n16)(e + (((u.i & 0x7fffff) + 0x1000) >> 13));
	}

	return DLS_depuis_r32_except(u.i);
}

n16 DLS_depuis_r64(r64 v)
{
	return DLS_depuis_r32((float)(v));
}

int __point_d_entree_systeme(int argc, char **argv);
#ifdef __cplusplus
}
#endif

int main(int argc, char **argv)
{
  return __point_d_entree_systeme(argc, argv);
}

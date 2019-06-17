#include <cstdio>
#include <cstdlib>

/**
 * Les logeurs doivent :
 * - connaître la taille lors du logement, du relogement, et du délogement.
 * - pouvoir se composer entre eux. 
 */

template <typename T>
concept bool ConceptTypeLogeur = requires(T l, void *ptr, size_t taille)
{
	{ T::alignement } -> unsigned;
	{ l.bonne_taille(taille) } -> size_t;
	{ l.loge(taille) } -> void*;
	{ l.loge_tout(taille) } -> void*;
	{ l.deloge(ptr, taille) } -> void;
	{ l.deloge_tout(ptr, taille) } -> void;
	{ l.reloge(ptr, taille, taille) } -> void *;
	{ l.possede(ptr) } -> bool;
	{ l.etend(ptr, taille) } -> bool;
};

/**
 * Crée une addresse en mémoire pour un nombre d'élément du type paramétré du gabarit.
 */
template <typename T>
T *loge(size_t taille = 1)
{
	auto ptr = malloc(sizeof(T) * taille + sizeof(size_t));

	auto ptr0 = static_cast<size_t *>(ptr);
	*ptr0 = sizeof(T) * taille;

	return reinterpret_cast<T *>(ptr0 + 1);
}

/**
 * Donne une nouvelle adresse en mémoire pour le pointeur spécifié.
 * La vieille_taille doit correspondre à la taille spécifiée en paramètre de 
 * l'appel à 'loge' ayant créé l'adresse du pointeur.
 */
template <typename T>
T *reloge(T *ptr, size_t vieille_taille, size_t nouvelle_taille)
{
	/* vérifie la vieille taille */
	auto tete = reinterpret_cast<size_t *>(ptr) - 1;

	if (*tete != (sizeof(T) * vieille_taille)) {
		fprintf(stderr, "reloge : la vieille taille ne correspond pas !");
		abort();
	}

	/* performe le relogement */
	auto ptr0 = realloc(tete, nouvelle_taille + sizeof(size_t));

	tete = static_cast<size_t *>(ptr0);
	*tete = sizeof(T) * nouvelle_taille;

	return reinterpret_cast<T *>(tete + 1);
}

/**
 * Libère la mémoire utilisée par le pointeur passé en paramètre en paramètre.
 * La taille spécifiée doit être la taille spécifiée dans un appel à 'loge' ou à
 * 'reloge' en fonction d'où vient le pointeur.
 *
 * Le pointeur passé est remis à zéro.
 */
template <typename T>
void deloge(T **ptr, size_t taille = 1)
{
	auto ptr0 = reinterpret_cast<size_t *>(*ptr) - 1;

	if (*ptr0 != (sizeof(T) * taille)) {
		fprintf(stderr, "La taille de déallocation ne correspond pas avec la taille d'allocation !");
		abort();
	}

	free(ptr0);
	*ptr = nullptr;
}

int main()
{
	auto x = loge<int>(100);

	for (int i = 0; i < 100; ++i) {
		x[i] = i * i;
	}

	for (int i = 0; i < 100; ++i) {
		printf("%d : %d\n", i, x[i]);
	}

	x = reloge(x, 100, 200);

	for (int i = 0; i < 200; ++i) {
		printf("%d : %d\n", i, x[i]);
	}

	deloge(&x, 200);

	return 0;
}

// https://dlang.org/phobos/std_range_primitives.html


#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename R>
concept bool ConceptRangeeEntree = requires(R r)
{
	typename R::type_valeur;
	{ r.vide() } -> bool;
	{ r.effronte() } -> void;
	{ r.front() } -> typename R::type_valeur;
};

template <typename R>
concept bool ConceptRangeeSortie = requires(R r, typename R::type_valeur e)
{
	typename R::type_valeur;
	{ r.pousse(e) } -> void;
};

template <typename R>
concept bool ConceptRangeeAvant = ConceptRangeeEntree<R> && requires(R r)
{
	{ r.sauvegarde() } -> R;
};

template <typename R>
concept bool ConceptRangeeBidirectionelle = ConceptRangeeAvant<R> && requires(R r)
{
	{ r.cul() } -> typename R::type_valeur;
	{ r.ecule() } -> void;
};

template <typename R>
concept bool ConceptRangeeAleatoire = ConceptRangeeBidirectionelle<R> && requires(R r, unsigned long i)
{
	{ r[i] } -> typename R::type_valeur;
};
#else
#	define ConceptRangeeEntree typename
#	define ConceptRangeeSortie typename
#	define ConceptRangeeAvant typename
#	define ConceptRangeeBidirectionelle typename
#	define ConceptRangeeAleatoire typename
#endif

template <ConceptRangeeEntree RE, ConceptRangeeSortie RS>
static auto copie(RE entree, RS &sortie) -> RS
{
	while (!entree.vide()) {
		sortie.pousse(entree.front());
		entree.effronte();
	}

	return sortie;
}

template <ConceptRangeeAvant RE, ConceptRangeeSortie RS>
static auto copie_carre(RE entree, RS &sortie) -> RS
{
	while (!entree.vide()) {
		auto sauvegarde = entree.sauvegarde();

		while (!sauvegarde.vide()) {
			sortie.pousse(sauvegarde.front());
			sauvegarde.effronte();
		}

		entree.effronte();
	}

	return sortie;
}

template <ConceptRangeeBidirectionelle RE, ConceptRangeeSortie RS>
static auto inverse(RE entree, RS &sortie) -> RS
{
	while (!entree.vide()) {
		sortie.pousse(entree.cul());
		entree.ecule();
	}

	return sortie;
}

template <ConceptRangeeAvant RE>
static auto trouve(RE entree, typename RE::type_valeur valeur) -> RE
{
	while (!entree.vide()) {
		if (entree.front() == valeur) {
			break;
		}

		entree.effronte();
	}

	return entree;
}

template <ConceptRangeeAvant RE, typename Op>
static auto trouve_si(RE entree, Op &&op) -> RE
{
	while (!entree.vide()) {
		if (op(entree.front())) {
			break;
		}

		entree.effronte();
	}

	return entree;
}

template <ConceptRangeeAvant RE, typename Op>
static auto trouve_si_pas(RE entree, Op &&op) -> RE
{
	while (!entree.vide()) {
		if (!op(entree.front())) {
			break;
		}

		entree.effronte();
	}

	return entree;
}

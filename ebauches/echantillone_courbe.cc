auto echantillone_cheveux(Polygone *poly, ListePoints *points, float longueur)
{
	if (longueur == 0.0f) {
		return points->point(poly->index_point(0));
	}

	if (longueur == 1.0f) {
		return points->point(poly->index_point(poly->nombre_sommets() - 1));
	}

	/* une simple interpolation linéaire suffit */
	if (poly->nombre_sommets == 2) {
		auto p0 = points->point(poly->index_point(0));
		auto p1 = points->point(poly->index_point(1));

		return p0 + (p1 - p0) * longueur;
	}

	if (poly->nombre_sommets() == 3) {
		auto p0 = points->point(poly->index_point(0));
		auto p1 = points->point(poly->index_point(1));
		auto p2 = points->point(poly->index_point(2));

		auto p0p1 = p0 + (p1 - p0) * longueur;
		auto p1p2 = p1 + (p2 - p1) * longueur;

		return p0p1 + (p1p2 - p0p1) * longueur;
	}

	auto p = points->point(poly->index_point(0));

	/* Interpolation quadratique */
	auto xt = (1.0f - longueur);
	auto yt = longueur;

	float poids[][] = {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f, 0.0f },
		{ 1.0f, 2.0f, 1.0f, 0.0f },
		{ 1.0f, 3.0f, 3.0f, 1.0f },
	}

	auto n = poly->nombre_sommets();

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto p0 = points->point(poly->index_point(0));

		p += poids[n][i] * std::pow(xt, static_cast<float>(n - i - 1)) * std::pow(t, i) * p0;
	}

	return points->point(poly->index_point(0));
}

poids = [1.Of, 2.0f, 1.0f]
puiss =

1
11
121
1331
14641

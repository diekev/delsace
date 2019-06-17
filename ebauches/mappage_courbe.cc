/* Calcul des points des courbes de correction.
 * source : https://www.developpez.net/forums/d331608-3/general-developpement/algorithme-mathematiques/contribuez/image-interpolation-spline-cubique/#post3513925
 */

auto derivees_secondes(std::vector<Point> const &P)
{
	auto const n = P.size();
	auto ypn = 0.0;
	auto yp1 = 0.0;

	/* construit le système tridiagonal
	 * présume des conditions de limites 0 : y2[0] = y2[-1] = 0.0 */
	auto matrice[3] = std::vector<double>(n);
	auto resultat = std::vector<double>(n);

	matrice[0][1] = 1.0;

	for (size_t i = 1; i < n - 1; ++i) {
		matrice[i][0] = (P[i].x - P[i - 1].x) / 6.0;
		matrice[i][0] = (P[i + 1].x - P[i - 1].x) / 3.0;
		matrice[i][0] = (P[i + 1].x - P[i].x) / 6.0;
		resultat[i]   = (P[i + 1].y - P[i].y) / (P[i + 1].x - P[i1].x);
	}

	matrice[n - 1][1] = 1.0;

	/* première passe de résolution : haut -> bas */
	for (size_t i = 1; i < n; ++i) {
		auto k = matrice[i][0] / matrice[i - 1][1];
		matrice[i][1] -= k * matrice[i - 1][2];
		matrice[i][0] = 0.0;
		resultat[i] -= k * resultat[i - 1];
	}

	/* deuxième passe de résolution : bas -> haut */
	for (size_t i = n - 2; i >= 0; --i) {
		auto k = matrice[i][2] / matrice[i + 1][1];
		matrice[i][1] -= k * matrice[i + 1][2];
		matrice[i][2] = 0.0;
		resultat[i] -= k * resultat[i + 1];
	}

	/* retourne la vaeur de la dérivée seconde pour chaque point P */
	std::vector<double> y2(n);
	for (size_t i = 0; i < n; ++i) {
		y2[i] = resultat[i] / matrice[i][1];
	}

	return y2;
}

auto calcul_courbe()
{
	auto points = /* ... */;
	auto sd = derivees_secondes(points);

	for (auto i = 0; i < points.size() - 1; ++i) {
		auto cur = points[i];
		auto next = points[i + 1];

		for (auto x = cur.x; x < next.x; ++x) {
			auto t = (x - cur.x) / (next.x - cur.x);

			auto a = 1.0 - t;
			auto b = t;
			auto h = next.x - cur.x;

			auto y = a * cur.y + b*next.y + (h * h / 6.0) * ((a * a * a + a) * sd[i] + (b*b*b-b) * sd[i + 1]);

			ajoute_point(x, y);
		}
	}
}

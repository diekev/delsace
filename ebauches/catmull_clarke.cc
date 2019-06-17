struct Vec3 {
	float x, y, z;
};

struct Sommet {
	Vec3 pos;
};

struct Arrete {
	Sommet *s[2];
};

struct Polygone {
	Sommet *s[4];
	Arrete *a[4];
};

struct Maillage {
	std::vector<Sommet *> sommets;
	std::vector<Arrete *> arretes;
	std::vector<Polygone *> polygones;

	~Maillage()
	{
		for (auto &s : sommets) {
			delete s;
		}
		for (auto &s : arretes) {
			delete s;
		}
		for (auto &s : polygones) {
			delete s;
		}
	}

	Sommet *ajoute_sommet(const Vec3 &pos)
	{
		return ajoute_sommet(pos.x, pos.y, pos.z);
	}

	Sommet *ajoute_sommet(float x, float y, float z)
	{
		Sommet *s = new Sommet();
		s->pos.x = x;
		s->pos.y = y;
		s->pos.z = z;
		sommets.push_back(s);

		return s;
	}
};

void subdivise(Maillage *maillage)
{
	// Start with a mesh of an arbitrary polyhedron. All the vertices
	// in this mesh shall be called original points.

	// For each face, add a face point
	// Set each face point to be the average of all original points for
	// the respective face.
	std::map<Polygone *, Sommet *> sommets_polys;
	std::map<Sommet *, std::vector<Polygone *>> tableau_sp;

	for (const auto *p : maillage->polygones) {
		const auto &pos = centroide(p);
		auto s = maillage->ajoute_sommet(pos);

		[p] = s;

		tableau_sp[p->s[0]].push_back(p);
		tableau_sp[p->s[1]].push_back(p);
		tableau_sp[p->s[2]].push_back(p);
		tableau_sp[p->s[3]].push_back(p);
	}

	// For each edge, add an edge point.
	// Set each edge point to be the average of the two neighbouring
	// face points and its two original endpoints.
	std::map<Arrete *, Sommet *> sommets_arretes;
	std::map<Sommet *, std::vector<Arrete *>> tableau_sa;

	for (const auto *a : maillage->arretes) {
		const auto &sp0 = sommets_polys[a->p[0]];
		const auto &sp1 = sommets_polys[a->p[1]];
		const auto &pos = centroide(a->s[0].pos, a->s[0].pos, sp0.pos, sp1.pos);
		auto s = maillage->ajoute_sommet(pos);

		sommets_arretes[a] = s;
		tableau_sa[a->s[0]].push_back(a);
		tableau_sa[a->s[1]].push_back(a);
	}

	// For each face point, add an edge for every edge of the face,
	// connecting the face point to each edge point for the face.
	auto nouveau_maillage = new Maillage;

	for (const auto *p : maillage->polygones) {
		Sommet *s0, *s1, *s2, *s3;

		Sommet *sp = sommets_polys[p];
		Sommet *sa0 = sommets_arretes[p->a[0]];
		Sommet *sa1 = sommets_arretes[p->a[1]];
		Sommet *sa2 = sommets_arretes[p->a[2]];
		Sommet *sa3 = sommets_arretes[p->a[3]];

		// crée un nouveau polygone
		s0 = p->s[0];
		s1 = sa0;
		s2 = sp;
		s3 = sa3;
		nouveau_maillage->ajoute_polygone(s0, s1, s2, s3);

		// crée un nouveau polygone
		s0 = sa0;
		s1 = p->s[1];
		s2 = sa1;
		s3 = sp;
		nouveau_maillage->ajoute_polygone(s0, s1, s2, s3);

		// crée un nouveau polygone
		s0 = sp;
		s1 = sa1;
		s2 = p->s[2];
		s3 = sa2;
		nouveau_maillage->ajoute_polygone(s0, s1, s2, s3);

		// crée un nouveau polygone
		s0 = sa3;
		s1 = sp;
		s2 = sa2;
		s3 = p->s[3];
		nouveau_maillage->ajoute_polygone(s0, s1, s2, s3);
	}

	// For each original point P, take the average F of all n (recently
	// created) face points for faces touching P, and take the average R
	// of all n edge midpoints for (original) edges touching P, where
	// each edge midpoint is the average of its two endpoint vertices
	// (not to be confused with new "edge points" above). Move each
	// original point to the point (F + 2R + (n-3)P)/n.
	// This is the barycenter of P, R and F with respective weights
	// (n − 3), 2 and 1. Connect each new vertex point to the new edge
	// points of all original edges incident on the original vertex.
	// Define new faces as enclosed by edges.

	for (auto &sommet : maillage->sommets) {
		// trouve tous les polygones où le sommet apparaît
		auto F = Vec3(0.0);
		auto vec_poly = tableau_sp[sommet];
		auto n = vec_poly.size();

		for (const auto &polygone : vec_poly) {
			F += sommets_polys[polygone]->pos;
		}

		F /= n;

		// trouve toutes les arrêtes où le sommet
		auto R = Vec3(0.0);
		auto vec_arrete = tableau_sa[sommet];
		auto na = vec_arrete.size();

		for (const auto &arrete : vec_arrete) {
			auto mid = (arrete->s[0] + arrete->s[1]) * 0.5;
			R += mid;
		}

		R /= na;

		sommet->pos = (F + 2.0 * R + (n - 3) * sommet->pos) / n;
	}
}

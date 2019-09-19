struct AccesseusePoints {
	ListePoints3D &liste_pnt;

	explicit AccesseusePoints(ListePoints3D &lp)
		: liste_pnt(lp)
	{}

	long taille() const
	{
		return liste_pnt.taille();
	}

	dls::math::vec3f &point(long i)
	{
		return liste_pnt[i];
	}

	dls::math::vec3f const &point(long i) const
	{
		return liste_pnt[i];
	}

	dls::math::vec3f point_transforme(long i) const
	{
		auto p = m_points.point(i);
		auto pos_monde_d = this->transformation(dls::math::point3d(p));
		return dls::math::converti_type_vecteur<float>(pos_monde_d);
	}

	long ajoute_point(dls::math::vec3f const &pos)
	{
		return ajoute_point(pos.x, pos.y, pos.z);
	}

	long ajoute_point(float x, float y, float z)
	{
		auto index = index_point(x, y, z);

		if (index != -1l) {
			return index;
		}

		auto point = dls::math::vec3f(x, y, z);
		m_points.pousse(point);

		redimensionne_attributs(portee_attr::POINT);

		return m_points.taille() - 1;
	}

	long index_point(float x, float y, float z)
	{
		INUTILISE(x);
		INUTILISE(y);
		INUTILISE(z);
	//	int i = 0;

	//	for (auto const &point : m_points.points()) {
	//		if (point.x == x && point.y == y && point.z == z) {
	//			return i;
	//		}

	//		++i;
	//	};

		return -1l;
	}
};

struct AccesseusePrims {
	explicit AccesseusePrims(ListePrimitive &lp)
		: liste_prims(lp)
	{}

	Primitive *prim(long i)
	{
		return liste_prims[i];
	}

	Primitive const *prim(long i) const
	{
		return liste_prims[i];
	}

	long taille() const
	{
		return liste_prims.taille();
	}

	Polygone *ajoute_polygone(long nombre_sommet)
	{
		auto p = memoire::loge<Polygone>("Polygone");
		p->type = type_polygone::FERME;
		p->reserve_sommets(nombre_sommets);

		ajoute_primitive(p);

		redimensionne_attributs(portee_attr::PRIMITIVE);

		return p;
	}

	Polygone *ajoute_polyligne(long nombre_sommet)
	{
		auto p = memoire::loge<Polygone>("Polygone");
		p->type = type_polygone::OUVERT;
		p->reserve_sommets(nombre_sommets);

		ajoute_primitive(p);

		redimensionne_attributs(portee_attr::PRIMITIVE);

		return p;
	}

	long ajoute_sommet(Polygone *poly, long idx_point)
	{
		auto idx_sommet = m_nombre_sommets++;

		poly->ajoute_point(idx_point, idx_sommet);

		redimensionne_attributs(portee_attr::VERTEX);

		return idx_sommet;
	}

	Sphere *ajoute_sphere(long idx_point, float rayon)
	{
		auto sphere = memoire::loge<Sphere>("Sphère", idx_point, rayon);
		ajoute_primitive(sphere);
		return sphere;
	}

	Volume *ajoute_volume()
	{
	}

private:
	ListePrimitive &liste_prims;

	void ajoute_primitive(Primitive *prim)
	{
		prim->index = this->taille();
		liste_prims.pousse(p);
	}
};

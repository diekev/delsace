int calcul_nombre_udim(float u, float v)
{
	return 1000 + (static_cast<int>(u) + 1) + (10 * static_cast<int>(v));
}

bool udim_valid(int udim)
{
	return udim < 1001;
}

/* https://www.youtube.com/watch?v=F-sQ6JFbbyA */
class OpVerifieUVs final : public OperatriceCorps {
public:
	int execute()
	{
		/* Vérifications
		 * - À FAIRE : UVs superposés
		 * - À FAIRE : UVs flipped
		 * - À FAIRE : UVs absent
		 * - À FAIRE : ratio UV différent entre la surface dans le monde 3d et la surface des patchs
		 * - À FAIRE : UV shells cross UDIM borders
		 * - À FAIRE : UVs negatifs résultant en des nombre UDIMs invalides
		 * - À FAIRE : faces avec des surface nulles ou inférieures à 1px
		 * - À FAIRE : normaux avec des longueurs == 0.0
		 * - À FAIRE : auto-intersection
		 */

		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte);

		auto attr_UV = m_corps.attribut("UV");

		if (attr_UV == nullptr) {
			this->ajoute_avertissement("Aucun UV trouvé");
			return EXECUTION_ECHOUEE;
		}

		auto prims = m_corps->prims();

		/* trois attributs couleur
		 * - attr_1:
		 * -- R : UVs flipped
		 * -- V : UVs superposés
		 * -- B : UDIM débordé ou invalide
		 * - attr_2:
		 * -- R : Aire texture == 0.0
		 * -- V : Aire texture < 1px
		 * -- B : Longueur normaux == 0.0
		 * - attr_3
		 * -- R : ratio UV sup
		 * -- V : ratio UV moins
		 * -- B : noUVs
		 */
		auto attr_1 = m_corps.ajoute_attribut("attr_valid_uv1", type_attribut::VEC3, portee_attr::PRIMITIVE);
		auto attr_2 = m_corps.ajoute_attribut("attr_valid_uv2", type_attribut::VEC3, portee_attr::PRIMITIVE);
		auto attr_3 = m_corps.ajoute_attribut("attr_valid_uv3", type_attribut::VEC3, portee_attr::PRIMITIVE);

		for (auto i = 0; i < prims->taille(); ++i) {
			auto poly = dynamic_cast<Polygone *>(prims->prim(i);
			auto c1 = dls::math::vec3f(0.0f, 0.0f, 0.0f);
			auto c2 = dls::math::vec3f(0.0f, 0.0f, 0.0f);
			auto c3 = dls::math::vec3f(0.0f, 0.0f, 0.0f);

			auto idx_udim = 0;

			for (auto j = 0; j < poly->nombre_sommets(); ++j) {

				auto uv = attr_UV->vec2(poly->index_sommet(j));

				if (uv.x == 0.0f && uv.y == 0.0f) {
					c3.b = 1.0f;
				}

				if (uv.x < 0.0f || uv.y < 0.0f) {
					c1.b = 1.0f;
				}

				if (j == 0) {
					idx_udim = calcul_nombre_udim(uv.x, uv.y);
				}
				else {
					/* débordement UDIM */
					if (idx_udim != calcul_nombre_udim(uv.x, uv.y)) {
						c1.b = 1.0f;
					}
				}
			}
		}

		attr_1->vec3(i, c1);
		attr_2->vec3(i, c2);
		attr_3->vec3(i, c3);
	}
};

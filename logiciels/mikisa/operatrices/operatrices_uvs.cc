/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_uvs.hh"

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/file.hh"

#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/iteration_corps.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

struct mi_arete_uv;
struct mi_face_uv;

struct mi_point_uv {
	dls::math::vec2f pos{};
	long index{};

	mi_point_uv() = default;
};

struct mi_sommet_uv {
	mi_point_uv *p = nullptr;

	/* la mi-arête de ce sommet */
	mi_arete_uv *arete = nullptr;

	bool est_sur_couture = false;

	mi_sommet_uv() = default;

	COPIE_CONSTRUCT(mi_sommet_uv);
};

struct mi_arete_uv {
	mi_sommet_uv *smt = nullptr;
	mi_arete_uv *suivante = nullptr;
	mi_arete_uv *paire = nullptr;
	mi_face_uv *face = nullptr;

	mi_arete_uv() = default;

	COPIE_CONSTRUCT(mi_arete_uv);
};

struct mi_face_uv {
	/* une des arête de la face */
	mi_arete_uv *arete = nullptr;

	/* label pour les algorithmes, par exemple pour stocker un index d'origine */
	unsigned int label0 = 0;
	unsigned int label1 = 0;

	mi_face_uv() = default;

	COPIE_CONSTRUCT(mi_face_uv);
};

struct PolyedreUV {
	dls::tableau<mi_point_uv *> points{};
	dls::tableau<mi_sommet_uv *> sommets{};
	dls::tableau<mi_arete_uv *> aretes{};
	dls::tableau<mi_face_uv *> faces{};

	~PolyedreUV()
	{
		for (auto pnt : points) {
			memoire::deloge("mi_point_uv", pnt);
		}

		for (auto smt : sommets) {
			memoire::deloge("mi_sommet_uv", smt);
		}

		for (auto art : aretes) {
			memoire::deloge("mi_arete_uv", art);
		}

		for (auto fac : faces) {
			memoire::deloge("mi_face_uv", fac);
		}
	}

	mi_point_uv *cree_point(dls::math::vec2f const &pos)
	{
		for (auto pnt : points) {
			if (pnt->pos == pos) {
				return pnt;
			}
		}

		auto pnt = memoire::loge<mi_point_uv>("mi_point_uv");
		pnt->pos = pos;
		pnt->index = points.taille();
		points.pousse(pnt);
		return pnt;
	}

	mi_sommet_uv *cree_sommet(mi_point_uv *pnt)
	{
		auto smt = memoire::loge<mi_sommet_uv>("mi_sommet_uv");
		smt->p = pnt;
		sommets.pousse(smt);
		return smt;
	}

	mi_arete_uv *cree_arete(mi_sommet_uv *smt, mi_face_uv *face)
	{
		auto art = memoire::loge<mi_arete_uv>("mi_arete_uv");
		art->smt = smt;
		art->face = face;
		aretes.pousse(art);
		return art;
	}

	mi_face_uv *cree_face()
	{
		auto fac = memoire::loge<mi_face_uv>("mi_arete_uv");
		faces.pousse(fac);
		return fac;
	}
};

inline auto index_arete(long i0, long i1)
{
	return static_cast<size_t>(i0 | (i1 << 32));
}

/**
 * Construit un polyèdre depuis un attribut UV. Le polyèdre possède la même
 * topologie que le maillage : deux faces UVs seront connectées si leurs
 * polygones d'origine le sont aussi et ce même si une couture devrait exister
 * entre entre elles.
 * Ils faudra un algorithme distinct pour séparer les faces en iles.
 */
static auto construit_polyedre_uv(Corps const &corps)
{
	auto polyedre = PolyedreUV();
	auto attr_UV = corps.attribut("UV");

	auto dico_aretes = dls::dico_desordonne<size_t, mi_arete_uv *>();

	pour_chaque_polygone_ferme(corps, [&](Corps const &, Polygone *poly)
	{
		auto f = polyedre.cree_face();
		f->label0 = static_cast<unsigned>(poly->index);
		f->label1 = 0;

		auto a = static_cast<mi_arete_uv *>(nullptr);

		for (auto i = 0; i < poly->nombre_sommets(); ++i) {
			auto idx = poly->index_sommet(i);

			auto uv = dls::math::vec2f();
			extrait(attr_UV->r32(idx), uv);

			auto pnt = polyedre.cree_point(uv);
			auto smt = polyedre.cree_sommet(pnt);

			auto a0 = polyedre.cree_arete(smt, f);

			if (f->arete == nullptr) {
				f->arete = a0;
			}

			if (a != nullptr) {
				a->suivante = a0;
			}

			a = a0;

			auto idx0 = poly->index_point(i);
			auto idx1 = poly->index_point((i + 1) % poly->nombre_sommets());
			auto idxi0i1 = index_arete(idx0, idx1);

			dico_aretes.insere({idxi0i1, a0});

			/* cherches la mi_arete opposée */
			auto idxi1i0 = index_arete(idx1, idx0);

			auto iter = dico_aretes.trouve(idxi1i0);

			if (iter != dico_aretes.fin()) {
				auto a1 = iter->second;
				a0->paire = a1;
				a1->paire = a0;
			}
		}

		/* clos la boucle */
		a->suivante = f->arete;
	});

	return polyedre;
}

static void ajourne_label_groupe(mi_face_uv *face, unsigned int groupe)
{
	auto file = dls::file<mi_face_uv *>();
	auto visites = dls::ensemble<mi_face_uv *>();

	file.enfile(face);

	while (!file.est_vide()) {
		face = file.defile();

		if (visites.trouve(face) != visites.fin()) {
			continue;
		}

		visites.insere(face);

		face->label1 = groupe;

		/* pour chaque face autour de la nôtre */
		auto a0 = face->arete;
		auto a1 = a0->suivante;
		auto fin = a1;

		do {
			if (a0->paire != nullptr) {
				file.enfile(a0->paire->face);
			}

			a0 = a1;
			a1 = a0->suivante;
		} while (a1 != fin);
	}
}

/* À FAIRE : algorithme pour détacher les iles UVs. */
static void detaches_iles_uvs(PolyedreUV &polyedre_uv)
{
	// une couture existe sur arête si leurs sommets n'ont pas le même point

	for (auto face : polyedre_uv.faces) {
		auto a0 = face->arete;
		auto a1 = a0->suivante;
		auto fin = a1;

		// il faut chercher l'arête opposée de la suivante de pour avoir deux
		// arêtes qui pointent vers le même sommet potentiel

		do {
			if (a1->paire != nullptr) {
				auto ap = a1->paire;

				if (ap->smt->p->index != a0->smt->p->index) {
					// cette arete est sur une couture
					a0->smt->est_sur_couture = true;
					ap->smt->est_sur_couture = true;
				}
			}

			a0 = a1;
			a1 = a0->suivante;
		} while (a1 != fin);
	}

	for (auto face : polyedre_uv.faces) {
		auto a0 = face->arete;
		auto a1 = a0->suivante;
		auto fin = a1;

		do {
			if (a0->smt->est_sur_couture && a1->smt->est_sur_couture) {
				a1->paire = nullptr;
			}

			a0 = a1;
			a1 = a0->suivante;
		} while (a1 != fin);
	}
}

/* ************************************************************************** */

struct OpVisualiseUV : public OperatriceCorps {
	static constexpr auto NOM = "Visualise UVs";
	static constexpr auto AIDE = "";

	OpVisualiseUV(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, true, true)) {
			return res_exec::ECHOUEE;
		}

		auto attr_UV = corps_entree->attribut("UV");

		if (attr_UV == nullptr || attr_UV->portee != portee_attr::VERTEX) {
			this->ajoute_avertissement("Aucun attribut UV sur les sommets trouvé");
			return res_exec::ECHOUEE;
		}

		auto polyedre_uv = construit_polyedre_uv(*corps_entree);

		for (auto point : polyedre_uv.points) {
			m_corps.ajoute_point(point->pos.x, 0.0f, point->pos.y);
		}

		for (auto face : polyedre_uv.faces) {
			auto arete = face->arete;
			auto poly = m_corps.ajoute_polygone(type_polygone::FERME);

			do {
				m_corps.ajoute_sommet(poly, arete->smt->p->index);

				arete = arete->suivante;
			} while (arete != face->arete);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

struct OpGroupeUV : public OperatriceCorps {
	static constexpr auto NOM = "Groupe Prims UVs";
	static constexpr auto AIDE = "Groupe les primitives selon les pièces détachées des UVs";

	OpGroupeUV(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		m_execute_toujours = true;
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto attr_UV = m_corps.attribut("UV");

		if (attr_UV == nullptr || attr_UV->portee != portee_attr::VERTEX) {
			this->ajoute_avertissement("Aucun attribut UV sur les sommets trouvé");
			return res_exec::ECHOUEE;
		}

		auto polyedre_uv = construit_polyedre_uv(m_corps);
		detaches_iles_uvs(polyedre_uv);

		auto nombre_groupe = 0u;
		for (auto face : polyedre_uv.faces) {
			if (face->label1 != 0) {
				continue;
			}

			ajourne_label_groupe(face, ++nombre_groupe);
		}

		auto groupes = dls::tableau<GroupePrimitive *>(nombre_groupe);

		for (auto i = 0u; i < nombre_groupe; ++i) {
			groupes[i] = m_corps.ajoute_groupe_primitive("pièce" + dls::vers_chaine(i));
		}

		for (auto face : polyedre_uv.faces) {
			groupes[face->label1 - 1]->ajoute_primitive(face->label0);
		}

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::PRIMITIVE);

		auto gna = GNA();

		for (auto i = 0u; i < nombre_groupe; ++i) {
			auto hsv = dls::math::vec3f(
						gna.uniforme(0.0f, 1.0f),
						1.0f,
						0.5f);

			auto couleur = dls::math::vec3f();
			dls::phys::hsv_vers_rvb(hsv.x, hsv.y, hsv.z, &couleur.x, &couleur.y, &couleur.z);

			auto groupe = groupes[i];

			for (auto j = 0; j < groupe->taille(); ++j) {
				auto idx = groupe->index(j);
				assigne(attr_C->r32(idx), couleur);
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_uvs(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpVisualiseUV>());
	usine.enregistre_type(cree_desc<OpGroupeUV>());
}

#pragma clang diagnostic pop

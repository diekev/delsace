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

#include "operatrice_graphe_detail.hh"

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/outils/fichier.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "corps/volume.hh"

#include "lcc/arbre_syntactic.h"
#include "lcc/code_inst.hh"
#include "lcc/lcc.hh"

#include "wolika/iteration.hh"

#include "composite.h"
#include "contexte_evaluation.hh"
#include "donnees_aval.hh"
#include "objet.h"
#include "mikisa.h"
#include "usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

struct DonneesCompilationNuanceur {
	dls::flux_chaine os{};
	int decalage_prise = 0;
};

/* ************************************************************************** */

/* les sorties des noeuds d'entrées */
static lcc::param_sorties params_noeuds_entree[] = {
	/* entrée détail point */
	lcc::param_sorties(
		lcc::donnees_parametre("P", lcc::type_var::VEC3)),
	/* entrée détail voxel */
	lcc::param_sorties(
		lcc::donnees_parametre("densité", lcc::type_var::DEC),
		lcc::donnees_parametre("pos_monde", lcc::type_var::VEC3),
		lcc::donnees_parametre("pos_unit", lcc::type_var::VEC3)),
	/* entrée détail pixel */
	lcc::param_sorties(
		lcc::donnees_parametre("couleur", lcc::type_var::COULEUR),
		lcc::donnees_parametre("P", lcc::type_var::VEC3)),
	/* entrée détail terrain */
	lcc::param_sorties(
		lcc::donnees_parametre("P", lcc::type_var::VEC3)),
	/* entrée détail poséidon */
	lcc::param_sorties(
		lcc::donnees_parametre("divergence", lcc::type_var::DEC),
		lcc::donnees_parametre("fioul", lcc::type_var::DEC),
		lcc::donnees_parametre("fumée", lcc::type_var::DEC),
		lcc::donnees_parametre("oxygène", lcc::type_var::DEC),
		lcc::donnees_parametre("température", lcc::type_var::DEC),
		lcc::donnees_parametre("vélocité", lcc::type_var::VEC3),
		lcc::donnees_parametre("pos_monde", lcc::type_var::VEC3),
		lcc::donnees_parametre("pos_unit", lcc::type_var::VEC3)),
	/* entrée détail nuançage */
	lcc::param_sorties(
		lcc::donnees_parametre("P", lcc::type_var::VEC3)),
};

/* les entrées des noeuds de sorties */
static lcc::param_entrees params_noeuds_sortie[] = {
	/* sortie détail point */
	lcc::param_entrees(
		lcc::donnees_parametre("P", lcc::type_var::VEC3)),
	/* sortie détail voxel */
	lcc::param_entrees(
		lcc::donnees_parametre("densité", lcc::type_var::DEC)),
	/* sortie détail pixel */
	lcc::param_entrees(
		lcc::donnees_parametre("couleur", lcc::type_var::COULEUR)),
	/* sortie détail terrain */
	lcc::param_entrees(
		lcc::donnees_parametre("hauteur", lcc::type_var::DEC)),
	/* sortie détail poséidon */
	lcc::param_entrees(
		lcc::donnees_parametre("divergence", lcc::type_var::DEC),
		lcc::donnees_parametre("fioul", lcc::type_var::DEC),
		lcc::donnees_parametre("fumée", lcc::type_var::DEC),
		lcc::donnees_parametre("oxygène", lcc::type_var::DEC),
		lcc::donnees_parametre("température", lcc::type_var::DEC),
		lcc::donnees_parametre("vélocité", lcc::type_var::VEC3)),
	/* sortie détail nuançage */
	lcc::param_entrees(
		lcc::donnees_parametre("couleur", lcc::type_var::COULEUR)),
};

/* ************************************************************************** */

static auto stocke_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		long idx_attr)
{
	for (auto const &donnee : gest_attrs.donnees) {
		/* ne stocke pas les attributs créés pour ne pas effacer les
		 * données d'assignation de constante à la position du
		 * pointeur */
		if (donnee->est_requis || donnee->est_propriete) {
			continue;
		}

		auto idx_pile = donnee->ptr;
		auto attr = std::any_cast<Attribut *>(donnee->ptr_donnees);

		switch (attr->type()) {
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_pile, ptr_attr, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::INVALIDE:
			case type_attribut::CHAINE:
			{
				break;
			}
		}
	}
}

static auto charge_attributs(
		gestionnaire_propriete const &gest_attrs,
		lcc::pile &donnees,
		long idx_attr)
{
	for (auto const &donnee : gest_attrs.donnees) {
		if (donnee->est_propriete || !donnee->est_modifiee) {
			continue;
		}

		auto idx_pile = donnee->ptr;
		auto attr = std::any_cast<Attribut *>(donnee->ptr_donnees);

		switch (attr->type()) {
			default:
			{
				auto taille = taille_octet_type_attribut(attr->type()) * attr->dimensions;

				auto ptr_attr = static_cast<char *>(attr->donnees()) + idx_attr * taille;
				auto ptr_pile = reinterpret_cast<char *>(donnees.donnees() + idx_pile);

				std::memcpy(ptr_attr, ptr_pile, static_cast<size_t>(taille));
				break;
			}
			case type_attribut::INVALIDE:
			case type_attribut::CHAINE:
			{
				break;
			}
		}
	}
}

/* À FAIRE : déduplique. */
static auto converti_type_lcc(lcc::type_var type, int &dimensions)
{
	switch (type) {
		case lcc::type_var::DEC:
		{
			dimensions = 1;
			return type_attribut::R32;
		}
		case lcc::type_var::ENT32:
		{
			dimensions = 1;
			return type_attribut::Z32;
		}
		case lcc::type_var::VEC2:
		{
			dimensions = 2;
			return type_attribut::R32;
		}
		case lcc::type_var::VEC3:
		{
			dimensions = 3;
			return type_attribut::R32;
		}
		case lcc::type_var::COULEUR:
		case lcc::type_var::VEC4:
		{
			dimensions = 4;
			return type_attribut::R32;
		}
		case lcc::type_var::MAT3:
		{
			dimensions = 9;
			return type_attribut::R32;
		}
		case lcc::type_var::MAT4:
		{
			dimensions = 16;
			return type_attribut::R32;
		}
		case lcc::type_var::CHAINE:
		{
			dimensions = 1;
			return type_attribut::CHAINE;
		}
		case lcc::type_var::INVALIDE:
		case lcc::type_var::TABLEAU:
		case lcc::type_var::POLYMORPHIQUE:
			return type_attribut::INVALIDE;
	}

	return type_attribut::INVALIDE;
}

static auto converti_type_attr(type_attribut type, int dimensions)
{
	switch (type) {
		case type_attribut::R32:
		{
			if (dimensions == 1) {
				return lcc::type_var::DEC;
			}

			if (dimensions == 2) {
				return lcc::type_var::VEC2;
			}

			if (dimensions == 3) {
				return lcc::type_var::VEC3;
			}

			if (dimensions == 4) {
				return lcc::type_var::VEC4;
			}

			if (dimensions == 9) {
				return lcc::type_var::MAT3;
			}

			if (dimensions == 16) {
				return lcc::type_var::MAT4;
			}

			break;
		}
		case type_attribut::Z8:
		case type_attribut::Z32:
		{
			if (dimensions == 1) {
				return lcc::type_var::ENT32;
			}

			break;
		}
		case type_attribut::CHAINE:
		{
			return lcc::type_var::CHAINE;
		}
		default:
		{
			return lcc::type_var::INVALIDE;
		}
	}

	return lcc::type_var::INVALIDE;
}

/* ************************************************************************** */

static auto trouve_noeuds_connectes_sortie(Graphe const &graphe)
{
	auto noeuds = dls::pile<Noeud *>();
	auto ensemble = dls::ensemble<Noeud *>();

	for (auto &noeud_graphe : graphe.noeuds()) {
		if (noeud_graphe->est_sortie) {
			noeuds.empile(noeud_graphe);
		}
	}

	while (!noeuds.est_vide()) {
		auto noeud = noeuds.depile();

		if (ensemble.trouve(noeud) != ensemble.fin()) {
			continue;
		}

		ensemble.insere(noeud);

		for (auto const &entree : noeud->entrees) {
			for (auto const &sortie : entree->liens) {
				noeuds.empile(sortie->parent);
			}
		}
	}

	return ensemble;
}

/* ************************************************************************** */

CompileuseGrapheLCC::CompileuseGrapheLCC(Graphe &ptr_graphe)
	: graphe(ptr_graphe)
{}

lcc::pile &CompileuseGrapheLCC::donnees()
{
	return m_compileuse.donnees();
}

void CompileuseGrapheLCC::stocke_attributs(lcc::pile &donnees, long idx_attr)
{
	::stocke_attributs(m_gest_attrs, donnees, idx_attr);
}

void CompileuseGrapheLCC::charge_attributs(lcc::pile &donnees, long idx_attr)
{
	::charge_attributs(m_gest_attrs, donnees, idx_attr);
}

bool CompileuseGrapheLCC::compile_graphe(ContexteEvaluation const &contexte, Corps *corps)
{
	m_compileuse = compileuse_lng();
	m_gest_attrs = gestionnaire_propriete();

	m_ctx_global.reinitialise();

	if (graphe.besoin_ajournement) {
		tri_topologique(graphe);
		graphe.besoin_ajournement = false;
	}

	auto donnees_aval = DonneesAval{};
	donnees_aval.table.insere({"coulisse", dls::chaine("lcc")});
	donnees_aval.table.insere({"compileuse", &m_compileuse});
	donnees_aval.table.insere({"ctx_global", &m_ctx_global});
	donnees_aval.table.insere({"gest_attrs", &m_gest_attrs});

	auto type_detail = std::any_cast<int>(graphe.donnees[0]);

	/* fais de la place pour les entrées et sorties */
	auto &params_entree = params_noeuds_entree[type_detail];

	for (auto i = 0; i < params_entree.taille(); ++i) {
		auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(params_entree.type(i)));
		m_gest_attrs.ajoute_propriete(params_entree.nom(i), params_entree.type(i), idx);
	}

	auto &params_sortie = params_noeuds_sortie[type_detail];

	for (auto i = 0; i < params_sortie.taille(); ++i) {
		auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(params_sortie.type(i)));
		m_gest_attrs.ajoute_propriete(params_sortie.nom(i), params_sortie.type(i), idx);
	}

	/* fais de la place pour les attributs */
	if (type_detail == DETAIL_POINTS && corps != nullptr) {
		for (auto &attr : corps->attributs()) {
			if (attr.portee != portee_attr::POINT) {
				continue;
			}

			auto idx = m_compileuse.donnees().loge_donnees(attr.dimensions);
			auto type_attr = converti_type_attr(attr.type(), attr.dimensions);
			m_gest_attrs.ajoute_attribut(attr.nom(), type_attr, idx);
		}
	}

	/* trouve les noeuds étant connectés à la sortie */
	auto ensemble = trouve_noeuds_connectes_sortie(graphe);

	/* performe la compilation */
	for (auto &noeud_graphe : graphe.noeuds()) {
		for (auto &sortie : noeud_graphe->sorties) {
			sortie->decalage_pile = 0;
		}

		if (ensemble.trouve(noeud_graphe) == ensemble.fin()) {
			continue;
		}

		auto operatrice = extrait_opimage(noeud_graphe->donnees);
		operatrice->reinitialise_avertisements();

		auto resultat = operatrice->execute(contexte, &donnees_aval);

		if (resultat == res_exec::ECHOUEE) {
			return false;
		}
	}

	/* prise en charge des requêtes */
	if (type_detail == DETAIL_POINTS && corps != nullptr) {
		for (auto &requete : m_gest_attrs.donnees) {
			requete->ptr_donnees = corps->attribut(requete->nom);
		}

		for (auto &requete : m_gest_attrs.requetes) {
			auto attr = corps->attribut(requete->nom);

			if (attr != nullptr) {
				/* L'attribut n'a pas la portée point */
				assert(attr->portee != portee_attr::POINT);

				corps->supprime_attribut(requete->nom);
			}

			auto dimensions = 0;
			auto type_attr = converti_type_lcc(requete->type, dimensions);

			attr = corps->ajoute_attribut(
						requete->nom,
						type_attr,
						dimensions,
						portee_attr::POINT);

			requete->ptr_donnees = attr;
		}
	}

	return true;
}

void CompileuseGrapheLCC::execute_pile(lcc::ctx_local &ctx_local, lcc::pile &donnees_pile)
{
	lcc::execute_pile(
				m_ctx_global,
				ctx_local,
				donnees_pile,
				m_compileuse.instructions(),
				0);
}

int CompileuseGrapheLCC::pointeur_donnees(const dls::chaine &nom)
{
	return m_gest_attrs.pointeur_donnees(nom);
}

/* ************************************************************************** */

OperatriceGrapheDetail::OperatriceGrapheDetail(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceCorps(graphe_parent, noeud_)
	, m_compileuse(noeud_.graphe)
{
	noeud.peut_avoir_graphe = true;
	noeud.graphe.type = type_graphe::DETAIL;
	noeud.graphe.donnees.efface();
	noeud.graphe.donnees.pousse(type_detail);

	entrees(1);
	sorties(1);
}

const char *OperatriceGrapheDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceGrapheDetail::texte_aide() const
{
	return AIDE;
}

const char *OperatriceGrapheDetail::chemin_entreface() const
{
	if (type_detail == DETAIL_PIXELS) {
		return "";
	}

	return "";
}

type_prise OperatriceGrapheDetail::type_entree(int) const
{
	if (type_detail == DETAIL_PIXELS) {
		return type_prise::IMAGE;
	}

	return type_prise::CORPS;
}

type_prise OperatriceGrapheDetail::type_sortie(int) const
{
	if (type_detail == DETAIL_PIXELS) {
		return type_prise::IMAGE;
	}

	return type_prise::CORPS;
}

int OperatriceGrapheDetail::type() const
{
	return OPERATRICE_GRAPHE_DETAIL;
}

res_exec OperatriceGrapheDetail::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (!this->entree(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return res_exec::ECHOUEE;
	}

	noeud.graphe.donnees.efface();
	noeud.graphe.donnees.pousse(type_detail);

	if (type_detail == DETAIL_PIXELS) {
		return execute_detail_pixel(contexte, donnees_aval);
	}

	return execute_detail_corps(contexte, donnees_aval);
}

res_exec OperatriceGrapheDetail::execute_detail_pixel(
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	m_image.reinitialise();

	auto chef = contexte.chef;

	if (!m_compileuse.compile_graphe(contexte, nullptr)) {
		ajoute_avertissement("Ne peut pas compiler le graphe, voir si les noeuds n'ont pas d'erreurs.");
		return res_exec::ECHOUEE;
	}

	calque_image *calque = nullptr;
	auto const &rectangle = contexte.resolution_rendu;

	if (entrees() == 0) {
		m_image.reinitialise();
		auto desc = desc_depuis_rectangle(rectangle);
		calque = m_image.ajoute_calque("image", desc, wlk::type_grille::COULEUR);
	}
	else if (entrees() >= 1) {
		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		auto nom_calque = "image";// evalue_chaine("nom_calque");
		calque = m_image.calque_pour_ecriture(nom_calque);
	}

	if (calque == nullptr) {
		ajoute_avertissement("Calque introuvable !");
		return res_exec::ECHOUEE;
	}

	chef->demarre_evaluation("graphe détail pixel");

	auto desc = calque->tampon()->desc();

	auto tampon = extrait_grille_couleur(calque);
	auto largeur_inverse = 1.0f / static_cast<float>(desc.resolution.x);
	auto hauteur_inverse = 1.0f / static_cast<float>(desc.resolution.y);

	boucle_parallele(tbb::blocked_range<int>(0, desc.resolution.y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef->interrompu()) {
			return;
		}

		/* fais une copie locale pour éviter les problèmes de concurrence critique */
		auto donnees = m_compileuse.donnees();

		for (auto l = plage.begin(); l < plage.end(); ++l) {
			if (chef->interrompu()) {
				return;
			}

			for (auto c = 0; c < desc.resolution.x; ++c) {
				auto ctx_local = lcc::ctx_local{};

				auto const x = static_cast<float>(c) * largeur_inverse;
				auto const y = static_cast<float>(l) * hauteur_inverse;

				auto index = tampon->calcul_index(dls::math::vec2i(c, l));

				auto pos = dls::math::vec3f(x, y, 0.0f);
				m_compileuse.remplis_donnees(donnees, "P", pos);

				auto clr = tampon->valeur(index);
				m_compileuse.remplis_donnees(donnees, "couleur", clr);

				m_compileuse.execute_pile(ctx_local, donnees);

				auto idx_sortie = m_compileuse.pointeur_donnees("couleur");
				clr = donnees.charge_couleur(idx_sortie);

				tampon->valeur(index, clr);
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin());
		delta *= hauteur_inverse;

		chef->indique_progression_parallele(delta * 100.0f);
	});

	chef->indique_progression(100.0f);

	return res_exec::REUSSIE;
}

res_exec OperatriceGrapheDetail::execute_detail_corps(
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	m_corps.reinitialise();
	entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

	auto volume = static_cast<Volume *>(nullptr);

	switch (type_detail) {
		case DETAIL_POINTS:
		{
			if (!valide_corps_entree(*this, &m_corps, true, false)) {
				return res_exec::ECHOUEE;
			}

			break;
		}
		case DETAIL_VOXELS:
		{
			auto idx_volume = -1;

			for (auto i = 0; i < m_corps.prims()->taille(); ++i) {
				auto prim = m_corps.prims()->prim(i);

				if (prim->type_prim() != type_primitive::VOLUME) {
					continue;
				}

				auto prim_vol = dynamic_cast<Volume *>(prim);

				if (prim_vol->grille->desc().type_donnees == wlk::type_grille::R32) {
					idx_volume = i;
					break;
				}
			}

			if (idx_volume == -1) {
				ajoute_avertissement("Aucun volume scalaire en entrée.");
				return res_exec::ECHOUEE;
			}

			m_corps.prims()->detache();

			volume = dynamic_cast<Volume *>(m_corps.prims()->prim(idx_volume));

			break;
		}
	}

	auto chef = contexte.chef;

	if (!m_compileuse.compile_graphe(contexte, &m_corps)) {
		ajoute_avertissement("Ne peut pas compiler le graphe, voir si les noeuds n'ont pas d'erreurs.");
		return res_exec::ECHOUEE;
	}

	switch (type_detail) {
		case DETAIL_POINTS:
		{
			chef->demarre_evaluation("graphe détail points");

			auto points_entree = m_corps.points_pour_lecture();
			auto points_sortie = static_cast<ListePoints3D *>(nullptr);

			auto donnees_prop = m_compileuse.m_gest_attrs.donnees_pour_propriete("P");
			if (donnees_prop->est_modifiee) {
				points_sortie = m_corps.points_pour_ecriture();
			}

			boucle_parallele(tbb::blocked_range<long>(0, points_entree->taille()),
							 [&](tbb::blocked_range<long> const &plage)
			{
				if (chef->interrompu()) {
					return;
				}

				/* fais une copie locale pour éviter les problèmes de concurrence critique */
				auto donnees = m_compileuse.donnees();

				for (auto i = plage.begin(); i < plage.end(); ++i) {
					auto ctx_local = lcc::ctx_local{};

					auto pos = points_entree->point(i);
					m_compileuse.remplis_donnees(donnees, "P", pos);

					/* stocke les attributs */
					m_compileuse.stocke_attributs(donnees, i);

					m_compileuse.execute_pile(ctx_local, donnees);

					auto idx_sortie = m_compileuse.pointeur_donnees("P");
					pos = donnees.charge_vec3(idx_sortie);

					/* charge les attributs */
					m_compileuse.charge_attributs(donnees, i);

					if (points_sortie) {
						points_sortie->point(i, pos);
					}
				}

				auto delta = static_cast<float>(plage.end() - plage.begin());
				delta /= static_cast<float>(points_entree->taille());
				chef->indique_progression_parallele(delta * 100.0f);
			});

			/* réinitialise la transformation puisque nous l'appliquons aux
			 * points dans la boucle au dessus */
			/* À FAIRE : retramsforme les points, via une accesseuse_points */
			//m_corps.transformation = math::transformation();

			break;
		}
		case DETAIL_VOXELS:
		{
			auto chef_wolika = ChefWolika(chef, "graphe détail voxels");

			auto grille = volume->grille;

			if (grille->est_eparse()) {
				auto grille_eparse = dynamic_cast<wlk::grille_eparse<float> *>(grille);

				wlk::pour_chaque_tuile_parallele(
							*grille_eparse,
							[&](wlk::tuile_scalaire<float> *tuile)
				{
					auto donnees = m_compileuse.donnees();

					auto index_tuile = 0;
					for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
						for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
							for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
								auto ctx_local = lcc::ctx_local{};
								auto pos_tuile = tuile->min;
								pos_tuile.x += i;
								pos_tuile.y += j;
								pos_tuile.z += k;

								auto pos_monde = grille_eparse->index_vers_monde(pos_tuile);
								auto pos_unit = grille_eparse->index_vers_unit(pos_tuile);

								auto v = tuile->donnees[index_tuile];

								m_compileuse.remplis_donnees(donnees, "densité", v);
								m_compileuse.remplis_donnees(donnees, "pos_monde", pos_monde);
								m_compileuse.remplis_donnees(donnees, "pos_unit", pos_unit);

								m_compileuse.execute_pile(ctx_local, donnees);

								auto idx_sortie = m_compileuse.m_gest_attrs.pointeur_donnees("densité");
								v = donnees.charge_decimal(idx_sortie);

								tuile->donnees[index_tuile] = v;
							}
						}
					}
				},
				&chef_wolika);
			}

			break;
		}
	}

	return res_exec::REUSSIE;
}

/* ************************************************************************** */

static auto converti_type_prise(lcc::type_var type)
{
	switch (type) {
		case lcc::type_var::DEC:
			return type_prise::DECIMAL;
		case lcc::type_var::ENT32:
			return type_prise::ENTIER;
		case lcc::type_var::VEC2:
			return type_prise::VEC2;
		case lcc::type_var::VEC3:
			return type_prise::VEC3;
		case lcc::type_var::VEC4:
			return type_prise::VEC4;
		case lcc::type_var::COULEUR:
			return type_prise::COULEUR;
		case lcc::type_var::MAT3:
			return type_prise::MAT3;
		case lcc::type_var::MAT4:
			return type_prise::MAT4;
		case lcc::type_var::CHAINE:
			return type_prise::CHAINE;
		case lcc::type_var::INVALIDE:
			return type_prise::INVALIDE;
		case lcc::type_var::TABLEAU:
			return type_prise::TABLEAU;
		case lcc::type_var::POLYMORPHIQUE:
			return type_prise::POLYMORPHIQUE;
	}

	return type_prise::INVALIDE;
}

static auto converti_type_prise(type_prise type)
{
	switch (type) {
		case type_prise::DECIMAL:
			return lcc::type_var::DEC;
		case type_prise::ENTIER:
			return lcc::type_var::ENT32;
		case type_prise::VEC2:
			return lcc::type_var::VEC2;
		case type_prise::VEC3:
			return lcc::type_var::VEC3;
		case type_prise::VEC4:
			return lcc::type_var::VEC4;
		case type_prise::COULEUR:
			return lcc::type_var::COULEUR;
		case type_prise::MAT3:
			return lcc::type_var::MAT3;
		case type_prise::MAT4:
			return lcc::type_var::MAT4;
		case type_prise::CHAINE:
			return lcc::type_var::CHAINE;
		case type_prise::INVALIDE:
			return lcc::type_var::INVALIDE;
		case type_prise::TABLEAU:
			return lcc::type_var::TABLEAU;
		case type_prise::POLYMORPHIQUE:
			return lcc::type_var::POLYMORPHIQUE;
		case type_prise::CORPS:
		case type_prise::IMAGE:
		case type_prise::OBJET:
			return lcc::type_var::INVALIDE;
	}

	return lcc::type_var::INVALIDE;
}

OperatriceFonctionDetail::OperatriceFonctionDetail(
		Graphe &graphe_parent,
		Noeud &noeud_,
		const dls::chaine &nom_fonc,
		const lcc::donnees_fonction *df)
	: OperatriceImage(graphe_parent, noeud_)
	, donnees_fonction(df)
	, nom_fonction(nom_fonc)
{
	if (donnees_fonction == nullptr) {
		entrees(0);
		sorties(0);
		return;
	}

	entrees(donnees_fonction->seing.entrees.taille());
	sorties(donnees_fonction->seing.sorties.taille());
}

const char *OperatriceFonctionDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceFonctionDetail::texte_aide() const
{
	return AIDE;
}

const char *OperatriceFonctionDetail::nom_entree(int i)
{
	return donnees_fonction->seing.entrees.nom(i);
}

const char *OperatriceFonctionDetail::nom_sortie(int i)
{
	return donnees_fonction->seing.sorties.nom(i);
}

int OperatriceFonctionDetail::type() const
{
	return OPERATRICE_DETAIL;
}

type_prise OperatriceFonctionDetail::type_entree(int i) const
{
	return converti_type_prise(donnees_fonction->seing.entrees.type(i));
}

type_prise OperatriceFonctionDetail::type_sortie(int i) const
{
	return converti_type_prise(donnees_fonction->seing.sorties.type(i));
}

inline auto corrige_type_specialise(lcc::type_var type_specialise, lcc::type_var type)
{
	if (type_specialise == lcc::type_var::INVALIDE) {
		return type;
	}

	if (taille_type(type) > taille_type(type_specialise)) {
		return type;
	}

	return type_specialise;
}

#undef DEBOGUE_SPECIALISATION

res_exec OperatriceFonctionDetail::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	INUTILISE(contexte);
	/* réimplémentation du code de génération d'instruction pour les appels de
	 * fonctions de LCC */

	auto coulisse = std::any_cast<dls::chaine>(donnees_aval->table["coulisse"]);
	auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);

	/* défini le type spécialisé pour les connexions d'entrées */

	auto type_params = lcc::types_entrees();
	auto type_specialise = lcc::type_var::INVALIDE;
	auto est_polymorphique = false;

	/* première boucle : défini le type spécialisé pour les connexions d'entrées */
	for (auto i = 0; i < entrees(); ++i) {
		auto type = donnees_fonction->seing.entrees.type(i);
		est_polymorphique = (type == lcc::type_var::POLYMORPHIQUE);

		if (entree(i)->connectee()) {
			auto ptr = entree(i)->pointeur();
			auto prise_sortie = ptr->liens[0];

			if (type == lcc::type_var::POLYMORPHIQUE) {
				type = converti_type_prise(prise_sortie->type_infere);
				type_specialise = corrige_type_specialise(type_specialise, type);
			}
		}

		type_params.ajoute(type);
	}

	/* deuxième boucle : cherche les pointeurs des entrées et converti les
	 * valeurs selon le type spécialisé */

	if (est_polymorphique && type_specialise == lcc::type_var::INVALIDE) {
		this->ajoute_avertissement("Ne peut pas instancier la fonction car les entrées polymorphiques n'ont pas de connexion.");
		return res_exec::ECHOUEE;
	}

#ifdef DEBOGUE_SPECIALISATION
	if (type_specialise != lcc::type_var::INVALIDE) {
		std::cerr << "Spécialisation pour type : " << lcc::chaine_type_var(type_specialise) << '\n';
	}
#endif

	auto pointeurs = dls::tableau<int>();

	for (auto i = 0; i < entrees(); ++i) {
		auto type = donnees_fonction->seing.entrees.type(i);

		if (entree(i)->connectee()) {
			auto ptr = entree(i)->pointeur();
			auto prise_sortie = ptr->liens[0];
			auto decalage = static_cast<int>(prise_sortie->decalage_pile);

			/* converti au besoin */
			if (type == lcc::type_var::POLYMORPHIQUE) {
				auto type_prise = converti_type_prise(prise_sortie->type_infere);

				if (type_prise != type_specialise) {
#ifdef DEBOGUE_SPECIALISATION
					std::cerr << "Conversion de "
							  << lcc::chaine_type_var(type_prise)
							  << " vers "
							  << lcc::chaine_type_var(type_specialise) << '\n';
#endif
					decalage = lcc::ajoute_conversion(*compileuse, type_prise, type_specialise, decalage);
				}
			}

			pointeurs.pousse(decalage);
		}
		else {
			/* alloue une valeur par défaut et prend le pointeurs */
			auto ptr = 0;

			if (type == lcc::type_var::POLYMORPHIQUE) {
				ptr = compileuse->donnees().loge_donnees(lcc::taille_type(type_specialise));
				auto ptr_loc = ptr;

				auto valeur = evalue_vecteur(donnees_fonction->seing.entrees.nom(i), contexte.temps_courant);

				switch (type_specialise) {
					case lcc::type_var::ENT32:
					{
						compileuse->donnees().stocke(ptr_loc, static_cast<int>(valeur[0]));
						break;
					}
					case lcc::type_var::DEC:
					{
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						break;
					}
					case lcc::type_var::VEC2:
					{
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						compileuse->donnees().stocke(ptr_loc, valeur[1]);
						break;
					}
					default:
					{
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
				}
			}
			else {
				ptr = compileuse->donnees().loge_donnees(lcc::taille_type(type));
				auto ptr_loc = ptr;

				auto nom_prop = donnees_fonction->seing.entrees.nom(i);

				switch (type) {
					case lcc::type_var::ENT32:
					{
						auto valeur = evalue_entier(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::DEC:
					{
						auto valeur = evalue_decimal(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::VEC2:
					{
						auto valeur = evalue_vecteur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						compileuse->donnees().stocke(ptr_loc, valeur[1]);
						break;
					}
					case lcc::type_var::VEC3:
					case lcc::type_var::VEC4:
					{
						auto valeur = evalue_vecteur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::COULEUR:
					{
						auto valeur = evalue_couleur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					default:
					{
						break;
					}
				}
			}

			pointeurs.pousse(ptr);
		}
	}

	/* ****************** crée les données pour les appels ****************** */

	if (coulisse == "lcc") {
		cree_code_coulisse_processeur(compileuse, type_specialise, pointeurs);
	}
	else {
		cree_code_coulisse_opengl(donnees_aval, type_specialise, pointeurs, contexte.temps_courant);
	}

	return res_exec::REUSSIE;
}

void OperatriceFonctionDetail::cree_code_coulisse_processeur(
		compileuse_lng *compileuse,
		lcc::type_var type_specialise,
		dls::tableau<int> const &pointeurs)
{
	/* ajoute le code_inst de la fonction */
	compileuse->ajoute_instructions(donnees_fonction->type);

	/* ajoute le type de la fonction pour choisir la bonne surcharge */
	if (type_specialise != lcc::type_var::INVALIDE) {
		compileuse->ajoute_instructions(type_specialise);
	}

	/* ajoute le pointeur de chaque paramètre */
	for (auto ptr : pointeurs) {
		compileuse->ajoute_instructions(ptr);
	}

	/* pour chaque sortie, nous réservons de la place sur la pile de données */
	auto pointeur_donnees = 0;
	for (auto i = 0; i < sorties(); ++i) {
		auto type = donnees_fonction->seing.sorties.type(i);

		if (type == lcc::type_var::POLYMORPHIQUE) {
			type = type_specialise;
		}

		auto pointeur = compileuse->donnees().loge_donnees(lcc::taille_type(type));
		auto prise_sortie = sortie(i)->pointeur();

		prise_sortie->decalage_pile = pointeur;
		prise_sortie->type_infere = converti_type_prise(type);

		if (i == 0) {
			pointeur_donnees = pointeur;
		}
	}

	/* ajoute le pointeur du premier paramètre aux instructions pour savoir
	 * où écrire */
	compileuse->ajoute_instructions(pointeur_donnees);
}

void OperatriceFonctionDetail::cree_code_coulisse_opengl(
		DonneesAval *donnees_aval,
		lcc::type_var type_specialise,
		dls::tableau<int> const &pointeurs,
		int temps_courant)
{
	auto donnees_compil = std::any_cast<DonneesCompilationNuanceur *>(donnees_aval->table["donnees_compil"]);
	auto &os = donnees_compil->os;

	for (auto i = 0; i < entrees(); ++i) {
		auto type = donnees_fonction->seing.entrees.type(i);
		auto ptr = pointeurs[i];

		if (!entree(i)->connectee()) {
			if (type == lcc::type_var::POLYMORPHIQUE) {
				auto valeur = evalue_vecteur(donnees_fonction->seing.entrees.nom(i), temps_courant);

				switch (type_specialise) {
					case lcc::type_var::ENT32:
					{
						os << "int __var" << ptr << " = " << static_cast<int>(valeur[0]) << ";\n";
						break;
					}
					case lcc::type_var::DEC:
					{
						os << "float __var" << ptr << " = " << valeur[0] << ";\n";
						break;
					}
					case lcc::type_var::VEC2:
					{
						os << "vec2 __var" << ptr << " = vec2(" << valeur[0] << ',' << valeur[1] << ");\n";
						break;
					}
					default:
					{
						os << "vec3 __var" << ptr << " = vec2(" << valeur[0] << ',' << valeur[1] << ',' << valeur[2] << ");\n";
						break;
					}
				}
			}
			else {
				auto nom_prop = donnees_fonction->seing.entrees.nom(i);

				switch (type) {
					case lcc::type_var::ENT32:
					{
						auto valeur = evalue_entier(nom_prop, temps_courant);
						os << "int __var" << ptr << " = " << valeur << ";\n";
						break;
					}
					case lcc::type_var::DEC:
					{
						auto valeur = evalue_decimal(nom_prop, temps_courant);
						os << "float __var" << ptr << " = " << valeur << ";\n";
						break;
					}
					case lcc::type_var::VEC2:
					{
						auto valeur = evalue_vecteur(nom_prop, temps_courant);
						os << "vec2 __var" << ptr << " = vec2(" << valeur[0] << ',' << valeur[1] << ");\n";
						break;
					}
					case lcc::type_var::VEC3:
					{
						auto valeur = evalue_vecteur(nom_prop, temps_courant);
						os << "vec3 __var" << ptr << " = vec3(" << valeur[0] << ',' << valeur[1] << ',' << valeur[2] << ");\n";
						break;
					}
					case lcc::type_var::VEC4:
					{
						auto valeur = evalue_vecteur(nom_prop, temps_courant);
						os << "vec4 __var" << ptr << " = vec4(" << valeur[0] << ',' << valeur[1] << ',' << valeur[2] << ", 0.0);\n";
						break;
					}
					case lcc::type_var::COULEUR:
					{
						auto valeur = evalue_couleur(nom_prop, temps_courant);
						os << "vec4 __var" << ptr << " = vec4(" << valeur[0] << ',' << valeur[1] << ',' << valeur[2] << ',' << valeur[2] << ");\n";
						break;
					}
					default:
					{
						break;
					}
				}
			}
		}
	}

	for (auto i = 0; i < sorties(); ++i) {
		/* ajourne les types inférés des sorties */
		auto type = donnees_fonction->seing.sorties.type(i);

		if (type == lcc::type_var::POLYMORPHIQUE) {
			type = type_specialise;
		}

		auto prise_sortie = sortie(i)->pointeur();
		prise_sortie->type_infere = converti_type_prise(type);

		os << type_var_opengl(type);
		os << " __var" << prise_sortie->decalage_pile << ";\n";
	}

	/* crée l'appel */

	os << lng::supprime_accents(nom_fonction);

	auto virgule = '(';

	for (auto ptr : pointeurs) {
		os << virgule << "__var" << ptr;
		virgule = ',';
	}

	for (auto i = 0; i < sorties(); ++i) {
		auto prise_sortie = sortie(i)->pointeur();
		os << virgule << "__var" << prise_sortie->decalage_pile;
		virgule = ',';
	}

	os << ");\n";
}

void OperatriceFonctionDetail::cree_proprietes()
{
	for (auto i = 0; i < entrees(); ++i) {
		auto nom_propriete = donnees_fonction->seing.entrees.nom(i);
		auto valeur_default = donnees_fonction->seing.entrees.valeur_defaut(i);

		auto prop = danjo::Propriete();

		switch (donnees_fonction->seing.entrees.type(i)) {
			case lcc::type_var::DEC:
			{
				prop.type = danjo::TypePropriete::DECIMAL;
				prop.valeur = valeur_default;
				break;
			}
			case lcc::type_var::ENT32:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = static_cast<int>(valeur_default);
				break;
			}
			case lcc::type_var::VEC2:
			case lcc::type_var::VEC3:
			case lcc::type_var::VEC4:
			case lcc::type_var::POLYMORPHIQUE:
			{
				prop.type = danjo::TypePropriete::VECTEUR;
				prop.valeur = dls::math::vec3f(valeur_default);
				break;
			}
			case lcc::type_var::COULEUR:
			{
				prop.type = danjo::TypePropriete::COULEUR;
				prop.valeur = dls::phys::couleur32(valeur_default);
				break;
			}
			case lcc::type_var::CHAINE:
			{
				prop.type = danjo::TypePropriete::CHAINE_CARACTERE;
				prop.valeur = dls::chaine();
				break;
			}
			case lcc::type_var::MAT3:
			case lcc::type_var::MAT4:
			case lcc::type_var::INVALIDE:
			case lcc::type_var::TABLEAU:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = static_cast<int>(valeur_default);
				break;
			}
		}

		ajoute_propriete_extra(nom_propriete, prop);
	}
}

/* ************************************************************************** */

class OperatriceEntreeDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Entrée Détail";
	static constexpr auto AIDE = "Entrée Détail";

	OperatriceEntreeDetail(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);

		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_entree[type_detail];
		sorties(params_noeud.taille());
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *nom_sortie(int i) override
	{
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_entree[type_detail];

		if (i >= params_noeud.taille()) {
			return "INVALIDE !";
		}

		return params_noeud.nom(i);
	}

	type_prise type_sortie(int i) const override
	{
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_entree[type_detail];

		if (i >= params_noeud.taille()) {
			return type_prise::INVALIDE;
		}

		return converti_type_prise(params_noeud.type(i));
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto coulisse = std::any_cast<dls::chaine>(donnees_aval->table["coulisse"]);
		auto gest_attrs = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_attrs"]);
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_entree[type_detail];

		if (coulisse == "lcc") {
			for (auto i = 0; i < params_noeud.taille(); ++i) {
				auto prise_sortie = sortie(i)->pointeur();
				prise_sortie->decalage_pile = gest_attrs->pointeur_donnees(params_noeud.nom(i));
				prise_sortie->type_infere = converti_type_prise(params_noeud.type(i));
			}
		}
		else {
			auto donnees_compil = std::any_cast<DonneesCompilationNuanceur *>(donnees_aval->table["donnees_compil"]);
			auto &os = donnees_compil->os;

			for (auto i = 0; i < params_noeud.taille(); ++i) {
				auto prise_sortie = sortie(i)->pointeur();
				prise_sortie->type_infere = converti_type_prise(params_noeud.type(i));

				os << type_var_opengl(params_noeud.type(i));
				os << " __var" << prise_sortie->decalage_pile;
				os << " = " << params_noeud.nom(i) << ";\n";
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSortieDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Sortie Détail";
	static constexpr auto AIDE = "Sortie Détail";

	OperatriceSortieDetail(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_sortie[type_detail];
		entrees(params_noeud.taille());
		sorties(0);

		noeud.est_sortie = true;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *nom_entree(int i) override
	{
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_sortie[type_detail];

		if (i >= params_noeud.taille()) {
			return "INVALIDE !";
		}

		return params_noeud.nom(i);
	}

	type_prise type_entree(int i) const override
	{
		INUTILISE(i);
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		auto const &params_noeud = params_noeuds_sortie[type_detail];

		if (i >= params_noeud.taille()) {
			return type_prise::INVALIDE;
		}

		return converti_type_prise(params_noeud.type(i));
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto coulisse = std::any_cast<dls::chaine>(donnees_aval->table["coulisse"]);
		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto gest_attrs = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_attrs"]);
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);

		if (coulisse == "lcc") {
			auto const &params_noeud = params_noeuds_sortie[type_detail];

			for (auto i = 0; i < params_noeud.taille(); ++i) {
				if (!gest_attrs->propriete_existe(params_noeud.nom(i))) {
					auto idx = compileuse->donnees().loge_donnees(taille_type(params_noeud.type(i)));
					gest_attrs->ajoute_propriete(params_noeud.nom(i), params_noeud.type(i), idx);
				}

				if (entree(i)->connectee()) {
					auto ptr_prise = entree(i)->pointeur();
					auto ptr_entree = static_cast<int>(ptr_prise->liens[0]->decalage_pile);
					auto ptr_sortie = gest_attrs->pointeur_donnees(params_noeud.nom(i));

					compileuse->ajoute_instructions(lcc::code_inst::ASSIGNATION);
					compileuse->ajoute_instructions(params_noeud.type(i));
					compileuse->ajoute_instructions(ptr_entree);
					compileuse->ajoute_instructions(ptr_sortie);
				}
			}
		}
		else if (coulisse == "opengl") {
			auto donnees_compil = std::any_cast<DonneesCompilationNuanceur *>(donnees_aval->table["donnees_compil"]);
			auto &os = donnees_compil->os;

			if (entree(0)->connectee()) {
				auto ptr_prise = entree(0)->pointeur();
				auto ptr_sortie = ptr_prise->liens[0];

				os << "couleur_sortie = __var" << ptr_sortie->decalage_pile << ";\n";
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceEntreeAttribut final : public OperatriceImage {
public:
	static constexpr auto NOM = "Entrée Attribut";
	static constexpr auto AIDE = "Entrée Attribut";

	OperatriceEntreeAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_attribut_detail.jo";
	}

	type_prise type_sortie(int) const override
	{
		return type_prise::POLYMORPHIQUE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto gest_attrs = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_attrs"]);
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);
		auto nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			this->ajoute_avertissement("Le nom de l'attribut est vide");
			return res_exec::ECHOUEE;
		}

		switch (type_detail) {
			case DETAIL_POINTS:
			{
				auto donnees = gest_attrs->donnees_pour_propriete(nom_attribut);

				if (donnees == nullptr) {
					this->ajoute_avertissement("L'attribut n'existe pas !");
					return res_exec::ECHOUEE;
				}

				auto prise_sortie = sortie(0)->pointeur();
				prise_sortie->decalage_pile = donnees->ptr;
				prise_sortie->type_infere = converti_type_prise(donnees->type);

				break;
			}
			case DETAIL_NUANCAGE:
			{
				break;
			}
			case DETAIL_PIXELS:
			{
				this->ajoute_avertissement("Opératrice non-implémentée pour les pixels");
				return res_exec::ECHOUEE;
			}
			case DETAIL_VOXELS:
			{
				this->ajoute_avertissement("Opératrice non-implémentée pour les voxels");
				return res_exec::ECHOUEE;
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSortieAttribut final : public OperatriceImage {
public:
	static constexpr auto NOM = "Sortie Attribut";
	static constexpr auto AIDE = "Sortie Attribut";

	OperatriceSortieAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
		sorties(0);

		noeud.est_sortie = true;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_attribut_detail.jo";
	}

	type_prise type_entree(int) const override
	{
		return type_prise::POLYMORPHIQUE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto gest_attrs = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_attrs"]);
		auto type_detail = std::any_cast<int>(m_graphe_parent.donnees[0]);
		auto nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			this->ajoute_avertissement("Le nom de l'attribut est vide");
			return res_exec::ECHOUEE;
		}

		switch (type_detail) {
			case DETAIL_POINTS:
			{
				if (!entree(0)->connectee()) {
					/* aucune connexion */
					return res_exec::REUSSIE;
				}

				auto ptr_prise_entree = entree(0)->pointeur();
				auto prise_sortie = ptr_prise_entree->liens[0];
				auto ptr_entree = static_cast<int>(prise_sortie->decalage_pile);
				auto type = converti_type_prise(prise_sortie->type_infere);

				if (type == lcc::type_var::POLYMORPHIQUE) {
					this->ajoute_avertissement("Le type de sortie est toujours polymorphique");
					return res_exec::ECHOUEE;
				}

				auto donnees = gest_attrs->donnees_pour_propriete(nom_attribut);

				if (donnees == nullptr) {
					auto ptr = compileuse->donnees().loge_donnees(taille_type(type));
					gest_attrs->requiers_attr(nom_attribut, type, ptr);
					donnees = gest_attrs->donnees_pour_propriete(nom_attribut);
				}
				else {
					if (type != donnees->type) {
						this->ajoute_avertissement("Le type n'est pas bon");
						return res_exec::ECHOUEE;
					}
				}

				compileuse->ajoute_instructions(lcc::code_inst::ASSIGNATION);
				compileuse->ajoute_instructions(type);
				compileuse->ajoute_instructions(ptr_entree);
				compileuse->ajoute_instructions(donnees->ptr);

				break;
			}
			case DETAIL_PIXELS:
			{
				this->ajoute_avertissement("Opératrice non-implémentée pour les pixels");
				return res_exec::ECHOUEE;
			}
			case DETAIL_VOXELS:
			{
				this->ajoute_avertissement("Opératrice non-implémentée pour les voxels");
				return res_exec::ECHOUEE;
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpChargeImage final : public OperatriceImage {
public:
	static constexpr auto NOM = "Charge Image";
	static constexpr auto AIDE = "Charge Image";

	OpChargeImage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_detail_echantimage.jo";
	}

	type_prise type_sortie(int i) const override
	{
		switch (i) {
			case 0:
			{
				return type_prise::ENTIER;
			}
		}

		return type_prise::INVALIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto ctx_global = std::any_cast<lcc::ctx_exec *>(donnees_aval->table["ctx_global"]);

		auto chemin_noeud = evalue_chaine("chemin_noeud");

		if (chemin_noeud == "") {
			this->ajoute_avertissement("Le chemin du noeud pour l'image est vide");
			return res_exec::ECHOUEE;
		}

		auto noeud_image = cherche_noeud_pour_chemin(*contexte.bdd, chemin_noeud);

		if (noeud_image == nullptr) {
			this->ajoute_avertissement("Impossible de trouver le noeud spécifié");
			return res_exec::ECHOUEE;
		}

		if (noeud_image->type != type_noeud::OPERATRICE) {
			this->ajoute_avertissement("Le noeud n'est pas un noeud composite !");
			return res_exec::ECHOUEE;
		}

		auto op = extrait_opimage(noeud_image->donnees);
		auto image = op->image();

		auto calque = image->calque_pour_lecture("image");

		if (calque == nullptr) {
			this->ajoute_avertissement("Calque « image » introuvable !");
			return res_exec::ECHOUEE;
		}

		auto ptr = compileuse->donnees().loge_donnees(taille_type(lcc::type_var::ENT32));

		auto ptr_sortie = sortie(0)->pointeur();
		ptr_sortie->decalage_pile = ptr;
		ptr_sortie->type_infere = type_prise::ENTIER;

		compileuse->donnees().stocke(ptr, static_cast<int>(ctx_global->images.taille()));

		ctx_global->images.pousse(image);

		return res_exec::REUSSIE;
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "chemin_noeud") {
			for (auto composite : contexte.bdd->composites()) {
				for (auto enfant : composite->noeud->enfants) {
					liste.pousse(enfant->chemin());
				}
			}
		}
	}
};

/* ************************************************************************** */

template <int O>
struct desc_operatrice_courbe_rampe;

template <>
struct desc_operatrice_courbe_rampe<0> {
	static constexpr auto NOM = "Crée Courbe Couleur";
	static constexpr auto AIDE = "Crée un ensemble de courbe bézier pour transformer une couleur.";
	static constexpr auto chemin_entreface = "entreface/operatrice_detail_courbe_couleur.jo";
};

template <>
struct desc_operatrice_courbe_rampe<1> {
	static constexpr auto NOM = "Crée Courbe Valeur";
	static constexpr auto AIDE = "Crée une courbe bézier pour transformer une valeur scalaire.";
	static constexpr auto chemin_entreface = "entreface/operatrice_detail_courbe_valeur.jo";
};

template <>
struct desc_operatrice_courbe_rampe<2> {
	static constexpr auto NOM = "Crée Rampe Couleur";
	static constexpr auto AIDE = "Crée une rampe pour interpoler des couleurs depuis une valeur scalaire.";
	static constexpr auto chemin_entreface = "entreface/operatrice_detail_rampe_couleur.jo";
};

template <int O>
class OpCreeCourbeRampe final : public OperatriceImage {
public:
	static constexpr auto NOM = desc_operatrice_courbe_rampe<O>::NOM;
	static constexpr auto AIDE = desc_operatrice_courbe_rampe<O>::AIDE;

	OpCreeCourbeRampe(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return desc_operatrice_courbe_rampe<O>::chemin_entreface;
	}

	type_prise type_sortie(int i) const override
	{
		switch (i) {
			case 0:
			{
				return type_prise::ENTIER;
			}
		}

		return type_prise::INVALIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto ctx_global = std::any_cast<lcc::ctx_exec *>(donnees_aval->table["ctx_global"]);

		auto ptr = compileuse->donnees().loge_donnees(taille_type(lcc::type_var::ENT32));

		auto ptr_sortie = sortie(0)->pointeur();
		ptr_sortie->decalage_pile = ptr;
		ptr_sortie->type_infere = type_prise::ENTIER;

		if (O == 0) {
			auto courbe_couleur = evalue_courbe_couleur("courbe");
			compileuse->donnees().stocke(ptr, static_cast<int>(ctx_global->courbes_couleur.taille()));
			ctx_global->courbes_couleur.pousse(courbe_couleur);
		}
		else if (O == 1) {
			auto courbe_valeur = evalue_courbe_valeur("courbe");
			compileuse->donnees().stocke(ptr, static_cast<int>(ctx_global->courbes_valeur.taille()));
			ctx_global->courbes_valeur.pousse(courbe_valeur);
		}
		else if (O == 2) {
			auto rampe_valeur = evalue_rampe_couleur("rampe");
			compileuse->donnees().stocke(ptr, static_cast<int>(ctx_global->rampes_couleur.taille()));
			ctx_global->rampes_couleur.pousse(rampe_valeur);
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpChercheCamera final : public OperatriceImage {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Cherche Caméra";
	static constexpr auto AIDE = "Cherche Caméra";

	OpChercheCamera(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	COPIE_CONSTRUCT(OpChercheCamera);

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_detail_camera.jo";
	}

	type_prise type_sortie(int i) const override
	{
		switch (i) {
			case 0:
			{
				return type_prise::ENTIER;
			}
		}

		return type_prise::INVALIDE;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto ctx_global = std::any_cast<lcc::ctx_exec *>(donnees_aval->table["ctx_global"]);

		auto chemin_camera = evalue_chaine("chemin_caméra");

		if (chemin_camera == "") {
			this->ajoute_avertissement("Le chemin de la caméra est vide");
			return res_exec::ECHOUEE;
		}

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Ne peut pas trouver l'objet caméra !");
			return res_exec::ECHOUEE;
		}

		if (m_objet->type != type_objet::CAMERA) {
			this->ajoute_avertissement("L'objet n'est pas une caméra !");
			return res_exec::ECHOUEE;
		}

		auto ptr = compileuse->donnees().loge_donnees(taille_type(lcc::type_var::ENT32));

		auto ptr_sortie = sortie(0)->pointeur();
		ptr_sortie->decalage_pile = ptr;
		ptr_sortie->type_infere = type_prise::ENTIER;

		compileuse->donnees().stocke(ptr, static_cast<int>(ctx_global->cameras.taille()));

		auto camera = static_cast<vision::Camera3D *>(nullptr);

		m_objet->donnees.accede_ecriture([&](DonneesObjet *donnees)
		{
			camera = &extrait_camera(donnees);
		});

		ctx_global->cameras.pousse(camera);

		return res_exec::REUSSIE;
	}

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_caméra");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet == nullptr) {
				return;
			}
		}

		compilatrice.ajoute_dependance(noeud_reseau, m_objet);
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_caméra") {
			for (auto &objet : contexte.bdd->objets()) {
				if (objet->type != type_objet::CAMERA) {
					continue;
				}

				liste.pousse(objet->noeud->nom);
			}
		}
	}
};

/* ************************************************************************** */

static auto params_info = lcc::param_sorties(
			lcc::donnees_parametre("temps_image", lcc::type_var::ENT32),
			lcc::donnees_parametre("temps_fractionnel", lcc::type_var::DEC),
			lcc::donnees_parametre("temps_seconde", lcc::type_var::DEC));

class OperatriceInfoExecution final : public OperatriceImage {
public:
	static constexpr auto NOM = "Info Exécution";
	static constexpr auto AIDE = "Donne des infos sur le contexte d'exécution du graphe.";

	OperatriceInfoExecution(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(params_info.taille());
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	type_prise type_sortie(int i) const override
	{
		return converti_type_prise(params_info.type(i));
	}

	const char *nom_sortie(int i) override
	{
		return params_info.nom(i);
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		auto gest_attrs = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_attrs"]);
		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);

		for (auto i = 0; i < sorties(); ++i) {
			auto idx = compileuse->donnees().loge_donnees(taille_type(params_info.type(i)));
			gest_attrs->ajoute_propriete(params_info.nom(i), params_info.type(i), idx);

			auto prise_sortie = sortie(i)->pointeur();
			prise_sortie->decalage_pile = idx;
			prise_sortie->type_infere = type_sortie(i);

			switch (i) {
				case 0:
				{
					compileuse->donnees().stocke(idx, contexte.temps_courant);
					break;
				}
				case 1:
				{
					auto temps_frac = static_cast<float>(contexte.temps_courant);
					temps_frac /= static_cast<float>(contexte.temps_fin - contexte.temps_debut);
					compileuse->donnees().stocke(idx, temps_frac);
					break;
				}
				case 2:
				{
					auto temps_sec = static_cast<float>(contexte.temps_courant);
					temps_sec /= static_cast<float>(contexte.cadence);
					compileuse->donnees().stocke(idx, temps_sec);
					break;
				}
			}
		}

		return res_exec::REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return noeud.a_des_sorties_liees();
	}
};

/* ************************************************************************** */

#include "nuanceur.hh"

bool compile_nuanceur_opengl(ContexteEvaluation const &contexte, Nuanceur &nuanceur)
{
	// résultat de la compilation nous donne des requêtes pour des attributs et
	// une source pour le corps du nuanceur fragment

	auto &graphe = nuanceur.noeud.graphe;

	if (graphe.besoin_ajournement) {
		tri_topologique(graphe);
		graphe.besoin_ajournement = false;
	}

	auto m_compileuse = compileuse_lng();
	auto m_ctx_global = lcc::ctx_exec{};
	auto m_gest_attrs = gestionnaire_propriete{};

	auto donnees_compil = DonneesCompilationNuanceur();

	auto donnees_aval = DonneesAval{};
	donnees_aval.table.insere({"coulisse", dls::chaine("opengl")});
	donnees_aval.table.insere({"compileuse", &m_compileuse});
	donnees_aval.table.insere({"ctx_global", &m_ctx_global});
	donnees_aval.table.insere({"gest_attrs", &m_gest_attrs});
	donnees_aval.table.insere({"donnees_compil", &donnees_compil});

	std::cerr << "++++++++++++ compilation nuanceur ++++++++++++\n";

	/* performe la compilation */
	for (auto &noeud_graphe : graphe.noeuds()) {
		for (auto &sortie : noeud_graphe->sorties) {
			sortie->decalage_pile = ++donnees_compil.decalage_prise;
		}

		auto operatrice = extrait_opimage(noeud_graphe->donnees);
		operatrice->reinitialise_avertisements();

		auto resultat = operatrice->execute(contexte, &donnees_aval);

		if (resultat == res_exec::ECHOUEE) {
			return false;
		}
	}

	/* nuanceur vertex */
	auto flux_vert = dls::flux_chaine();
	flux_vert << "#version 330 core\n";
	flux_vert << "layout (location=0) in vec3 sommets;\n";
	flux_vert << "layout (location=1) in vec3 normaux;\n";
	flux_vert << "uniform mat4 matrice;\n";
	flux_vert << "uniform mat4 MVP;\n";
	flux_vert << "smooth out vec3 P;\n";
	flux_vert << "smooth out vec3 N;\n";
	flux_vert << "void main()\n";
	flux_vert << "{\n";
	flux_vert << "vec4 P_monde = matrice * vec4(P, 1.0);\n";
	flux_vert << "gl_Position = MVP * P_monde;\n";
	flux_vert << "P = P_monde.xyz;\n";
	flux_vert << "N = normaux;\n";
	flux_vert << "}\n";

	nuanceur.source_vert_glsl = flux_vert.chn();

	/* nuanceur fragment */
	auto source_fonctions_poly = dls::contenu_fichier("nuanceurs/fonctions_polymorphiques.glsl");
	auto source_fonctions_norm = dls::contenu_fichier("nuanceurs/fonctions_normales.glsl");

	auto flux_frag = dls::flux_chaine();
	flux_frag << "#version 330 core\n";
	flux_frag << "layout (location=0) out vec4 couleur_sortie;\n";
	flux_frag << "smooth in vec3 P;\n";
	flux_frag << "smooth in vec3 N;\n";

	/* fonctions normales */
	flux_frag << source_fonctions_norm << '\n';

	/* fonctions polymorphiques */
	flux_frag << "#define TYPE_POLY float\n";
	flux_frag << source_fonctions_poly << '\n';
	flux_frag << "#define TYPE_POLY vec2\n";
	flux_frag << source_fonctions_poly << '\n';
	flux_frag << "#define TYPE_POLY vec3\n";
	flux_frag << source_fonctions_poly << '\n';
	flux_frag << "#define TYPE_POLY vec4\n";
	flux_frag << source_fonctions_poly << '\n';

	flux_frag << "void main()\n";
	flux_frag << "{\n";
	flux_frag << "couleur_sortie = vec4(1.0, 0.0, 1.0, 1.0);\n";
	flux_frag << donnees_compil.os.chn();
	flux_frag << "}\n";

	nuanceur.source_frag_glsl = flux_frag.chn();

	std::cerr << nuanceur.source_vert_glsl << '\n';
	std::cerr << nuanceur.source_frag_glsl << '\n';

	return true;
}

/* ************************************************************************** */

void enregistre_operatrices_detail(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGrapheDetail>());
	usine.enregistre_type(cree_desc<OperatriceEntreeDetail>());
	usine.enregistre_type(cree_desc<OperatriceSortieDetail>());
	usine.enregistre_type(cree_desc<OperatriceEntreeAttribut>());
	usine.enregistre_type(cree_desc<OperatriceSortieAttribut>());
	usine.enregistre_type(cree_desc<OperatriceInfoExecution>());
	usine.enregistre_type(cree_desc<OpChargeImage>());
	usine.enregistre_type(cree_desc<OpCreeCourbeRampe<0>>());
	usine.enregistre_type(cree_desc<OpCreeCourbeRampe<1>>());
	usine.enregistre_type(cree_desc<OpCreeCourbeRampe<2>>());
}

OperatriceFonctionDetail *cree_op_detail(
		Mikisa &mikisa,
		Graphe &graphe,
		Noeud &noeud,
		const dls::chaine &nom_fonction)
{
	/* À FAIRE : gestion des fonctions avec surcharges. */
	auto const &table_df = mikisa.lcc->fonctions.table[nom_fonction];
	auto df = &table_df.front();

	return memoire::loge<OperatriceFonctionDetail>("Fonction Détail", graphe, noeud, nom_fonction, df);
}

#pragma clang diagnostic pop

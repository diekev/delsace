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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "typeuse.hh"

#include "operateurs.hh"
#include "outils_morceaux.hh"

/* ************************************************************************** */

long Typeuse::Indexeuse::trouve_index(const DonneesTypeFinal &dt)
{
	auto iter = donnees_type_index.trouve(dt);

	if (iter != donnees_type_index.fin()) {
		return iter->second;
	}

	return -1;
}

long Typeuse::Indexeuse::index_type(const DonneesTypeFinal &dt)
{
	auto index = donnees_types.taille();
	donnees_types.pousse(dt);

	donnees_type_index.insere({dt, index});

	return index;
}

/* ************************************************************************** */

struct DonneesTypeCommun {
	TypeBase val_enum;
	DonneesTypeFinal dt;
};

static const DonneesTypeCommun donnees_types_communs[] = {
	{ TypeBase::N8, DonneesTypeFinal(id_morceau::N8) },
	{ TypeBase::N16, DonneesTypeFinal(id_morceau::N16) },
	{ TypeBase::N32, DonneesTypeFinal(id_morceau::N32) },
	{ TypeBase::N64, DonneesTypeFinal(id_morceau::N64) },
	{ TypeBase::N128, DonneesTypeFinal(id_morceau::N128) },
	{ TypeBase::Z8, DonneesTypeFinal(id_morceau::Z8) },
	{ TypeBase::Z16, DonneesTypeFinal(id_morceau::Z16) },
	{ TypeBase::Z32, DonneesTypeFinal(id_morceau::Z32) },
	{ TypeBase::Z64, DonneesTypeFinal(id_morceau::Z64) },
	{ TypeBase::Z128, DonneesTypeFinal(id_morceau::Z128) },
	{ TypeBase::R16, DonneesTypeFinal(id_morceau::R16) },
	{ TypeBase::R32, DonneesTypeFinal(id_morceau::R32) },
	{ TypeBase::R64, DonneesTypeFinal(id_morceau::R64) },
	{ TypeBase::R128, DonneesTypeFinal(id_morceau::R128) },
	{ TypeBase::EINI, DonneesTypeFinal(id_morceau::EINI) },
	{ TypeBase::CHAINE, DonneesTypeFinal(id_morceau::CHAINE) },
	{ TypeBase::RIEN, DonneesTypeFinal(id_morceau::RIEN) },
	{ TypeBase::BOOL, DonneesTypeFinal(id_morceau::BOOL) },
	{ TypeBase::OCTET, DonneesTypeFinal(id_morceau::OCTET) },

	{ TypeBase::PTR_N8, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::N8) },
	{ TypeBase::PTR_N16, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::N16) },
	{ TypeBase::PTR_N32, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::N32) },
	{ TypeBase::PTR_N64, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::N64) },
	{ TypeBase::PTR_N128, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::N128) },
	{ TypeBase::PTR_Z8, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::Z8) },
	{ TypeBase::PTR_Z16, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::Z16) },
	{ TypeBase::PTR_Z32, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::Z32) },
	{ TypeBase::PTR_Z64, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::Z64) },
	{ TypeBase::PTR_Z128, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::Z128) },
	{ TypeBase::PTR_R16, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::R16) },
	{ TypeBase::PTR_R32, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::R32) },
	{ TypeBase::PTR_R64, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::R64) },
	{ TypeBase::PTR_R128, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::R128) },
	{ TypeBase::PTR_EINI, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::EINI) },
	{ TypeBase::PTR_CHAINE, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::CHAINE) },
	{ TypeBase::PTR_RIEN, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::RIEN) },
	{ TypeBase::PTR_NUL, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::NUL) },
	{ TypeBase::PTR_BOOL, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::BOOL) },
	{ TypeBase::PTR_OCTET, DonneesTypeFinal(id_morceau::POINTEUR, id_morceau::OCTET) },

	{ TypeBase::REF_N8, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::N8) },
	{ TypeBase::REF_N16, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::N16) },
	{ TypeBase::REF_N32, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::N32) },
	{ TypeBase::REF_N64, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::N64) },
	{ TypeBase::REF_N128, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::N128) },
	{ TypeBase::REF_Z8, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::Z8) },
	{ TypeBase::REF_Z16, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::Z16) },
	{ TypeBase::REF_Z32, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::Z32) },
	{ TypeBase::REF_Z64, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::Z64) },
	{ TypeBase::REF_Z128, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::Z128) },
	{ TypeBase::REF_R16, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::R16) },
	{ TypeBase::REF_R32, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::R32) },
	{ TypeBase::REF_R64, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::R64) },
	{ TypeBase::REF_R128, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::R128) },
	{ TypeBase::REF_EINI, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::EINI) },
	{ TypeBase::REF_CHAINE, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::CHAINE) },
	{ TypeBase::REF_RIEN, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::RIEN) },
	{ TypeBase::REF_NUL, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::NUL) },
	{ TypeBase::REF_BOOL, DonneesTypeFinal(id_morceau::REFERENCE, id_morceau::BOOL) },

	{ TypeBase::TABL_N8, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::N8) },
	{ TypeBase::TABL_N16, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::N16) },
	{ TypeBase::TABL_N32, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::N32) },
	{ TypeBase::TABL_N64, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::N64) },
	{ TypeBase::TABL_N128, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::N128) },
	{ TypeBase::TABL_Z8, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::Z8) },
	{ TypeBase::TABL_Z16, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::Z16) },
	{ TypeBase::TABL_Z32, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::Z32) },
	{ TypeBase::TABL_Z64, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::Z64) },
	{ TypeBase::TABL_Z128, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::Z128) },
	{ TypeBase::TABL_R16, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::R16) },
	{ TypeBase::TABL_R32, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::R32) },
	{ TypeBase::TABL_R64, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::R64) },
	{ TypeBase::TABL_R128, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::R128) },
	{ TypeBase::TABL_EINI, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::EINI) },
	{ TypeBase::TABL_CHAINE, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::CHAINE) },
	{ TypeBase::TABL_BOOL, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::BOOL) },
	{ TypeBase::TABL_OCTET, DonneesTypeFinal(id_morceau::TABLEAU, id_morceau::OCTET) },
};

/* ************************************************************************** */

Typeuse::Typeuse(GrapheDependance &g, Operateurs &o)
	: graphe(g)
	, operateurs(o)
{
	/* initialise les types communs */
	indexeuse.index_types_communs.redimensionne(static_cast<long>(TypeBase::TOTAL));

	/* initialise_relation_pour_type dépend de l'index de certains types de
	 * bases, donc d'abord nous créons leurs index, et ensuite nous générons les
	 * relations de bases */
	for (auto donnees : donnees_types_communs) {
		auto const idx = static_cast<long>(donnees.val_enum);
		indexeuse.index_types_communs[idx] = ajoute_type_sans_relations(donnees.dt);
	}

	for (auto donnees : donnees_types_communs) {
		initialise_relations_pour_type(donnees.dt, indexeuse[donnees.val_enum]);
	}
}

long Typeuse::ajoute_type(const DonneesTypeFinal &donnees)
{
	auto index = ajoute_type_sans_relations(donnees);

	if (index != -1l) {
		initialise_relations_pour_type(donnees, index);
	}

	return index;
}

long Typeuse::ajoute_type_sans_relations(const DonneesTypeFinal &donnees)
{
	if (donnees.est_invalide()) {
		return -1l;
	}

	auto index = indexeuse.trouve_index(donnees);

	if (index != -1) {
		return index;
	}

	index = indexeuse.index_type(donnees);

	auto index_params = dls::tableau<long>();

	/* ajoute les types des paramètres et de retour des fonctions */
	if (donnees.type_base() == id_morceau::FONC || donnees.type_base() == id_morceau::COROUT) {
		long nombre_type_retour = 0;
		index_params = donnees_types_parametres(*this, donnees, nombre_type_retour);
	}

	graphe.cree_noeud_type(index);

	for (auto index_param : index_params) {
		graphe.connecte_type_type(index, index_param);
	}

	return index;
}

long Typeuse::type_tableau_pour(long index_type)
{
	auto idx = graphe.trouve_index_type(index_type, TypeRelation::TYPE_TABLEAU);

	if (idx == -1l) {
		auto dt = DonneesTypeFinal();
		dt.pousse(id_morceau::TABLEAU);
		dt.pousse(indexeuse.donnees_types[index_type]);

		idx = ajoute_type(dt);
	}

	return idx;
}

long Typeuse::type_reference_pour(long index_type)
{
	auto idx = graphe.trouve_index_type(index_type, TypeRelation::TYPE_REFERENCE);

	if (idx == -1l) {
		auto dt = DonneesTypeFinal();
		dt.pousse(id_morceau::REFERENCE);
		dt.pousse(indexeuse.donnees_types[index_type]);

		idx = ajoute_type(dt);
	}

	return idx;
}

long Typeuse::type_dereference_pour(long index_type)
{
	return graphe.trouve_index_type(index_type, TypeRelation::TYPE_DEREFERENCE);
}

long Typeuse::type_pointeur_pour(long index_type)
{
	auto idx = graphe.trouve_index_type(index_type, TypeRelation::TYPE_POINTEUR);

	if (idx == -1l) {
		auto dt = DonneesTypeFinal();
		dt.pousse(id_morceau::POINTEUR);
		dt.pousse(indexeuse.donnees_types[index_type]);

		idx = ajoute_type(dt);
	}

	return idx;
}

void Typeuse::initialise_relations_pour_type(const DonneesTypeFinal &dt, long i)
{
	auto const &idx_dt_ptr_nul = indexeuse[TypeBase::PTR_NUL];
	auto const &idx_dt_bool = indexeuse[TypeBase::BOOL];

	if (!peut_etre_dereference(dt.type_base())) {
		return;
	}

	/* ajoute d'abord les sous-types possibles afin de résoudre les problèmes de
	 * dépendances lors de la génération du code C */
	auto idx_deref = this->ajoute_type(dt.dereference());

	if (idx_deref == -1l) {
		return;
	}

	graphe.connecte_type_type(i, idx_deref);

	if (dt.type_base() == id_morceau::REFERENCE) {
		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_REFERENCE);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);
	}
	else if ((dt.type_base() & 0xff) == id_morceau::TABLEAU) {
		if (est_type_tableau_fixe(dt.plage())) {
			auto dt_tableau = DonneesTypeFinal();
			dt_tableau.pousse(id_morceau::TABLEAU);
			dt_tableau.pousse(dt.dereference());

			this->ajoute_type(dt_tableau);
		}

		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_TABLEAU);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);
	}
	else if (dt.type_base() == id_morceau::POINTEUR) {
		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_POINTEUR);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);

		/* À FAIRE : considération de la taille en octet des types */
		std::array<long, 10> const types_entiers = {
			indexeuse[TypeBase::N8],
			indexeuse[TypeBase::N16],
			indexeuse[TypeBase::N32],
			indexeuse[TypeBase::N64],
			indexeuse[TypeBase::N128],
			indexeuse[TypeBase::Z8],
			indexeuse[TypeBase::Z16],
			indexeuse[TypeBase::Z32],
			indexeuse[TypeBase::Z64],
			indexeuse[TypeBase::Z128],
		};

		operateurs.ajoute_basique(id_morceau::EGALITE, i, idx_dt_ptr_nul, idx_dt_bool);
		operateurs.ajoute_basique(id_morceau::DIFFERENCE, i, idx_dt_ptr_nul, idx_dt_bool);
		operateurs.ajoute_basique(id_morceau::INFERIEUR, i, idx_dt_bool);
		operateurs.ajoute_basique(id_morceau::INFERIEUR_EGAL, i, idx_dt_bool);
		operateurs.ajoute_basique(id_morceau::SUPERIEUR, i, idx_dt_bool);
		operateurs.ajoute_basique(id_morceau::SUPERIEUR_EGAL, i, idx_dt_bool);

		for (auto idx_type_entier : types_entiers) {
			operateurs.ajoute_basique(id_morceau::PLUS, i, idx_type_entier, i);
			operateurs.ajoute_basique(id_morceau::MOINS, i, idx_type_entier, i);
			operateurs.ajoute_basique(id_morceau::PLUS_EGAL, i, idx_type_entier, i);
			operateurs.ajoute_basique(id_morceau::MOINS_EGAL, i, idx_type_entier, i);
		}
	}
}

/* ************************************************************************** */

static DonneesTypeFinal analyse_type(
		DonneesTypeFinal::iterateur_const &debut,
		bool pousse_virgule = false)
{
	auto dt = DonneesTypeFinal{};

	if (*debut == id_morceau::FONC || *debut == id_morceau::COROUT) {
		/* fonc ou corout */
		dt.pousse(*debut--);

		/* ( */
		dt.pousse(*debut--);
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(analyse_type(debut, true));
		}
		/* ) */
		dt.pousse(*debut--);

		/* ( */
		dt.pousse(*debut--);
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(analyse_type(debut, true));
		}
		/* ) */
		dt.pousse(*debut--);

		if (*debut == id_morceau::VIRGULE) {
			--debut;
		}
	}
	else {
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(*debut--);

			if (*debut == id_morceau::VIRGULE) {
				if (pousse_virgule) {
					dt.pousse(*debut);
				}

				--debut;
				break;
			}
		}
	}

	return dt;
}

/**
 * Retourne un vecteur contenant les DonneesTypeFinal de chaque paramètre et du
 * type de retour d'un DonneesTypeFinal d'un pointeur fonction. Si le
 * DonneesTypeFinal passé en paramètre n'est pas un pointeur fonction, retourne
 * un vecteur vide.
 */
[[nodiscard]] auto donnees_types_parametres(
		Typeuse &typeuse,
		DonneesTypeFinal const &donnees_type,
		long &nombre_types_retour) noexcept(false) -> dls::tableau<long>
{
	if (donnees_type.type_base() != id_morceau::FONC && donnees_type.type_base() != id_morceau::COROUT) {
		return {};
	}

	auto donnees_types = dls::tableau<long>{};

	auto debut = donnees_type.end() - 1;

	--debut; /* fonc ou corout */
	--debut; /* ( */

	/* type paramètres */
	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		auto dt = analyse_type(debut);
		donnees_types.pousse(typeuse.ajoute_type(dt));
	}

	--debut; /* ) */

	--debut; /* ( */

	/* type retour */
	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		auto dt = analyse_type(debut);
		donnees_types.pousse(typeuse.ajoute_type(dt));
		++nombre_types_retour;
	}

	--debut; /* ) */

	return donnees_types;
}

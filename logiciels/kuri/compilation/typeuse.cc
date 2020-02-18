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
#include "outils_lexemes.hh"

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
	{ TypeBase::N8, DonneesTypeFinal(GenreLexeme::N8) },
	{ TypeBase::N16, DonneesTypeFinal(GenreLexeme::N16) },
	{ TypeBase::N32, DonneesTypeFinal(GenreLexeme::N32) },
	{ TypeBase::N64, DonneesTypeFinal(GenreLexeme::N64) },
	{ TypeBase::Z8, DonneesTypeFinal(GenreLexeme::Z8) },
	{ TypeBase::Z16, DonneesTypeFinal(GenreLexeme::Z16) },
	{ TypeBase::Z32, DonneesTypeFinal(GenreLexeme::Z32) },
	{ TypeBase::Z64, DonneesTypeFinal(GenreLexeme::Z64) },
	{ TypeBase::R16, DonneesTypeFinal(GenreLexeme::R16) },
	{ TypeBase::R32, DonneesTypeFinal(GenreLexeme::R32) },
	{ TypeBase::R64, DonneesTypeFinal(GenreLexeme::R64) },
	{ TypeBase::EINI, DonneesTypeFinal(GenreLexeme::EINI) },
	{ TypeBase::CHAINE, DonneesTypeFinal(GenreLexeme::CHAINE) },
	{ TypeBase::RIEN, DonneesTypeFinal(GenreLexeme::RIEN) },
	{ TypeBase::BOOL, DonneesTypeFinal(GenreLexeme::BOOL) },
	{ TypeBase::OCTET, DonneesTypeFinal(GenreLexeme::OCTET) },

	{ TypeBase::PTR_N8, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::N8) },
	{ TypeBase::PTR_N16, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::N16) },
	{ TypeBase::PTR_N32, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::N32) },
	{ TypeBase::PTR_N64, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::N64) },
	{ TypeBase::PTR_Z8, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::Z8) },
	{ TypeBase::PTR_Z16, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::Z16) },
	{ TypeBase::PTR_Z32, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::Z32) },
	{ TypeBase::PTR_Z64, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::Z64) },
	{ TypeBase::PTR_R16, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::R16) },
	{ TypeBase::PTR_R32, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::R32) },
	{ TypeBase::PTR_R64, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::R64) },
	{ TypeBase::PTR_EINI, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::EINI) },
	{ TypeBase::PTR_CHAINE, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::CHAINE) },
	{ TypeBase::PTR_RIEN, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::RIEN) },
	{ TypeBase::PTR_NUL, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::NUL) },
	{ TypeBase::PTR_BOOL, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::BOOL) },
	{ TypeBase::PTR_OCTET, DonneesTypeFinal(GenreLexeme::POINTEUR, GenreLexeme::OCTET) },

	{ TypeBase::REF_N8, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::N8) },
	{ TypeBase::REF_N16, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::N16) },
	{ TypeBase::REF_N32, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::N32) },
	{ TypeBase::REF_N64, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::N64) },
	{ TypeBase::REF_Z8, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::Z8) },
	{ TypeBase::REF_Z16, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::Z16) },
	{ TypeBase::REF_Z32, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::Z32) },
	{ TypeBase::REF_Z64, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::Z64) },
	{ TypeBase::REF_R16, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::R16) },
	{ TypeBase::REF_R32, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::R32) },
	{ TypeBase::REF_R64, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::R64) },
	{ TypeBase::REF_EINI, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::EINI) },
	{ TypeBase::REF_CHAINE, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::CHAINE) },
	{ TypeBase::REF_RIEN, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::RIEN) },
	{ TypeBase::REF_NUL, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::NUL) },
	{ TypeBase::REF_BOOL, DonneesTypeFinal(GenreLexeme::REFERENCE, GenreLexeme::BOOL) },

	{ TypeBase::TABL_N8, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::N8) },
	{ TypeBase::TABL_N16, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::N16) },
	{ TypeBase::TABL_N32, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::N32) },
	{ TypeBase::TABL_N64, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::N64) },
	{ TypeBase::TABL_Z8, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::Z8) },
	{ TypeBase::TABL_Z16, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::Z16) },
	{ TypeBase::TABL_Z32, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::Z32) },
	{ TypeBase::TABL_Z64, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::Z64) },
	{ TypeBase::TABL_R16, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::R16) },
	{ TypeBase::TABL_R32, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::R32) },
	{ TypeBase::TABL_R64, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::R64) },
	{ TypeBase::TABL_EINI, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::EINI) },
	{ TypeBase::TABL_CHAINE, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::CHAINE) },
	{ TypeBase::TABL_BOOL, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::BOOL) },
	{ TypeBase::TABL_OCTET, DonneesTypeFinal(GenreLexeme::TABLEAU, GenreLexeme::OCTET) },
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
	if (donnees.type_base() == GenreLexeme::FONC || donnees.type_base() == GenreLexeme::COROUT) {
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
		dt.pousse(GenreLexeme::TABLEAU);
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
		dt.pousse(GenreLexeme::REFERENCE);
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
		dt.pousse(GenreLexeme::POINTEUR);
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

	if (dt.type_base() == GenreLexeme::REFERENCE) {
		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_REFERENCE);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);
	}
	else if ((dt.type_base() & 0xff) == GenreLexeme::TABLEAU) {
		if (est_type_tableau_fixe(dt.plage())) {
			auto dt_tableau = DonneesTypeFinal();
			dt_tableau.pousse(GenreLexeme::TABLEAU);
			dt_tableau.pousse(dt.dereference());

			this->ajoute_type(dt_tableau);
		}

		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_TABLEAU);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);
	}
	else if (dt.type_base() == GenreLexeme::POINTEUR) {
		graphe.connecte_type_type(idx_deref, i, TypeRelation::TYPE_POINTEUR);
		graphe.connecte_type_type(i, idx_deref, TypeRelation::TYPE_DEREFERENCE);

		auto indice = IndiceTypeOp::ENTIER_RELATIF;
		auto raison = RaisonOp::POUR_COMPARAISON;

		operateurs.ajoute_basique(GenreLexeme::EGALITE, i, idx_dt_ptr_nul, idx_dt_bool, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, i, idx_dt_ptr_nul, idx_dt_bool, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::INFERIEUR, i, idx_dt_bool, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::INFERIEUR_EGAL, i, idx_dt_bool, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::SUPERIEUR, i, idx_dt_bool, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::SUPERIEUR_EGAL, i, idx_dt_bool, indice, raison);

		/* Pour l'arithmétique de pointeur nous n'utilisons que le type le plus
		 * gros, la résolution de l'opérateur ajoutera une transformation afin
		 * que le type plus petit soit transtyper à la bonne taille. */
		auto idx_type_entier = indexeuse[TypeBase::Z64];
		raison = RaisonOp::POUR_ARITHMETIQUE;

		operateurs.ajoute_basique(GenreLexeme::PLUS, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::MOINS, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::MOINS, i, i, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::PLUS_EGAL, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::MOINS_EGAL, i, idx_type_entier, i, indice, raison);

		idx_type_entier = indexeuse[TypeBase::N64];
		indice = IndiceTypeOp::ENTIER_NATUREL;

		operateurs.ajoute_basique(GenreLexeme::PLUS, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::MOINS, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::PLUS_EGAL, i, idx_type_entier, i, indice, raison);
		operateurs.ajoute_basique(GenreLexeme::MOINS_EGAL, i, idx_type_entier, i, indice, raison);
	}
}

/* ************************************************************************** */

static DonneesTypeFinal analyse_type(
		DonneesTypeFinal::iterateur_const &debut,
		bool pousse_virgule = false)
{
	auto dt = DonneesTypeFinal{};

	if (*debut == GenreLexeme::FONC || *debut == GenreLexeme::COROUT) {
		/* fonc ou corout */
		dt.pousse(*debut--);

		/* ( */
		dt.pousse(*debut--);
		while (*debut != GenreLexeme::PARENTHESE_FERMANTE) {
			dt.pousse(analyse_type(debut, true));
		}
		/* ) */
		dt.pousse(*debut--);

		/* ( */
		dt.pousse(*debut--);
		while (*debut != GenreLexeme::PARENTHESE_FERMANTE) {
			dt.pousse(analyse_type(debut, true));
		}
		/* ) */
		dt.pousse(*debut--);

		if (*debut == GenreLexeme::VIRGULE) {
			--debut;
		}
	}
	else {
		while (*debut != GenreLexeme::PARENTHESE_FERMANTE) {
			dt.pousse(*debut--);

			if (*debut == GenreLexeme::VIRGULE) {
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
	if (donnees_type.type_base() != GenreLexeme::FONC && donnees_type.type_base() != GenreLexeme::COROUT) {
		return {};
	}

	auto donnees_types = dls::tableau<long>{};

	auto debut = donnees_type.end() - 1;

	--debut; /* fonc ou corout */
	--debut; /* ( */

	/* type paramètres */
	while (*debut != GenreLexeme::PARENTHESE_FERMANTE) {
		auto dt = analyse_type(debut);
		donnees_types.pousse(typeuse.ajoute_type(dt));
	}

	--debut; /* ) */

	--debut; /* ( */

	/* type retour */
	while (*debut != GenreLexeme::PARENTHESE_FERMANTE) {
		auto dt = analyse_type(debut);
		donnees_types.pousse(typeuse.ajoute_type(dt));
		++nombre_types_retour;
	}

	--debut; /* ) */

	return donnees_types;
}

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

#include "typage.hh"

#include "biblinternes/outils/assert.hh"

#include "compilatrice.hh"
#include "operateurs.hh"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "statistiques.hh"

/* ************************************************************************** */

struct DonneesTypeCommun {
	TypeBase val_enum;
	GenreLexeme dt[2];
};

static DonneesTypeCommun donnees_types_communs[] = {
	{ TypeBase::PTR_N8, { GenreLexeme::POINTEUR, GenreLexeme::N8 } },
	{ TypeBase::PTR_N16, { GenreLexeme::POINTEUR, GenreLexeme::N16 } },
	{ TypeBase::PTR_N32, { GenreLexeme::POINTEUR, GenreLexeme::N32 } },
	{ TypeBase::PTR_N64, { GenreLexeme::POINTEUR, GenreLexeme::N64 } },
	{ TypeBase::PTR_Z8, { GenreLexeme::POINTEUR, GenreLexeme::Z8 } },
	{ TypeBase::PTR_Z16, { GenreLexeme::POINTEUR, GenreLexeme::Z16 } },
	{ TypeBase::PTR_Z32, { GenreLexeme::POINTEUR, GenreLexeme::Z32 } },
	{ TypeBase::PTR_Z64, { GenreLexeme::POINTEUR, GenreLexeme::Z64 } },
	{ TypeBase::PTR_R16, { GenreLexeme::POINTEUR, GenreLexeme::R16 } },
	{ TypeBase::PTR_R32, { GenreLexeme::POINTEUR, GenreLexeme::R32 } },
	{ TypeBase::PTR_R64, { GenreLexeme::POINTEUR, GenreLexeme::R64 } },
	{ TypeBase::PTR_EINI, { GenreLexeme::POINTEUR, GenreLexeme::EINI } },
	{ TypeBase::PTR_CHAINE, { GenreLexeme::POINTEUR, GenreLexeme::CHAINE } },
	{ TypeBase::PTR_RIEN, { GenreLexeme::POINTEUR, GenreLexeme::RIEN } },
	{ TypeBase::PTR_NUL, { GenreLexeme::POINTEUR, GenreLexeme::NUL } },
	{ TypeBase::PTR_BOOL, { GenreLexeme::POINTEUR, GenreLexeme::BOOL } },
	{ TypeBase::PTR_OCTET, { GenreLexeme::POINTEUR, GenreLexeme::OCTET } },

	{ TypeBase::REF_N8, { GenreLexeme::REFERENCE, GenreLexeme::N8 } },
	{ TypeBase::REF_N16, { GenreLexeme::REFERENCE, GenreLexeme::N16 } },
	{ TypeBase::REF_N32, { GenreLexeme::REFERENCE, GenreLexeme::N32 } },
	{ TypeBase::REF_N64, { GenreLexeme::REFERENCE, GenreLexeme::N64 } },
	{ TypeBase::REF_Z8, { GenreLexeme::REFERENCE, GenreLexeme::Z8 } },
	{ TypeBase::REF_Z16, { GenreLexeme::REFERENCE, GenreLexeme::Z16 } },
	{ TypeBase::REF_Z32, { GenreLexeme::REFERENCE, GenreLexeme::Z32 } },
	{ TypeBase::REF_Z64, { GenreLexeme::REFERENCE, GenreLexeme::Z64 } },
	{ TypeBase::REF_R16, { GenreLexeme::REFERENCE, GenreLexeme::R16 } },
	{ TypeBase::REF_R32, { GenreLexeme::REFERENCE, GenreLexeme::R32 } },
	{ TypeBase::REF_R64, { GenreLexeme::REFERENCE, GenreLexeme::R64 } },
	{ TypeBase::REF_EINI, { GenreLexeme::REFERENCE, GenreLexeme::EINI } },
	{ TypeBase::REF_CHAINE, { GenreLexeme::REFERENCE, GenreLexeme::CHAINE } },
	{ TypeBase::REF_RIEN, { GenreLexeme::REFERENCE, GenreLexeme::RIEN } },
	{ TypeBase::REF_BOOL, { GenreLexeme::REFERENCE, GenreLexeme::BOOL } },

	{ TypeBase::TABL_N8, { GenreLexeme::TABLEAU, GenreLexeme::N8 } },
	{ TypeBase::TABL_N16, { GenreLexeme::TABLEAU, GenreLexeme::N16 } },
	{ TypeBase::TABL_N32, { GenreLexeme::TABLEAU, GenreLexeme::N32 } },
	{ TypeBase::TABL_N64, { GenreLexeme::TABLEAU, GenreLexeme::N64 } },
	{ TypeBase::TABL_Z8, { GenreLexeme::TABLEAU, GenreLexeme::Z8 } },
	{ TypeBase::TABL_Z16, { GenreLexeme::TABLEAU, GenreLexeme::Z16 } },
	{ TypeBase::TABL_Z32, { GenreLexeme::TABLEAU, GenreLexeme::Z32 } },
	{ TypeBase::TABL_Z64, { GenreLexeme::TABLEAU, GenreLexeme::Z64 } },
	{ TypeBase::TABL_R16, { GenreLexeme::TABLEAU, GenreLexeme::R16 } },
	{ TypeBase::TABL_R32, { GenreLexeme::TABLEAU, GenreLexeme::R32 } },
	{ TypeBase::TABL_R64, { GenreLexeme::TABLEAU, GenreLexeme::R64 } },
	{ TypeBase::TABL_EINI, { GenreLexeme::TABLEAU, GenreLexeme::EINI } },
	{ TypeBase::TABL_CHAINE, { GenreLexeme::TABLEAU, GenreLexeme::CHAINE } },
	{ TypeBase::TABL_BOOL, { GenreLexeme::TABLEAU, GenreLexeme::BOOL } },
	{ TypeBase::TABL_OCTET, { GenreLexeme::TABLEAU, GenreLexeme::OCTET } },
};

/* ************************************************************************** */

const char *chaine_genre_type(GenreType genre)
{
#define ENUMERE_GENRE_TYPE_EX(genre) case GenreType::genre: { return #genre; }
	switch (genre) {
		ENUMERE_GENRES_TYPES
	}
#undef ENUMERE_GENRE_TYPE_EX

	return "erreur, ceci ne devrait pas s'afficher";
}

std::ostream &operator<<(std::ostream &os, GenreType genre)
{
	os << chaine_genre_type(genre);
	return os;
}

/* ************************************************************************** */

Type *Type::cree_entier(unsigned taille_octet, bool est_naturel)
{
	auto type = memoire::loge<Type>("Type");
	type->genre = est_naturel ? GenreType::ENTIER_NATUREL : GenreType::ENTIER_RELATIF;
	type->taille_octet = taille_octet;
	type->alignement = taille_octet;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);
	return type;
}

Type *Type::cree_entier_constant()
{
	auto type = memoire::loge<Type>("Type");
	type->genre = GenreType::ENTIER_CONSTANT;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);
	return type;
}

Type *Type::cree_reel(unsigned taille_octet)
{
	auto type = memoire::loge<Type>("Type");
	type->genre = GenreType::REEL;
	type->taille_octet = taille_octet;
	type->alignement = taille_octet;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);
	return type;
}

Type *Type::cree_rien()
{
	auto type = memoire::loge<Type>("Type");
	type->genre = GenreType::RIEN;
	type->taille_octet = 0;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);
	return type;
}

Type *Type::cree_bool()
{
	auto type = memoire::loge<Type>("Type");
	type->genre = GenreType::BOOL;
	type->taille_octet = 1;
	type->alignement = 1;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);
	return type;
}

Type *Type::cree_octet()
{
	auto type = memoire::loge<Type>("Type");
	type->genre = GenreType::OCTET;
	type->taille_octet = 1;
	type->alignement = 1;
	type->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);
	return type;
}

TypePointeur::TypePointeur(Type *type_pointe_)
	: TypePointeur()
{
	this->type_pointe = type_pointe_;
	this->taille_octet = 8;
	this->alignement = 8;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);

	if (type_pointe_) {
		if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
		}

		type_pointe_->drapeaux |= POSSEDE_TYPE_POINTEUR;
	}
}

TypeReference::TypeReference(Type *type_pointe_)
	: TypeReference()
{
	assert(type_pointe_);

	this->type_pointe = type_pointe_;
	this->taille_octet = 8;
	this->alignement = 8;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);

	if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
	}

	type_pointe_->drapeaux |= POSSEDE_TYPE_REFERENCE;
}

TypeFonction::TypeFonction(dls::tablet<Type *, 6> const &entrees, dls::tablet<Type *, 6> const &sorties)
	: TypeFonction()
{
	this->types_entrees.reserve(entrees.taille());
	POUR (entrees) {
		this->types_entrees.pousse(it);
	}

	this->types_sorties.reserve(sorties.taille());
	POUR (sorties) {
		this->types_sorties.pousse(it);
	}

	this->taille_octet = 8;
	this->alignement = 8;
	this->marque_polymorphique();
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);
}

void TypeFonction::marque_polymorphique()
{
	POUR (types_entrees) {
		if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
			return;
		}
	}

	POUR (types_sorties) {
		if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
			this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
			return;
		}
	}
}

TypeCompose *TypeCompose::cree_eini()
{
	auto type = memoire::loge<TypeCompose>("TypeCompose");
	type->genre = GenreType::EINI;
	type->taille_octet = 16;
	type->alignement = 8;
	type->drapeaux = (TYPE_EST_NORMALISE);
	return type;
}

TypeCompose *TypeCompose::cree_chaine()
{
	auto type = memoire::loge<TypeCompose>("TypeCompose");
	type->genre = GenreType::CHAINE;
	type->taille_octet = 16;
	type->alignement = 8;
	type->drapeaux = (TYPE_EST_NORMALISE);
	return type;
}

TypeTableauFixe::TypeTableauFixe(Type *type_pointe_, long taille_, kuri::tableau<TypeCompose::Membre> &&membres_)
	: TypeTableauFixe()
{
	assert(type_pointe_);

	this->membres = std::move(membres_);
	this->type_pointe = type_pointe_;
	this->taille = taille_;
	this->alignement = type_pointe_->alignement;
	this->taille_octet = type_pointe_->taille_octet * static_cast<unsigned>(taille_);
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);

	if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
	}

	type_pointe_->drapeaux |= POSSEDE_TYPE_TABLEAU_FIXE;
}

TypeTableauDynamique::TypeTableauDynamique(Type *type_pointe_, kuri::tableau<TypeCompose::Membre> &&membres_)
	: TypeTableauDynamique()
{
	assert(type_pointe_);

	this->membres = std::move(membres_);
	this->type_pointe = type_pointe_;
	this->taille_octet = 24;
	this->alignement = 8;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);

	if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
	}

	type_pointe_->drapeaux |= POSSEDE_TYPE_TABLEAU_DYNAMIQUE;
}

TypeVariadique::TypeVariadique(Type *type_pointe_, kuri::tableau<TypeCompose::Membre> &&membres_)
	: TypeVariadique()
{
	this->type_pointe = type_pointe_;

	if (type_pointe_ && type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
	}

	this->membres = std::move(membres_);
	this->taille_octet = 24;
	this->alignement = 8;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);
}

TypeTypeDeDonnees::TypeTypeDeDonnees(Type *type_connu_)
	: TypeTypeDeDonnees()
{
	this->genre = GenreType::TYPE_DE_DONNEES;
	// un type 'type' est un genre de pointeur déguisé, donc donnons lui les mêmes caractéristiques
	this->taille_octet = 8;
	this->alignement = 8;
	this->type_connu = type_connu_;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);

	if (type_connu_) {
		type_connu_->drapeaux |= POSSEDE_TYPE_TYPE_DE_DONNEES;
	}
}

TypePolymorphique::TypePolymorphique(IdentifiantCode *ident_)
	: TypePolymorphique()
{
	this->ident = ident_;
	this->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE);
}

TypeOpaque::TypeOpaque(IdentifiantCode *ident_, Type *opacifie)
	: TypeOpaque()
{
	this->ident = ident_;
	this->type_opacifie = opacifie;
	this->drapeaux |= TYPE_FUT_VALIDE;
}

/* ************************************************************************** */

static Type *cree_type_pour_lexeme(GenreLexeme lexeme)
{
	switch (lexeme) {
		case GenreLexeme::BOOL:
		{
			return Type::cree_bool();
		}
		case GenreLexeme::OCTET:
		{
			return Type::cree_octet();
		}
		case GenreLexeme::N8:
		{
			return Type::cree_entier(1, true);
		}
		case GenreLexeme::Z8:
		{
			return Type::cree_entier(1, false);
		}
		case GenreLexeme::N16:
		{
			return Type::cree_entier(2, true);
		}
		case GenreLexeme::Z16:
		{
			return Type::cree_entier(2, false);
		}
		case GenreLexeme::N32:
		{
			return Type::cree_entier(4, true);
		}
		case GenreLexeme::Z32:
		{
			return Type::cree_entier(4, false);
		}
		case GenreLexeme::N64:
		{
			return Type::cree_entier(8, true);
		}
		case GenreLexeme::Z64:
		{
			return Type::cree_entier(8, false);
		}
		case GenreLexeme::R16:
		{
			return Type::cree_reel(2);
		}
		case GenreLexeme::R32:
		{
			return Type::cree_reel(4);
		}
		case GenreLexeme::R64:
		{
			return Type::cree_reel(8);
		}
		case GenreLexeme::RIEN:
		{
			return Type::cree_rien();
		}
		default:
		{
			return nullptr;
		}
	}
}

Typeuse::Typeuse(dls::outils::Synchrone<GrapheDependance> &g, dls::outils::Synchrone<Operateurs> &o)
	: graphe_(g)
	, operateurs_(o)
{
	/* initialise les types communs */
	types_communs.redimensionne(static_cast<long>(TypeBase::TOTAL));

	type_eini = TypeCompose::cree_eini();
	type_chaine = TypeCompose::cree_chaine();

	types_communs[static_cast<long>(TypeBase::N8)] = cree_type_pour_lexeme(GenreLexeme::N8);
	types_communs[static_cast<long>(TypeBase::N16)] = cree_type_pour_lexeme(GenreLexeme::N16);
	types_communs[static_cast<long>(TypeBase::N32)] = cree_type_pour_lexeme(GenreLexeme::N32);
	types_communs[static_cast<long>(TypeBase::N64)] = cree_type_pour_lexeme(GenreLexeme::N64);
	types_communs[static_cast<long>(TypeBase::Z8)] = cree_type_pour_lexeme(GenreLexeme::Z8);
	types_communs[static_cast<long>(TypeBase::Z16)] = cree_type_pour_lexeme(GenreLexeme::Z16);
	types_communs[static_cast<long>(TypeBase::Z32)] = cree_type_pour_lexeme(GenreLexeme::Z32);
	types_communs[static_cast<long>(TypeBase::Z64)] = cree_type_pour_lexeme(GenreLexeme::Z64);
	types_communs[static_cast<long>(TypeBase::R16)] = cree_type_pour_lexeme(GenreLexeme::R16);
	types_communs[static_cast<long>(TypeBase::R32)] = cree_type_pour_lexeme(GenreLexeme::R32);
	types_communs[static_cast<long>(TypeBase::R64)] = cree_type_pour_lexeme(GenreLexeme::R64);
	types_communs[static_cast<long>(TypeBase::EINI)] = type_eini;
	types_communs[static_cast<long>(TypeBase::CHAINE)] = type_chaine;
	types_communs[static_cast<long>(TypeBase::RIEN)] = cree_type_pour_lexeme(GenreLexeme::RIEN);
	types_communs[static_cast<long>(TypeBase::BOOL)] = cree_type_pour_lexeme(GenreLexeme::BOOL);
	types_communs[static_cast<long>(TypeBase::OCTET)] = cree_type_pour_lexeme(GenreLexeme::OCTET);
	types_communs[static_cast<long>(TypeBase::ENTIER_CONSTANT)] = Type::cree_entier_constant();

	for (auto i = static_cast<long>(TypeBase::N8); i <= static_cast<long>(TypeBase::ENTIER_CONSTANT); ++i) {
		if (i == static_cast<long>(TypeBase::EINI) || i == static_cast<long>(TypeBase::CHAINE)) {
			continue;
		}

		types_simples->pousse(types_communs[i]);
	}

	type_type_de_donnees_ = types_type_de_donnees->ajoute_element(nullptr);

	// nous devons créer le pointeur nul avant les autres types, car nous en avons besoin pour définir les opérateurs pour les pointeurs
	auto ptr_nul = types_pointeurs->ajoute_element(nullptr);

	types_communs[static_cast<long>(TypeBase::PTR_NUL)] = ptr_nul;

	for (auto &donnees : donnees_types_communs) {
		auto const idx = static_cast<long>(donnees.val_enum);
		auto type = this->type_pour_lexeme(donnees.dt[1]);

		if (donnees.dt[0] == GenreLexeme::TABLEAU) {
			type = this->type_tableau_dynamique(type);
		}
		else if (donnees.dt[0] == GenreLexeme::POINTEUR) {
			type = this->type_pointeur_pour(type);
		}
		else if (donnees.dt[0] == GenreLexeme::REFERENCE) {
			type = this->type_reference_pour(type);
		}
		else {
			assert_rappel(false, [&]() { std::cerr << "Genre de type non-géré : " << chaine_du_lexeme(donnees.dt[0]) << '\n'; });
		}

		types_communs[idx] = type;
	}

	type_contexte = reserve_type_structure(nullptr);
	type_info_type_ = reserve_type_structure(nullptr);

	auto membres_eini = kuri::tableau<TypeCompose::Membre>();
	membres_eini.pousse({ types_communs[static_cast<long>(TypeBase::PTR_RIEN)], "pointeur", 0 });
	membres_eini.pousse({ type_pointeur_pour(type_info_type_), "info", 8 });
	type_eini->membres = std::move(membres_eini);
	type_eini->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);

	auto membres_chaine = kuri::tableau<TypeCompose::Membre>();
	membres_chaine.pousse({ types_communs[static_cast<long>(TypeBase::PTR_Z8)], "pointeur", 0 });
	membres_chaine.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	type_chaine->membres = std::move(membres_chaine);
	type_chaine->drapeaux |= (TYPE_FUT_VALIDE | RI_TYPE_FUT_GENEREE | TYPE_EST_NORMALISE);

	POUR (types_communs) {
		it->drapeaux |= TYPE_EST_NORMALISE;
	}
}

Typeuse::~Typeuse()
{
#define DELOGE_TYPES(Type, Tableau) \
	for (auto ptr : *Tableau.verrou_lecture()) {\
	memoire::deloge(#Type, ptr); \
}

	DELOGE_TYPES(TypePointeur, types_simples);

	memoire::deloge("TypeCompose", type_eini);
	memoire::deloge("TypeCompose", type_chaine);

#undef DELOGE_TYPES
}

Type *Typeuse::type_pour_lexeme(GenreLexeme lexeme)
{
	switch (lexeme) {
		case GenreLexeme::BOOL:
		{
			return types_communs[static_cast<long>(TypeBase::BOOL)];
		}
		case GenreLexeme::OCTET:
		{
			return types_communs[static_cast<long>(TypeBase::OCTET)];
		}
		case GenreLexeme::N8:
		{
			return types_communs[static_cast<long>(TypeBase::N8)];
		}
		case GenreLexeme::Z8:
		{
			return types_communs[static_cast<long>(TypeBase::Z8)];
		}
		case GenreLexeme::N16:
		{
			return types_communs[static_cast<long>(TypeBase::N16)];
		}
		case GenreLexeme::Z16:
		{
			return types_communs[static_cast<long>(TypeBase::Z16)];
		}
		case GenreLexeme::N32:
		{
			return types_communs[static_cast<long>(TypeBase::N32)];
		}
		case GenreLexeme::Z32:
		{
			return types_communs[static_cast<long>(TypeBase::Z32)];
		}
		case GenreLexeme::N64:
		{
			return types_communs[static_cast<long>(TypeBase::N64)];
		}
		case GenreLexeme::Z64:
		{
			return types_communs[static_cast<long>(TypeBase::Z64)];
		}
		case GenreLexeme::R16:
		{
			return types_communs[static_cast<long>(TypeBase::R16)];
		}
		case GenreLexeme::R32:
		{
			return types_communs[static_cast<long>(TypeBase::R32)];
		}
		case GenreLexeme::R64:
		{
			return types_communs[static_cast<long>(TypeBase::R64)];
		}
		case GenreLexeme::CHAINE:
		{
			return types_communs[static_cast<long>(TypeBase::CHAINE)];
		}
		case GenreLexeme::EINI:
		{
			return types_communs[static_cast<long>(TypeBase::EINI)];
		}
		case GenreLexeme::RIEN:
		{
			return types_communs[static_cast<long>(TypeBase::RIEN)];
		}
		case GenreLexeme::TYPE_DE_DONNEES:
		{
			return type_type_de_donnees_;
		}
		default:
		{
			return nullptr;
		}
	}
}

TypePointeur *Typeuse::type_pointeur_pour(Type *type)
{
	Prof(type_pointeur_pour);

	if (!type) {
		return ((*this)[TypeBase::PTR_NUL])->comme_pointeur();
	}

	auto types_pointeurs_ = types_pointeurs.verrou_ecriture();

	if ((type->drapeaux & POSSEDE_TYPE_POINTEUR) != 0) {
		POUR_TABLEAU_PAGE ((*types_pointeurs_)) {
			if (it.type_pointe == type) {
				return &it;
			}
		}
	}

	auto resultat = types_pointeurs_->ajoute_element(type);

	if (type != nullptr) {
		auto graphe = graphe_.verrou_ecriture();
		graphe->connecte_type_type(resultat, type);
	}

	auto indice = IndiceTypeOp::ENTIER_RELATIF;

	auto const &idx_dt_ptr_nul = types_communs[static_cast<long>(TypeBase::PTR_NUL)];
	auto const &idx_dt_bool = types_communs[static_cast<long>(TypeBase::BOOL)];

	auto operateurs = operateurs_.verrou_ecriture();

	operateurs->ajoute_basique(GenreLexeme::EGALITE, resultat, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::DIFFERENCE, resultat, idx_dt_bool, indice);

	operateurs->ajoute_basique(GenreLexeme::EGALITE, resultat, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::DIFFERENCE, resultat, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::INFERIEUR, resultat, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::INFERIEUR_EGAL, resultat, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::SUPERIEUR, resultat, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::SUPERIEUR_EGAL, resultat, idx_dt_bool, indice);

	/* Pour l'arithmétique de pointeur nous n'utilisons que le type le plus
	 * gros, la résolution de l'opérateur ajoutera une transformation afin
	 * que le type plus petit soit transtyper à la bonne taille. */
	auto idx_type_entier = types_communs[static_cast<long>(TypeBase::Z64)];

	operateurs->ajoute_basique(GenreLexeme::PLUS, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::MOINS, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::MOINS, resultat, resultat, idx_type_entier, indice);
	operateurs->ajoute_basique(GenreLexeme::PLUS_EGAL, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::MOINS_EGAL, resultat, idx_type_entier, resultat, indice);

	idx_type_entier = types_communs[static_cast<long>(TypeBase::N64)];
	indice = IndiceTypeOp::ENTIER_NATUREL;

	operateurs->ajoute_basique(GenreLexeme::PLUS, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::MOINS, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::PLUS_EGAL, resultat, idx_type_entier, resultat, indice);
	operateurs->ajoute_basique(GenreLexeme::MOINS_EGAL, resultat, idx_type_entier, resultat, indice);

	return resultat;
}

TypeReference *Typeuse::type_reference_pour(Type *type)
{
	Prof(type_reference_pour);

	auto types_references_ = types_references.verrou_ecriture();

	if ((type->drapeaux & POSSEDE_TYPE_REFERENCE) != 0) {
		POUR_TABLEAU_PAGE ((*types_references_)) {
			if (it.type_pointe == type) {
				return &it;
			}
		}
	}

	auto resultat = types_references_->ajoute_element(type);

	auto graphe = graphe_.verrou_ecriture();
	graphe->connecte_type_type(resultat, type);

	return resultat;
}

TypeTableauFixe *Typeuse::type_tableau_fixe(Type *type_pointe, long taille)
{
	Prof(type_tableau_fixe);

	auto types_tableaux_fixes_ = types_tableaux_fixes.verrou_ecriture();

	if ((type_pointe->drapeaux & POSSEDE_TYPE_TABLEAU_FIXE) != 0) {
		POUR_TABLEAU_PAGE ((*types_tableaux_fixes_)) {
			if (it.type_pointe == type_pointe && it.taille == taille) {
				return &it;
			}
		}
	}

	// les décalages sont à zéros car ceci n'est pas vraiment une structure
	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 0 });

	auto type = types_tableaux_fixes_->ajoute_element(type_pointe, taille, std::move(membres));

	auto graphe = graphe_.verrou_ecriture();
	graphe->connecte_type_type(type, type_pointe);

	return type;
}

TypeTableauDynamique *Typeuse::type_tableau_dynamique(Type *type_pointe)
{
	Prof(type_tableau_dynamique);

	auto types_tableaux_dynamiques_ = types_tableaux_dynamiques.verrou_ecriture();

	if ((type_pointe->drapeaux & POSSEDE_TYPE_TABLEAU_DYNAMIQUE) != 0) {
		POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques_)) {
			if (it.type_pointe == type_pointe) {
				return &it;
			}
		}
	}

	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "capacité", 16 });

	auto type = types_tableaux_dynamiques_->ajoute_element(type_pointe, std::move(membres));

	auto graphe = graphe_.verrou_ecriture();
	graphe->connecte_type_type(type, type_pointe);

	return type;
}

TypeVariadique *Typeuse::type_variadique(Type *type_pointe)
{
	Prof(type_variadique);

	auto types_variadiques_ = types_variadiques.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*types_variadiques_)) {
		if (it.type_pointe == type_pointe) {
			return &it;
		}
	}

	auto membres = kuri::tableau<TypeCompose::Membre>();
	membres.pousse({ type_pointeur_pour(type_pointe), "pointeur", 0 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "taille", 8 });
	membres.pousse({ types_communs[static_cast<long>(TypeBase::Z64)], "capacité", 16 });

	auto type = types_variadiques_->ajoute_element(type_pointe, std::move(membres));

	if (type_pointe != nullptr) {
		auto graphe = graphe_.verrou_ecriture();
		graphe->connecte_type_type(type, type_pointe);
	}

	return type;
}

TypeFonction *Typeuse::discr_type_fonction(TypeFonction *it, dls::tablet<Type *, 6> const &entrees, dls::tablet<Type *, 6> const &sorties)
{
	if (it->types_entrees.taille != entrees.taille()) {
		return nullptr;
	}

	if (it->types_sorties.taille != sorties.taille()) {
		return nullptr;
	}

	for (int i = 0; i < it->types_entrees.taille; ++i) {
		if (it->types_entrees[i] != entrees[i]) {
			return nullptr;
		}
	}

	for (int i = 0; i < it->types_sorties.taille; ++i) {
		if (it->types_sorties[i] != sorties[i]) {
			return nullptr;
		}
	}

	return it;
}

//static int nombre_types_apparies = 0;
//static int nombre_appels = 0;

TypeFonction *Typeuse::type_fonction(dls::tablet<Type *, 6> const &entrees, dls::tablet<Type *, 6> const &sorties)
{
	Prof(type_fonction);

	//nombre_appels += 1;

	auto types_fonctions_ = types_fonctions.verrou_ecriture();

	uint64_t tag_entrees = 0;
	uint64_t tag_sorties = 0;

	POUR (entrees) {
		tag_entrees |= reinterpret_cast<uint64_t>(it);
	}

	POUR (sorties) {
		tag_sorties |= reinterpret_cast<uint64_t>(it);
	}

	POUR_TABLEAU_PAGE ((*types_fonctions_)) {
		if (it.tag_entrees != tag_entrees || it.tag_sorties != tag_sorties) {
			continue;
		}

		auto type = discr_type_fonction(&it, entrees, sorties);

		if (type != nullptr) {
//			nombre_types_apparies += 1;
//			std::cerr << "appariements : " << nombre_types_apparies << '\n';
//			std::cerr << "appels       : " << nombre_appels << '\n';
			return type;
		}
	}

	auto type = types_fonctions_->ajoute_element(entrees, sorties);
	type->tag_entrees = tag_entrees;
	type->tag_sorties = tag_sorties;

	auto indice = IndiceTypeOp::ENTIER_RELATIF;

	auto const &idx_dt_ptr_nul = types_communs[static_cast<long>(TypeBase::PTR_NUL)];
	auto const &idx_dt_bool = types_communs[static_cast<long>(TypeBase::BOOL)];

	auto operateurs = operateurs_.verrou_ecriture();

	operateurs->ajoute_basique(GenreLexeme::EGALITE, type, idx_dt_ptr_nul, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::DIFFERENCE, type, idx_dt_ptr_nul, idx_dt_bool, indice);

	operateurs->ajoute_basique(GenreLexeme::EGALITE, type, idx_dt_bool, indice);
	operateurs->ajoute_basique(GenreLexeme::DIFFERENCE, type, idx_dt_bool, indice);

	auto graphe = graphe_.verrou_ecriture();

	POUR (type->types_entrees) {
		graphe->connecte_type_type(type, it);
	}

	POUR (type->types_sorties) {
		graphe->connecte_type_type(type, it);
	}

//	std::cerr << "appariements : " << nombre_types_apparies << '\n';
//	std::cerr << "appels       : " << nombre_appels << '\n';
	return type;
}

TypeTypeDeDonnees *Typeuse::type_type_de_donnees(Type *type_connu)
{
	Prof(type_type_de_donnees);

	if (type_connu == nullptr) {
		return type_type_de_donnees_;
	}

	auto types_type_de_donnees_ = types_type_de_donnees.verrou_ecriture();

	if ((type_connu->drapeaux & POSSEDE_TYPE_TYPE_DE_DONNEES) != 0) {
		POUR_TABLEAU_PAGE ((*types_type_de_donnees_)) {
			if (it.type_connu == type_connu) {
				return &it;
			}
		}
	}

	return types_type_de_donnees_->ajoute_element(type_connu);
}

TypeStructure *Typeuse::reserve_type_structure(NoeudStruct *decl)
{
	auto type = types_structures->ajoute_element();
	type->decl = decl;

	// decl peut être nulle pour la réservation du type pour InfoType
	if (type->decl) {
		type->nom = decl->lexeme->chaine;
	}

	return type;
}

TypeEnum *Typeuse::reserve_type_enum(NoeudEnum *decl)
{
	auto type = types_enums->ajoute_element();
	type->nom = decl->lexeme->chaine;
	type->decl = decl;
	type->drapeaux |= (RI_TYPE_FUT_GENEREE);

	return type;
}

TypeUnion *Typeuse::reserve_type_union(NoeudStruct *decl)
{
	auto type = types_unions->ajoute_element();
	type->nom = decl->lexeme->chaine;
	type->decl = decl;

	return type;
}

TypeUnion *Typeuse::union_anonyme(const dls::tablet<TypeCompose::Membre, 6> &membres)
{
	Prof(union_anonyme);

	auto types_unions_ = types_unions.verrou_ecriture();

	POUR_TABLEAU_PAGE ((*types_unions_)) {
		if (!it.est_anonyme) {
			continue;
		}

		if (it.membres.taille != membres.taille()) {
			continue;
		}

		auto type_apparie = true;

		for (auto i = 0; i < it.membres.taille; ++i) {
			if (it.membres[i].type != membres[i].type) {
				type_apparie = false;
				break;
			}
		}

		if (type_apparie) {
			return &it;
		}
	}

	auto type = types_unions_->ajoute_element();
	type->nom = "anonyme";

	type->membres.reserve(membres.taille());
	POUR (membres) {
		type->membres.pousse(it);
	}

	type->est_anonyme = true;
	type->drapeaux |= (TYPE_FUT_VALIDE);

	calcule_taille_type_compose(type);

	type->cree_type_structure(*this, type->decalage_index);

	return type;
}

TypeEnum *Typeuse::reserve_type_erreur(NoeudEnum *decl)
{
	auto type = reserve_type_enum(decl);
	type->genre = GenreType::ERREUR;

	return type;
}

TypePolymorphique *Typeuse::cree_polymorphique(IdentifiantCode *ident)
{
	Prof(cree_polymorphique);

	auto types_polymorphiques_ = types_polymorphiques.verrou_ecriture();

	// pour le moment un ident nul est utilisé pour les types polymorphiques des
	// structures dans les types des fonction (foo :: fonc (p: Polymorphe(T = $T)),
	// donc ne déduplique pas ces types pour éviter les problèmes quand nous validons
	// ces expresssions, car les données associées doivent être spécifiques à chaque
	// déclaration
	if (ident) {
		POUR_TABLEAU_PAGE ((*types_polymorphiques_)) {
			if (it.ident == ident) {
				return &it;
			}
		}
	}

	return types_polymorphiques_->ajoute_element(ident);
}

TypeOpaque *Typeuse::cree_opaque(IdentifiantCode *ident, Type *type_opacifie)
{
	return types_opaques->ajoute_element(ident, type_opacifie);
}

void Typeuse::rassemble_statistiques(Statistiques &stats) const
{
#define DONNEES_ENTREE(Type, Tableau) \
	#Type, Tableau->taille(), Tableau->taille() * (taille_de(Type *) + taille_de(Type))

	auto &stats_types = stats.stats_types;

	stats_types.fusionne_entree({ DONNEES_ENTREE(Type, types_simples) });
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypePointeur, types_pointeurs) });
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeReference, types_references) });
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeTypeDeDonnees, types_type_de_donnees) });
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypePolymorphique, types_polymorphiques) });

	auto memoire_membres_structures = 0l;
	POUR_TABLEAU_PAGE ((*types_structures.verrou_lecture())) {
		memoire_membres_structures += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeStructure, types_structures) + memoire_membres_structures });

	auto memoire_membres_enums = 0l;
	POUR_TABLEAU_PAGE ((*types_enums.verrou_lecture())) {
		memoire_membres_enums += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeEnum, types_enums) + memoire_membres_enums });

	auto memoire_membres_unions = 0l;
	POUR_TABLEAU_PAGE ((*types_unions.verrou_lecture())) {
		memoire_membres_unions += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeUnion, types_unions) + memoire_membres_unions });

	auto memoire_membres_tfixes = 0l;
	POUR_TABLEAU_PAGE ((*types_tableaux_fixes.verrou_lecture())) {
		memoire_membres_tfixes += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeTableauFixe, types_tableaux_fixes) + memoire_membres_tfixes });

	auto memoire_membres_tdyns = 0l;
	POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques.verrou_lecture())) {
		memoire_membres_tdyns += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeTableauDynamique, types_tableaux_dynamiques) + memoire_membres_tdyns });

	auto memoire_membres_tvars = 0l;
	POUR_TABLEAU_PAGE ((*types_variadiques.verrou_lecture())) {
		memoire_membres_tvars += it.membres.taille * taille_de(TypeCompose::Membre);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeVariadique, types_variadiques) + memoire_membres_tvars });

	auto memoire_params_fonctions = 0l;
	POUR_TABLEAU_PAGE ((*types_fonctions.verrou_lecture())) {
		memoire_params_fonctions += it.types_entrees.taille * taille_de(Type *);
		memoire_params_fonctions += it.types_sorties.taille * taille_de(Type *);
	}
	stats_types.fusionne_entree({ DONNEES_ENTREE(TypeFonction, types_fonctions) + memoire_params_fonctions });

	// les types communs sont dans les types simples, ne comptons que la mémoire du tableau
	stats_types.fusionne_entree({ "TypeCommun", types_communs.taille(), types_communs.taille() * taille_de(Type *) });

	stats_types.fusionne_entree({ "eini", 1, taille_de(TypeCompose) + taille_de(TypeCompose *) + type_eini->membres.taille * taille_de(TypeCompose::Membre) });
	stats_types.fusionne_entree({ "chaine", 1, taille_de(TypeCompose) + taille_de(TypeCompose *) + type_chaine->membres.taille * taille_de(TypeCompose::Membre) });

#undef DONNES_ENTREE
}

void Typeuse::construit_table_types()
{
	/* À FAIRE(table type) : idéalement nous devrions générer une table de type uniquement pour les types utilisés
	 * dans le programme final (ignorant les types générés par la constructrice, comme les pointeurs pour les arguments).
	 * Pour ce faire, nous ne devrions assigner un index qu'à la fin de la génération de code, mais celui-ci requiers les
	 * index pour les expressions sur les types. Nous devrions peut-être avoir un système de patch où nous rassemblons
	 * les différentes instructions utilisant les index des types pour les ajourner avec le bon index à la fin de la compilation.
	 */

#define ASSIGNE_INDEX(type) \
	if (type->index_dans_table_types == 0u) type->index_dans_table_types = index_type++

	auto index_type = 1u;
	ASSIGNE_INDEX(type_type_de_donnees_);
	ASSIGNE_INDEX(type_chaine);
	ASSIGNE_INDEX(type_eini);
	POUR (*types_simples.verrou_ecriture()) { ASSIGNE_INDEX(it); }
	POUR_TABLEAU_PAGE ((*types_pointeurs.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_references.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_structures.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_enums.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_tableaux_fixes.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_fonctions.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_variadiques.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_unions.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
	POUR_TABLEAU_PAGE ((*types_opaques.verrou_ecriture())) { ASSIGNE_INDEX((&it)); }
}

/* ************************************************************************** */

dls::chaine chaine_type(const Type *type)
{
	if (type == nullptr) {
		return "nul";
	}

	switch (type->genre) {
		case GenreType::EINI:
		{
			return "eini";
		}
		case GenreType::CHAINE:
		{
			return "chaine";
		}
		case GenreType::RIEN:
		{
			return "rien";
		}
		case GenreType::BOOL:
		{
			return "bool";
		}
		case GenreType::OCTET:
		{
			return "octet";
		}
		case GenreType::ENTIER_CONSTANT:
		{
			return "entier_constant";
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				return "n8";
			}

			if (type->taille_octet == 2) {
				return "n16";
			}

			if (type->taille_octet == 4) {
				return "n32";
			}

			if (type->taille_octet == 8) {
				return "n64";
			}

			return "invalide";
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				return "z8";
			}

			if (type->taille_octet == 2) {
				return "z16";
			}

			if (type->taille_octet == 4) {
				return "z32";
			}

			if (type->taille_octet == 8) {
				return "z64";
			}

			return "invalide";
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return "r16";
			}

			if (type->taille_octet == 4) {
				return "r32";
			}

			if (type->taille_octet == 8) {
				return "r64";
			}

			return "invalide";
		}
		case GenreType::REFERENCE:
		{
			return "&" + chaine_type(static_cast<TypeReference const *>(type)->type_pointe);
		}
		case GenreType::POINTEUR:
		{
			return "*" + chaine_type(static_cast<TypePointeur const *>(type)->type_pointe);
		}
		case GenreType::UNION:
		{
			return static_cast<TypeUnion const *>(type)->nom;
		}
		case GenreType::STRUCTURE:
		{
			return static_cast<TypeStructure const *>(type)->nom;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			return "[]" + chaine_type(static_cast<TypeTableauDynamique const *>(type)->type_pointe);
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = static_cast<TypeTableauFixe const *>(type);

			auto res = dls::chaine("[");
			res += dls::vers_chaine(type_tabl->taille);
			res += "]";

			return res + chaine_type(type_tabl->type_pointe);
		}
		case GenreType::VARIADIQUE:
		{
			return "..." + chaine_type(static_cast<TypeVariadique const *>(type)->type_pointe);
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction const *>(type);

			auto res = dls::chaine("fonc");

			auto virgule = '(';

			POUR (type_fonc->types_entrees) {
				res += virgule;
				res += chaine_type(it);
				virgule = ',';
			}

			if (type_fonc->types_entrees.taille == 0) {
				res += virgule;
			}

			res += ')';

			virgule = '(';

			POUR (type_fonc->types_sorties) {
				res += virgule;
				res += chaine_type(it);
				virgule = ',';
			}

			res += ')';

			return res;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			return static_cast<TypeEnum const *>(type)->nom;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			return "type_de_données";
		}
		case GenreType::POLYMORPHIQUE:
		{
			auto type_polymorphique = static_cast<TypePolymorphique const *>(type);
			auto res = dls::chaine("$");

			if (type_polymorphique->est_structure_poly) {
				return res + type_polymorphique->structure->ident->nom;
			}

			return res + type_polymorphique->ident->nom;
		}
		case GenreType::OPAQUE:
		{
			return static_cast<TypeOpaque const *>(type)->ident->nom;
		}
	}

	return "";
}

Type *type_dereference_pour(Type *type)
{
	Prof(type_dereference_pour);

	if (type->genre == GenreType::TABLEAU_FIXE) {
		return type->comme_tableau_fixe()->type_pointe;
	}

	if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
		return type->comme_tableau_dynamique()->type_pointe;
	}

	if (type->genre == GenreType::POINTEUR) {
		return type->comme_pointeur()->type_pointe;
	}

	if (type->genre == GenreType::REFERENCE) {
		return type->comme_reference()->type_pointe;
	}

	if (type->genre == GenreType::VARIADIQUE) {
		return type->comme_variadique()->type_pointe;
	}

	return nullptr;
}

void rassemble_noms_type_polymorphique(Type *type, kuri::tableau<dls::vue_chaine_compacte> &noms)
{
	if (type->genre == GenreType::FONCTION) {
		auto type_fonction = type->comme_fonction();

		POUR (type_fonction->types_entrees) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				rassemble_noms_type_polymorphique(it, noms);
			}
		}

		POUR (type_fonction->types_sorties) {
			if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
				rassemble_noms_type_polymorphique(it, noms);
			}
		}

		return;
	}

	while (type->genre != GenreType::POLYMORPHIQUE) {
		type = type_dereference_pour(type);
	}

	noms.pousse(type->comme_polymorphique()->ident->nom);
}

bool est_type_conditionnable(Type *type)
{
	return dls::outils::est_element(
				type->genre,
				GenreType::BOOL,
				GenreType::CHAINE,
				GenreType::ENTIER_CONSTANT,
				GenreType::ENTIER_NATUREL,
				GenreType::ENTIER_RELATIF,
				GenreType::FONCTION,
				GenreType::POINTEUR,
				GenreType::TABLEAU_DYNAMIQUE);
}

void TypeUnion::cree_type_structure(Typeuse &typeuse, unsigned alignement_membre_actif)
{
	assert(type_le_plus_grand);
	assert(!est_nonsure);

	auto membres_ = kuri::tableau<TypeCompose::Membre>(2);
	membres_[0] = { type_le_plus_grand, "valeur", 0 };
	membres_[1] = { typeuse[TypeBase::Z32], "membre_actif", alignement_membre_actif };

	type_structure = typeuse.reserve_type_structure(nullptr);
	type_structure->membres = std::move(membres_);
	type_structure->taille_octet = this->taille_octet;
	type_structure->alignement = this->alignement;
	type_structure->nom = this->nom;
	type_structure->est_anonyme = this->est_anonyme;
}

/* Pour la génération de RI, les types doivent être normalisés afin de se rapprocher de la manière dont ceux-ci sont « représenter » dans la machine.
 * Ceci se fait en :
 * - remplaçant les références par des pointeurs
 * - convertissant les unions en leurs « types machines » : une structure pour les unions sûres, le type le plus grand pour les sûres
 */
Type *normalise_type(Typeuse &typeuse, Type *type)
{
	if (type == nullptr) {
		return type;
	}

	auto resultat = type;

	if (type->genre == GenreType::UNION) {
		auto type_union = type->comme_union();

		if (type_union->est_nonsure) {
			resultat = type_union->type_le_plus_grand;
		}
		else {
			resultat = type_union->type_structure;
		}
	}
	else if (type->genre == GenreType::TABLEAU_FIXE) {
		auto type_tableau_fixe = type->comme_tableau_fixe();
		resultat = normalise_type(typeuse, type_tableau_fixe->type_pointe);
		resultat = typeuse.type_tableau_fixe(type_tableau_fixe->type_pointe, type_tableau_fixe->taille);
	}
	else if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
		auto type_tableau_dyn = type->comme_tableau_dynamique();
		auto type_normalise = normalise_type(typeuse, type_tableau_dyn->type_pointe);

		if (type_normalise != type_tableau_dyn->type_pointe) {
			resultat = typeuse.type_tableau_dynamique(type_tableau_dyn->type_pointe);
		}
	}
	else if (type->genre == GenreType::VARIADIQUE) {
		auto type_variadique = type->comme_variadique();
		auto type_normalise = normalise_type(typeuse, type_variadique->type_pointe);

		if (type_normalise != type_variadique) {
			resultat = typeuse.type_variadique(type_variadique->type_pointe);
		}
	}
	else if (type->genre == GenreType::POINTEUR) {
		auto type_pointeur = type->comme_pointeur();
		auto type_normalise = normalise_type(typeuse, type_pointeur->type_pointe);

		if (type_normalise != type_pointeur) {
			resultat = typeuse.type_pointeur_pour(type_pointeur->type_pointe);
		}
	}
	else if (type->genre == GenreType::REFERENCE) {
		auto type_reference = type->comme_reference();
		auto type_normalise = normalise_type(typeuse, type_reference->type_pointe);
		resultat = typeuse.type_pointeur_pour(type_normalise);
	}
	else if (type->genre == GenreType::FONCTION) {
		auto type_fonction = type->comme_fonction();

		auto types_entrees = dls::tablet<Type *, 6>();
		types_entrees.reserve(type_fonction->types_entrees.taille);

		POUR (type_fonction->types_entrees) {
			types_entrees.pousse(normalise_type(typeuse, it));
		}

		auto types_sorties = dls::tablet<Type *, 6>();
		types_sorties.reserve(type_fonction->types_entrees.taille);

		POUR (type_fonction->types_sorties) {
			types_sorties.pousse(normalise_type(typeuse, it));
		}

		resultat = typeuse.type_fonction(types_entrees, types_sorties);
	}

	resultat->drapeaux |= TYPE_EST_NORMALISE;

	return resultat;
}

void calcule_taille_type_compose(TypeCompose *type)
{
	if (type->genre == GenreType::UNION) {
		auto type_union = type->comme_union();

		auto max_alignement = 0u;
		auto taille_union = 0u;
		auto type_le_plus_grand = Type::nul();

		POUR (type->membres) {
			if (it.drapeaux & TypeStructure::Membre::EST_CONSTANT) {
				continue;
			}

			auto type_membre = it.type;
			auto taille = type_membre->taille_octet;
			max_alignement = std::max(taille, max_alignement);

			if (taille > taille_union) {
				type_le_plus_grand = type_membre;
				taille_union = taille;
			}
		}

		/* Pour les unions sûres, il nous faut prendre en compte le
		 * membre supplémentaire. */
		if (!type_union->est_nonsure) {
			/* ajoute une marge d'alignement */
			auto padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
			taille_union += padding;

			type_union->decalage_index = taille_union;

			/* ajoute la taille du membre actif */
			taille_union += static_cast<unsigned>(taille_de(int));

			/* ajoute une marge d'alignement finale */
			padding = (max_alignement - (taille_union % max_alignement)) % max_alignement;
			taille_union += padding;
		}

		type_union->type_le_plus_grand = type_le_plus_grand;
		type_union->taille_octet = taille_union;
		type_union->alignement = max_alignement;
	}
	else if (type->genre == GenreType::STRUCTURE) {
		auto decalage = 0u;
		auto max_alignement = 0u;

		POUR (type->membres) {
			if (it.drapeaux & TypeStructure::Membre::EST_CONSTANT) {
				continue;
			}

			auto align_type = it.type->alignement;
			max_alignement = std::max(align_type, max_alignement);

			auto padding = (align_type - (decalage % align_type)) % align_type;
			decalage += padding;

			it.decalage = decalage;

			decalage += it.type->taille_octet;
		}

		auto padding = (max_alignement - (decalage % max_alignement)) % max_alignement;
		decalage += padding;

		type->taille_octet = decalage;
		type->alignement = max_alignement;
	}
}

static dls::chaine nom_portable(NoeudBloc *bloc, dls::vue_chaine_compacte nom)
{
	auto resultat = dls::chaine();

	while (bloc) {
		if (bloc->ident) {
			resultat = bloc->ident->nom + resultat;
		}

		bloc = bloc->bloc_parent;
	}

	resultat += nom;

	return resultat;
}

const dls::chaine &TypeStructure::nom_portable()
{
	if (nom_portable_ != "") {
		return nom_portable_;
	}

	nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom);
	return nom_portable_;
}

const dls::chaine &TypeUnion::nom_portable()
{
	if (nom_portable_ != "") {
		return nom_portable_;
	}

	nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom);
	return nom_portable_;
}

const dls::chaine &TypeEnum::nom_portable()
{
	if (nom_portable_ != "") {
		return nom_portable_;
	}

	nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom);
	return nom_portable_;
}

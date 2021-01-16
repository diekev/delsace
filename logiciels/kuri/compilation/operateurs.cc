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

#include "operateurs.hh"

#include "biblinternes/structures/dico_fixe.hh"

#include "compilatrice.hh"
#include "lexemes.hh"
#include "statistiques.hh"

static OperateurBinaire::Genre genre_op_binaire_pour_lexeme(
		GenreLexeme genre_lexeme,
		IndiceTypeOp type_operandes)
{
	switch (genre_lexeme) {
		case GenreLexeme::PLUS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Addition_Reel;
			}

			return OperateurBinaire::Genre::Addition;
		}
		case GenreLexeme::MOINS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Soustraction_Reel;
			}

			return OperateurBinaire::Genre::Soustraction;
		}
		case GenreLexeme::FOIS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Multiplication_Reel;
			}

			return OperateurBinaire::Genre::Multiplication;
		}
		case GenreLexeme::DIVISE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Division_Reel;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Division_Naturel;
			}

			return OperateurBinaire::Genre::Division_Relatif;
		}
		case GenreLexeme::POURCENT:
		{			
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Reste_Naturel;
			}

			return OperateurBinaire::Genre::Reste_Relatif;
		}
		case GenreLexeme::DECALAGE_DROITE:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Dec_Droite_Logique;
			}

			return OperateurBinaire::Genre::Dec_Droite_Arithm;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		{
			return OperateurBinaire::Genre::Dec_Gauche;
		}
		case GenreLexeme::ESPERLUETTE:
		{
			return OperateurBinaire::Genre::Et_Binaire;
		}
		case GenreLexeme::BARRE:
		{
			return OperateurBinaire::Genre::Ou_Binaire;
		}
		case GenreLexeme::CHAPEAU:
		{
			return OperateurBinaire::Genre::Ou_Exclusif;
		}
		case GenreLexeme::INFERIEUR:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Inf_Reel;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Comp_Inf_Nat;
			}

			return OperateurBinaire::Genre::Comp_Inf;
		}
		case GenreLexeme::INFERIEUR_EGAL:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Inf_Egal_Reel;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Comp_Inf_Egal_Nat;
			}

			return OperateurBinaire::Genre::Comp_Inf_Egal;
		}
		case GenreLexeme::SUPERIEUR:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Sup_Reel;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Comp_Sup_Nat;
			}

			return OperateurBinaire::Genre::Comp_Sup;
		}
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Sup_Egal_Reel;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return OperateurBinaire::Genre::Comp_Sup_Egal_Nat;
			}

			return OperateurBinaire::Genre::Comp_Sup_Egal;
		}
		case GenreLexeme::EGALITE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Egal_Reel;
			}
			return OperateurBinaire::Genre::Comp_Egal;
		}
		case GenreLexeme::DIFFERENCE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return OperateurBinaire::Genre::Comp_Inegal_Reel;
			}
			return OperateurBinaire::Genre::Comp_Inegal;
		}
		case GenreLexeme::CROCHET_OUVRANT:
		{
			return OperateurBinaire::Genre::Indexage;
		}
		default:
		{
			return OperateurBinaire::Genre::Invalide;
		}
	}
}

static OperateurUnaire::Genre genre_op_unaire_pour_lexeme(GenreLexeme genre_lexeme)
{
	switch (genre_lexeme) {
		case GenreLexeme::PLUS_UNAIRE:
		{
			return OperateurUnaire::Genre::Positif;
		}
		case GenreLexeme::MOINS_UNAIRE:
		{
			return OperateurUnaire::Genre::Complement;
		}
		case GenreLexeme::TILDE:
		{
			return OperateurUnaire::Genre::Non_Binaire;
		}
		case GenreLexeme::EXCLAMATION:
		{
			return OperateurUnaire::Genre::Non_Logique;
		}
		default:
		{
			return OperateurUnaire::Genre::Invalide;
		}
	}
}

// types comparaisons :
// ==, !=, <, >, <=, =>
static GenreLexeme operateurs_comparaisons[] = {
	GenreLexeme::EGALITE,
	GenreLexeme::DIFFERENCE,
	GenreLexeme::INFERIEUR,
	GenreLexeme::SUPERIEUR,
	GenreLexeme::INFERIEUR_EGAL,
	GenreLexeme::SUPERIEUR_EGAL
};

// types entiers et réels :
// +, -, *, / (assignés +=, -=, /=, *=)
static GenreLexeme operateurs_entiers_reels[] = {
	GenreLexeme::PLUS,
	GenreLexeme::MOINS,
	GenreLexeme::FOIS,
	GenreLexeme::DIVISE,
};

// types entiers :
// %, <<, >>, &, |, ^ (assignés %=, <<=, >>=, &, |, ^)
static GenreLexeme operateurs_entiers[] = {
	GenreLexeme::POURCENT,
	GenreLexeme::DECALAGE_GAUCHE,
	GenreLexeme::DECALAGE_DROITE,
	GenreLexeme::ESPERLUETTE,
	GenreLexeme::BARRE,
	GenreLexeme::CHAPEAU,
	GenreLexeme::TILDE
};

static bool est_commutatif(GenreLexeme id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case GenreLexeme::PLUS:
		case GenreLexeme::FOIS:
		case GenreLexeme::EGALITE:
		case GenreLexeme::DIFFERENCE:
		{
			return true;
		}
	}
}

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre)
{
	switch (genre) {
		case OperateurBinaire::Genre::Addition:
		{
			return "ajt";
		}
		case OperateurBinaire::Genre::Addition_Reel:
		{
			return "ajtr";
		}
		case OperateurBinaire::Genre::Soustraction:
		{
			return "sst";
		}
		case OperateurBinaire::Genre::Soustraction_Reel:
		{
			return "sstr";
		}
		case OperateurBinaire::Genre::Multiplication:
		{
			return "mul";
		}
		case OperateurBinaire::Genre::Multiplication_Reel:
		{
			return "mulr";
		}
		case OperateurBinaire::Genre::Division_Naturel:
		{
			return "divn";
		}
		case OperateurBinaire::Genre::Division_Relatif:
		{
			return "divz";
		}
		case OperateurBinaire::Genre::Division_Reel:
		{
			return "divr";
		}
		case OperateurBinaire::Genre::Reste_Naturel:
		{
			return "modn";
		}
		case OperateurBinaire::Genre::Reste_Relatif:
		{
			return "modz";
		}
		case OperateurBinaire::Genre::Comp_Egal:
		{
			return "eg";
		}
		case OperateurBinaire::Genre::Comp_Inegal:
		{
			return "neg";
		}
		case OperateurBinaire::Genre::Comp_Inf:
		{
			return "inf";
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal:
		{
			return "infeg";
		}
		case OperateurBinaire::Genre::Comp_Sup:
		{
			return "sup";
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal:
		{
			return "supeg";
		}
		case OperateurBinaire::Genre::Comp_Inf_Nat:
		{
			return "infn";
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal_Nat:
		{
			return "infegn";
		}
		case OperateurBinaire::Genre::Comp_Sup_Nat:
		{
			return "supn";
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal_Nat:
		{
			return "supegn";
		}
		case OperateurBinaire::Genre::Comp_Egal_Reel:
		{
			return "egr";
		}
		case OperateurBinaire::Genre::Comp_Inegal_Reel:
		{
			return "negr";
		}
		case OperateurBinaire::Genre::Comp_Inf_Reel:
		{
			return "infr";
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal_Reel:
		{
			return "infegr";
		}
		case OperateurBinaire::Genre::Comp_Sup_Reel:
		{
			return "supr";
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal_Reel:
		{
			return "supegr";
		}
		case OperateurBinaire::Genre::Et_Binaire:
		{
			return "et";
		}
		case OperateurBinaire::Genre::Ou_Binaire:
		{
			return "ou";
		}
		case OperateurBinaire::Genre::Ou_Exclusif:
		{
			return "oux";
		}
		case OperateurBinaire::Genre::Dec_Gauche:
		{
			return "decg";
		}
		case OperateurBinaire::Genre::Dec_Droite_Arithm:
		{
			return "decda";
		}
		case OperateurBinaire::Genre::Dec_Droite_Logique:
		{
			return "decdl";
		}
		case OperateurBinaire::Genre::Indexage:
		case OperateurBinaire::Genre::Invalide:
		{
			return "invalide";
		}
	}

	return "inconnu";
}

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre)
{
	switch (genre) {
		case OperateurUnaire::Genre::Positif:
		{
			return "plus";
		}
		case OperateurUnaire::Genre::Complement:
		{
			return "moins";
		}
		case OperateurUnaire::Genre::Non_Logique:
		{
			return "non";
		}
		case OperateurUnaire::Genre::Non_Binaire:
		{
			return "non";
		}
		case OperateurUnaire::Genre::Prise_Adresse:
		{
			return "addr";
		}
		case OperateurUnaire::Genre::Invalide:
		{
			return "invalide";
		}
	}

	return "inconnu";
}

inline int index_op_binaire(GenreLexeme lexeme)
{
	// À FAIRE: l'indice n'est pas bon, nous devrions utiliser le genre pour le bon type de données
	return static_cast<int>(genre_op_binaire_pour_lexeme(lexeme, IndiceTypeOp::ENTIER_NATUREL));
}

inline int index_op_unaire(GenreLexeme lexeme)
{
	return static_cast<int>(genre_op_unaire_pour_lexeme(lexeme));
}

constexpr inline int nombre_genre_op_binaires()
{
	int compte = 0;
#define ENUMERE_GENRE_OPBINAIRE_EX(x) compte += 1;
	ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
	return compte;
}

constexpr inline int nombre_genre_op_unaires()
{
	int compte = 0;
#define ENUMERE_GENRE_OPUNAIRE_EX(x) compte += 1;
	ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
	return compte;
}

Operateurs::Operateurs()
{
	operateurs_unaires.redimensionne(nombre_genre_op_unaires());
	operateurs_binaires.redimensionne(nombre_genre_op_binaires());
}

Operateurs::~Operateurs()
{
}

const Operateurs::type_conteneur_unaire &Operateurs::trouve_unaire(GenreLexeme id) const
{
	return operateurs_unaires[index_op_unaire(id)];
}

const Operateurs::type_conteneur_binaire &Operateurs::trouve_binaire(GenreLexeme id) const
{
	return operateurs_binaires[index_op_binaire(id)];
}

OperateurBinaire *Operateurs::ajoute_basique(
		GenreLexeme id,
		Type *type,
		Type *type_resultat,
		IndiceTypeOp indice_type)
{
	return ajoute_basique(id, type, type, type_resultat, indice_type);
}

OperateurBinaire *Operateurs::ajoute_basique(
		GenreLexeme id,
		Type *type1,
		Type *type2,
		Type *type_resultat,
		IndiceTypeOp indice_type)
{
	assert(type1);
	assert(type2);

	auto op = operateurs_binaires[index_op_binaire(id)].ajoute_element();
	op->type1 = type1;
	op->type2 = type2;
	op->type_resultat = type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = true;
	op->genre = genre_op_binaire_pour_lexeme(id, indice_type);
	return op;
}

void Operateurs::ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat)
{
	auto op = operateurs_unaires[index_op_unaire(id)].ajoute_element();
	op->type_operande = type;
	op->type_resultat = type_resultat;
	op->est_basique = true;
	op->genre = genre_op_unaire_pour_lexeme(id);
}

void Operateurs::ajoute_perso(
		GenreLexeme id,
		Type *type1,
		Type *type2,
		Type *type_resultat,
		NoeudDeclarationEnteteFonction *decl)
{
	auto op = operateurs_binaires[index_op_binaire(id)].ajoute_element();
	op->type1 = type1;
	op->type2 = type2;
	op->type_resultat = type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = false;
	op->decl = decl;
}

void Operateurs::ajoute_perso_unaire(
		GenreLexeme id,
		Type *type,
		Type *type_resultat,
		NoeudDeclarationEnteteFonction *decl)
{
	auto op = operateurs_unaires[index_op_unaire(id)].ajoute_element();
	op->type_operande = type;
	op->type_resultat = type_resultat;
	op->est_basique = false;
	op->decl = decl;
	op->genre = genre_op_unaire_pour_lexeme(id);
}

void Operateurs::ajoute_operateur_basique_enum(TypeEnum *type)
{
	auto indice_type_op = IndiceTypeOp();
	if (type->type_donnees->est_entier_naturel()) {
		indice_type_op = IndiceTypeOp::ENTIER_NATUREL;
	}
	else {
		indice_type_op = IndiceTypeOp::ENTIER_RELATIF;
	}

	for (auto op : operateurs_comparaisons) {
		auto op_bin = this->ajoute_basique(op, type, type_bool, indice_type_op);

		if (op == GenreLexeme::EGALITE) {
			type->operateur_egt = op_bin;
		}
	}

	for (auto op : operateurs_entiers) {
		this->ajoute_basique(op, type, type, indice_type_op);
	}

	this->ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
}

void Operateurs::rassemble_statistiques(Statistiques &stats) const
{
	auto nombre_unaires = 0l;
	auto memoire_unaires = operateurs_unaires.taille * (taille_de(type_conteneur_unaire));

	POUR (operateurs_unaires) {
		memoire_unaires += it.memoire_utilisee();
		nombre_unaires += it.taille();
	}

	auto nombre_binaires = 0l;
	auto memoire_binaires = operateurs_binaires.taille * (taille_de(type_conteneur_binaire));

	POUR (operateurs_binaires) {
		memoire_binaires += it.memoire_utilisee();
		nombre_binaires += it.taille();
	}

	auto &stats_ops = stats.stats_operateurs;
	stats_ops.fusionne_entree({ "OperateurUnaire", nombre_unaires, memoire_unaires });
	stats_ops.fusionne_entree({ "OperateurBinaire", nombre_binaires, memoire_binaires });
}

static std::pair<bool, double> verifie_compatibilite(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_arg,
		Type *type_enf,
		TransformationType &transformation)
{
	if (cherche_transformation(espace, contexte, type_enf, type_arg, transformation)) {
		return { true, 0.0 };
	}

	if (transformation.type == TypeTransformation::INUTILE) {
		/* ne convertissons pas implicitement vers *nul quand nous avons une opérande */
		if (type_arg->est_pointeur() && type_arg->comme_pointeur()->type_pointe == nullptr && type_arg != type_enf) {
			return { false, 0.0 };
		}

		return { false, 1.0 };
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return { false, 0.0 };
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return { false, 0.5 };
}

bool cherche_candidats_operateurs(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type1,
		Type *type2,
		GenreLexeme type_op,
		dls::tablet<OperateurCandidat, 10> &candidats)
{
	assert(type1);
	assert(type2);

	auto op_candidats = dls::tablet<OperateurBinaire const *, 10>();

	auto &iter = espace.operateurs->trouve_binaire(type_op);

	for (auto i = 0; i < iter.taille(); ++i) {
		auto op = &iter[i];

		if (op->type1 == type1 && op->type2 == type2) {
			op_candidats.efface();
			op_candidats.ajoute(op);
			break;
		}

		if (op->type1 == type1 || op->type2 == type2) {
			op_candidats.ajoute(op);
		}
		else if (op->est_commutatif && (op->type2 == type1 || op->type1 == type2)) {
			op_candidats.ajoute(op);
		}
	}

	for (auto const op : op_candidats) {
		auto seq1 = TransformationType{};
		auto seq2 = TransformationType{};

		auto [erreur_dep1, poids1] = verifie_compatibilite(espace, contexte, op->type1, type1, seq1);

		if (erreur_dep1) {
			return true;
		}

		auto [erreur_dep2, poids2] = verifie_compatibilite(espace, contexte, op->type2, type2, seq2);

		if (erreur_dep2) {
			return true;
		}

		auto poids = poids1 * poids2;

		if (poids != 0.0) {
			auto candidat = OperateurCandidat{};
			candidat.op = op;
			candidat.poids = poids;
			candidat.transformation_type1 = seq1;
			candidat.transformation_type2 = seq2;

			candidats.ajoute(candidat);
		}

		if (op->est_commutatif && poids != 1.0) {
			auto [erreur_dep3, poids3] = verifie_compatibilite(espace, contexte, op->type1, type2, seq2);

			if (erreur_dep3) {
				return true;
			}

			auto [erreur_dep4, poids4] = verifie_compatibilite(espace, contexte, op->type2, type1, seq1);

			if (erreur_dep4) {
				return true;
			}

			poids = poids3 * poids4;

			if (poids != 0.0) {
				auto candidat = OperateurCandidat{};
				candidat.op = op;
				candidat.poids = poids;
				candidat.transformation_type1 = seq1;
				candidat.transformation_type2 = seq2;
				candidat.permute_operandes = true;

				candidats.ajoute(candidat);
			}
		}
	}

	return false;
}

const OperateurUnaire *cherche_operateur_unaire(
		Operateurs const &operateurs,
		Type *type1,
		GenreLexeme type_op)
{
	auto &iter = operateurs.trouve_unaire(type_op);

	for (auto i = 0; i < iter.taille(); ++i) {
		auto op = &iter[i];

		if (op->type_operande == type1) {
			return op;
		}
	}

	return nullptr;
}

void enregistre_operateurs_basiques(
		EspaceDeTravail &espace,
		Operateurs &operateurs)
{
	Type *types_entiers_naturels[] = {
		espace.typeuse[TypeBase::N8],
		espace.typeuse[TypeBase::N16],
		espace.typeuse[TypeBase::N32],
		espace.typeuse[TypeBase::N64],
	};

	Type *types_entiers_relatifs[] = {
		espace.typeuse[TypeBase::Z8],
		espace.typeuse[TypeBase::Z16],
		espace.typeuse[TypeBase::Z32],
		espace.typeuse[TypeBase::Z64],
	};

	auto type_r32 = espace.typeuse[TypeBase::R32];
	auto type_r64 = espace.typeuse[TypeBase::R64];

	Type *types_reels[] = {
		type_r32, type_r64
	};

	auto type_entier_constant = espace.typeuse[TypeBase::ENTIER_CONSTANT];
	auto type_octet = espace.typeuse[TypeBase::OCTET];
	auto type_bool = espace.typeuse[TypeBase::BOOL];
	operateurs.type_bool = type_bool;

	for (auto op : operateurs_entiers_reels) {
		for (auto type : types_entiers_relatifs) {
			auto operateur = operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF);

			if (op == GenreLexeme::PLUS) {
				type->operateur_ajt = operateur;
			}
			else if (op == GenreLexeme::MOINS) {
				type->operateur_sst = operateur;
			}
		}

		for (auto type : types_entiers_naturels) {
			auto operateur = operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_NATUREL);

			if (op == GenreLexeme::PLUS) {
				type->operateur_ajt = operateur;
			}
			else if (op == GenreLexeme::MOINS) {
				type->operateur_sst = operateur;
			}
		}

		for (auto type : types_reels) {
			auto operateur = operateurs.ajoute_basique(op, type, type, IndiceTypeOp::REEL);

			if (op == GenreLexeme::PLUS) {
				type->operateur_ajt = operateur;
			}
			else if (op == GenreLexeme::MOINS) {
				type->operateur_sst = operateur;
			}
		}

		operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF);
		operateurs.ajoute_basique(op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL);
	}

	for (auto op : operateurs_comparaisons) {
		for (auto type : types_entiers_relatifs) {
			auto operateur = operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::ENTIER_RELATIF);

			if (op == GenreLexeme::SUPERIEUR) {
				type->operateur_sup = operateur;
			}
			else if (op == GenreLexeme::SUPERIEUR_EGAL) {
				type->operateur_seg = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR) {
				type->operateur_inf = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR_EGAL) {
				type->operateur_ieg = operateur;
			}
		}

		for (auto type : types_entiers_naturels) {
			auto operateur = operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::ENTIER_NATUREL);

			if (op == GenreLexeme::SUPERIEUR) {
				type->operateur_sup = operateur;
			}
			else if (op == GenreLexeme::SUPERIEUR_EGAL) {
				type->operateur_seg = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR) {
				type->operateur_inf = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR_EGAL) {
				type->operateur_ieg = operateur;
			}
		}

		for (auto type : types_reels) {
			auto operateur = operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::REEL);

			if (op == GenreLexeme::SUPERIEUR) {
				type->operateur_sup = operateur;
			}
			else if (op == GenreLexeme::SUPERIEUR_EGAL) {
				type->operateur_seg = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR) {
				type->operateur_inf = operateur;
			}
			else if (op == GenreLexeme::INFERIEUR_EGAL) {
				type->operateur_ieg = operateur;
			}
		}

		operateurs.ajoute_basique(op, type_octet, type_bool, IndiceTypeOp::ENTIER_RELATIF);
		operateurs.ajoute_basique(op, type_entier_constant, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	}

	for (auto op : operateurs_entiers) {
		for (auto type : types_entiers_relatifs) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF);
		}

		for (auto type : types_entiers_naturels) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_NATUREL);
		}

		operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF);
		operateurs.ajoute_basique(op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL);
	}

	// operateurs booléens & | ^ == !=
	operateurs.ajoute_basique(GenreLexeme::CHAPEAU, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	operateurs.ajoute_basique(GenreLexeme::ESPERLUETTE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	operateurs.ajoute_basique(GenreLexeme::BARRE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	operateurs.ajoute_basique(GenreLexeme::EGALITE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);

	// opérateurs unaires + - ~
	for (auto type : types_entiers_naturels) {
		operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
	}

	for (auto type : types_entiers_relatifs) {
		operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
	}

	for (auto type : types_reels) {
		operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
		operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
	}

	auto type_type_de_donnees = espace.typeuse.type_type_de_donnees_;

	operateurs.op_comp_egal_types = operateurs.ajoute_basique(GenreLexeme::EGALITE, type_type_de_donnees, type_bool, IndiceTypeOp::ENTIER_NATUREL);
	operateurs.op_comp_diff_types = operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, type_type_de_donnees, type_bool, IndiceTypeOp::ENTIER_NATUREL);
}

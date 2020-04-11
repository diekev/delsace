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

#include "lexemes.hh"

static llvm::Instruction::BinaryOps instruction_llvm_pour_operateur(
		GenreLexeme id_operateur,
		IndiceTypeOp type_operandes)
{
	switch (id_operateur) {
		case GenreLexeme::PLUS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::Instruction::FAdd;
			}

			return llvm::Instruction::Add;
		}
		case GenreLexeme::MOINS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::Instruction::FSub;
			}

			return llvm::Instruction::Sub;
		}
		case GenreLexeme::FOIS:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::Instruction::FMul;
			}

			return llvm::Instruction::Mul;
		}
		case GenreLexeme::DIVISE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::Instruction::FDiv;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::Instruction::UDiv;
			}

			return llvm::Instruction::SDiv;
		}
		case GenreLexeme::POURCENT:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::Instruction::FRem;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::Instruction::URem;
			}

			return llvm::Instruction::SRem;
		}
		case GenreLexeme::DECALAGE_DROITE:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::Instruction::LShr;
			}

			return llvm::Instruction::AShr;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		{
			return llvm::Instruction::Shl;
		}
		case GenreLexeme::ESPERLUETTE:
		case GenreLexeme::ESP_ESP:
		{
			return llvm::Instruction::And;
		}
		case GenreLexeme::BARRE:
		case GenreLexeme::BARRE_BARRE:
		{
			return llvm::Instruction::Or;
		}
		case GenreLexeme::CHAPEAU:
		{
			return llvm::Instruction::Xor;
		}
		default:
		{
			return static_cast<llvm::Instruction::BinaryOps>(0);
		}
	}
}

static llvm::CmpInst::Predicate predicat_llvm_pour_operateur(
		GenreLexeme id_operateur,
		IndiceTypeOp type_operandes)
{
	switch (id_operateur) {
		case GenreLexeme::INFERIEUR:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::CmpInst::Predicate::ICMP_ULT;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_RELATIF) {
				return llvm::CmpInst::Predicate::ICMP_SLT;
			}

			return llvm::CmpInst::Predicate::FCMP_OLT;
		}
		case GenreLexeme::INFERIEUR_EGAL:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::CmpInst::Predicate::ICMP_ULE;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_RELATIF) {
				return llvm::CmpInst::Predicate::ICMP_SLE;
			}

			return llvm::CmpInst::Predicate::FCMP_OLE;
		}
		case GenreLexeme::SUPERIEUR:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::CmpInst::Predicate::ICMP_UGT;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_RELATIF) {
				return llvm::CmpInst::Predicate::ICMP_SGT;
			}

			return llvm::CmpInst::Predicate::FCMP_OGT;
		}
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
				return llvm::CmpInst::Predicate::ICMP_UGE;
			}

			if (type_operandes == IndiceTypeOp::ENTIER_RELATIF) {
				return llvm::CmpInst::Predicate::ICMP_SGE;
			}

			return llvm::CmpInst::Predicate::FCMP_OGE;
		}
		case GenreLexeme::EGALITE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::CmpInst::Predicate::FCMP_OEQ;
			}

			return llvm::CmpInst::Predicate::ICMP_EQ;
		}
		case GenreLexeme::DIFFERENCE:
		{
			if (type_operandes == IndiceTypeOp::REEL) {
				return llvm::CmpInst::Predicate::FCMP_ONE;
			}

			return llvm::CmpInst::Predicate::ICMP_NE;
		}
		default:
		{
			return static_cast<llvm::CmpInst::Predicate>(0);
		}
	}
}

static OperateurBinaire::Genre genre_op_binaire_pour_lexeme(GenreLexeme genre_lexeme)
{
	switch (genre_lexeme) {
		case GenreLexeme::PLUS:
		{
			return OperateurBinaire::Genre::Addition;
		}
		case GenreLexeme::MOINS:
		{
			return OperateurBinaire::Genre::Soustraction;
		}
		case GenreLexeme::FOIS:
		{
			return OperateurBinaire::Genre::Multiplication;
		}
		case GenreLexeme::DIVISE:
		{
			return OperateurBinaire::Genre::Division;
		}
		case GenreLexeme::POURCENT:
		{
			return OperateurBinaire::Genre::Reste;
		}
		case GenreLexeme::DECALAGE_DROITE:
		{
			return OperateurBinaire::Genre::Dec_Droite;
		}
		case GenreLexeme::DECALAGE_GAUCHE:
		{
			return OperateurBinaire::Genre::Dec_Gauche;
		}
		case GenreLexeme::ESP_ESP:
		{
			return OperateurBinaire::Genre::Et_Logique;
		}
		case GenreLexeme::BARRE_BARRE:
		{
			return OperateurBinaire::Genre::Ou_Logique;
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
			return OperateurBinaire::Genre::Comp_Inf;
		}
		case GenreLexeme::INFERIEUR_EGAL:
		{
			return OperateurBinaire::Genre::Comp_Inf_Egal;
		}
		case GenreLexeme::SUPERIEUR:
		{
			return OperateurBinaire::Genre::Comp_Sup;
		}
		case GenreLexeme::SUPERIEUR_EGAL:
		{
			return OperateurBinaire::Genre::Comp_Sup_Egal;
		}
		case GenreLexeme::EGALITE:
		{
			return OperateurBinaire::Genre::Comp_Egal;
		}
		case GenreLexeme::DIFFERENCE:
		{
			return OperateurBinaire::Genre::Comp_Inegal;
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

#include "contexte_generation_code.h"

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
		case OperateurBinaire::Genre::Soustraction:
		{
			return "sst";
		}
		case OperateurBinaire::Genre::Multiplication:
		{
			return "mul";
		}
		case OperateurBinaire::Genre::Division:
		{
			return "div";
		}
		case OperateurBinaire::Genre::Reste:
		{
			return "mod";
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
		case OperateurBinaire::Genre::Et_Logique:
		{
			return "et";
		}
		case OperateurBinaire::Genre::Ou_Logique:
		{
			return "ou";
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
		case OperateurBinaire::Genre::Dec_Droite:
		{
			return "decd";
		}
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

Operateurs::~Operateurs()
{
}

const Operateurs::type_conteneur_unaire &Operateurs::trouve_unaire(GenreLexeme id) const
{
	return operateurs_unaires.trouve(id)->second;
}

const Operateurs::type_conteneur_binaire &Operateurs::trouve_binaire(GenreLexeme id) const
{
	return operateurs_binaires.trouve(id)->second;
}

void Operateurs::ajoute_basique(
		GenreLexeme id,
		Type *type,
		Type *type_resultat,
		IndiceTypeOp indice_type,
		RaisonOp raison)
{
	ajoute_basique(id, type, type, type_resultat, indice_type, raison);
}

void Operateurs::ajoute_basique(
		GenreLexeme id,
		Type *type1,
		Type *type2,
		Type *type_resultat,
		IndiceTypeOp indice_type,
		RaisonOp raison)
{
	assert(type1);
	assert(type2);

	auto op = operateurs_binaires[id].ajoute_element();
	op->type1 = type1;
	op->type2 = type2;
	op->type_resultat = type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = true;
	op->genre = genre_op_binaire_pour_lexeme(id);

	if (raison == RaisonOp::POUR_COMPARAISON) {
		op->est_comp_reel = indice_type == IndiceTypeOp::REEL;
		op->est_comp_entier = !op->est_comp_reel;
		op->predicat_llvm = predicat_llvm_pour_operateur(id, indice_type);
	}
	else if (raison == RaisonOp::POUR_ARITHMETIQUE) {
		op->instr_llvm = instruction_llvm_pour_operateur(id, indice_type);
	}
}

void Operateurs::ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat)
{
	auto op = operateurs_unaires[id].ajoute_element();
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
		const dls::chaine &nom_fonction)
{
	auto op = operateurs_binaires[id].ajoute_element();
	op->type1 = type1;
	op->type2 = type2;
	op->type_resultat = type_resultat;
	op->est_commutatif = est_commutatif(id);
	op->est_basique = false;
	op->nom_fonction = nom_fonction;
	op->genre = genre_op_binaire_pour_lexeme(id);
}

void Operateurs::ajoute_perso_unaire(
		GenreLexeme id,
		Type *type,
		Type *type_resultat,
		const dls::chaine &nom_fonction)
{
	auto op = operateurs_unaires[id].ajoute_element();
	op->type_operande = type;
	op->type_resultat = type_resultat;
	op->est_basique = false;
	op->nom_fonction = nom_fonction;
	op->genre = genre_op_unaire_pour_lexeme(id);
}

void Operateurs::ajoute_operateur_basique_enum(Type *type)
{
	for (auto op : operateurs_comparaisons) {
		/* À FAIRE: typage exacte de l'énumération */
		this->ajoute_basique(op, type, type_bool, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_COMPARAISON);
	}

	for (auto op : operateurs_entiers) {
		/* À FAIRE: typage exacte de l'énumération */
		this->ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_ARITHMETIQUE);
	}
}

size_t Operateurs::memoire_utilisee() const
{
	auto memoire = 0ul;

	// compte la mémoire des noeuds de la table de hachage
	memoire += static_cast<size_t>(operateurs_unaires.taille()) * (sizeof(GenreLexeme) + sizeof(type_conteneur_unaire));
	memoire += static_cast<size_t>(operateurs_binaires.taille()) * (sizeof(GenreLexeme) + sizeof(type_conteneur_binaire));

	POUR (operateurs_unaires) {
		memoire += static_cast<size_t>(it.second.taille()) * (sizeof(OperateurUnaire) + sizeof(OperateurUnaire *));
	}

	POUR (operateurs_binaires) {
		memoire += static_cast<size_t>(it.second.taille()) * (sizeof(OperateurBinaire) + sizeof(OperateurBinaire *));
	}

	return memoire;
}

static double verifie_compatibilite(
		Type *type_arg,
		Type *type_enf,
		TransformationType &transformation)
{
	transformation = cherche_transformation(type_enf, type_arg);

	if (transformation.type == TypeTransformation::INUTILE) {
		return 1.0;
	}

	if (transformation.type == TypeTransformation::IMPOSSIBLE) {
		return 0.0;
	}

	/* nous savons que nous devons transformer la valeur (par ex. eini), donc
	 * donne un mi-poids à l'argument */
	return 0.5;
}

dls::tablet<OperateurCandidat, 10> cherche_candidats_operateurs(
		ContexteGenerationCode const &contexte,
		Type *type1,
		Type *type2,
		GenreLexeme type_op)
{
	assert(type1);
	assert(type2);

	auto op_candidats = dls::tablet<OperateurBinaire const *, 10>();

	auto &iter = contexte.operateurs.trouve_binaire(type_op);

	for (auto i = 0; i < iter.taille(); ++i) {
		auto op = &iter[i];

		if (op->type1 == type1 && op->type2 == type2) {
			op_candidats.efface();
			op_candidats.pousse(op);
			break;
		}

		if (op->type1 == type1 || op->type2 == type2) {
			op_candidats.pousse(op);
		}
		else if (op->est_commutatif && (op->type2 == type1 || op->type1 == type2)) {
			op_candidats.pousse(op);
		}
	}

	auto candidats = dls::tablet<OperateurCandidat, 10>();

	for (auto const op : op_candidats) {
		auto seq1 = TransformationType{};
		auto seq2 = TransformationType{};

		auto poids1 = verifie_compatibilite(op->type1, type1, seq1);
		auto poids2 = verifie_compatibilite(op->type2, type2, seq2);

		auto poids = poids1 * poids2;

		if (poids != 0.0) {
			auto candidat = OperateurCandidat{};
			candidat.op = op;
			candidat.poids = poids;
			candidat.transformation_type1 = seq1;
			candidat.transformation_type2 = seq2;

			candidats.pousse(candidat);
		}

		if (op->est_commutatif && poids != 1.0) {
			poids1 = verifie_compatibilite(op->type1, type2, seq2);
			poids2 = verifie_compatibilite(op->type2, type1, seq1);

			poids = poids1 * poids2;

			if (poids != 0.0) {
				auto candidat = OperateurCandidat{};
				candidat.op = op;
				candidat.poids = poids;
				candidat.transformation_type1 = seq1;
				candidat.transformation_type2 = seq2;
				candidat.inverse_operandes = true;

				candidats.pousse(candidat);
			}
		}
	}

	return candidats;
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
		ContexteGenerationCode &contexte,
		Operateurs &operateurs)
{
	Type *types_entiers_naturels[] = {
		contexte.typeuse[TypeBase::N8],
		contexte.typeuse[TypeBase::N16],
		contexte.typeuse[TypeBase::N32],
		contexte.typeuse[TypeBase::N64],
	};

	Type *types_entiers_relatifs[] = {
		contexte.typeuse[TypeBase::Z8],
		contexte.typeuse[TypeBase::Z16],
		contexte.typeuse[TypeBase::Z32],
		contexte.typeuse[TypeBase::Z64],
	};

	auto type_r16 = contexte.typeuse[TypeBase::R16];
	auto type_r32 = contexte.typeuse[TypeBase::R32];
	auto type_r64 = contexte.typeuse[TypeBase::R64];

	Type *types_reels[] = {
		type_r32, type_r64
	};

	auto type_entier_constant = contexte.typeuse[TypeBase::ENTIER_CONSTANT];
	auto type_octet = contexte.typeuse[TypeBase::OCTET];
	auto type_bool = contexte.typeuse[TypeBase::BOOL];
	operateurs.type_bool = type_bool;

	for (auto op : operateurs_entiers_reels) {
		for (auto type : types_entiers_relatifs) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_ARITHMETIQUE);
		}

		for (auto type : types_entiers_naturels) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
		}

		for (auto type : types_reels) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::REEL, RaisonOp::POUR_ARITHMETIQUE);
		}

		operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_ARITHMETIQUE);
		operateurs.ajoute_basique(op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	}

	for (auto op : operateurs_comparaisons) {
		for (auto type : types_entiers_relatifs) {
			operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_COMPARAISON);
		}

		for (auto type : types_entiers_naturels) {
			operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_COMPARAISON);
		}

		for (auto type : types_reels) {
			operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::REEL, RaisonOp::POUR_COMPARAISON);
		}

		operateurs.ajoute_basique(op, type_octet, type_bool, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_COMPARAISON);
		operateurs.ajoute_basique(op, type_entier_constant, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_COMPARAISON);
	}

	for (auto op : operateurs_entiers) {
		for (auto type : types_entiers_relatifs) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_ARITHMETIQUE);
		}

		for (auto type : types_entiers_naturels) {
			operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
		}

		operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF, RaisonOp::POUR_ARITHMETIQUE);
		operateurs.ajoute_basique(op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	}

	// operateurs booléens & | ^ && || == !=
	operateurs.ajoute_basique(GenreLexeme::CHAPEAU, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::ESPERLUETTE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::BARRE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::ESP_ESP, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::BARRE_BARRE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::EGALITE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);
	operateurs.ajoute_basique(GenreLexeme::DIFFERENCE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL, RaisonOp::POUR_ARITHMETIQUE);

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

	// opérateurs unaires booléens !
	operateurs.ajoute_basique_unaire(GenreLexeme::EXCLAMATION, type_bool, type_bool);

	// type r16

	operateurs.ajoute_perso(GenreLexeme::EGAL, type_r16, type_r32, type_r16, "DLS_depuis_r32");
	operateurs.ajoute_perso(GenreLexeme::EGAL, type_r16, type_r64, type_r16, "DLS_depuis_r64");

	operateurs.ajoute_perso(GenreLexeme::EGAL, type_r32, type_r16, type_r32, "DLS_vers_r32");
	operateurs.ajoute_perso(GenreLexeme::EGAL, type_r64, type_r16, type_r64, "DLS_vers_r64");
}

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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "donnees_type.h"

#ifdef AVEC_LLVM
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#include <llvm/IR/TypeBuilder.h>
#pragma GCC diagnostic pop
#endif

#include <cassert>
#include <sstream>

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "morceaux.hh"

DonneesType::DonneesType(id_morceau i0)
{
	m_donnees.push_back(i0);
}

DonneesType::DonneesType(id_morceau i0, id_morceau i1)
{
	m_donnees.push_back(i0);
	m_donnees.push_back(i1);
}

void DonneesType::pousse(id_morceau identifiant)
{
	m_donnees.push_back(identifiant);
}

void DonneesType::pousse(const DonneesType &autre)
{
	auto const taille = m_donnees.size();
	m_donnees.resize(taille + autre.m_donnees.size());
	std::copy(autre.m_donnees.begin(), autre.m_donnees.end(), m_donnees.begin() + static_cast<long>(taille));
}

id_morceau DonneesType::type_base() const
{
	return m_donnees.front();
}

bool DonneesType::est_invalide() const
{
	if (m_donnees.empty()) {
		return true;
	}

	switch (m_donnees.back()) {
		case id_morceau::POINTEUR:
		case id_morceau::REFERENCE:
		case id_morceau::TABLEAU:
			return true;
		default:
			return false;
	}
}

DonneesType::iterateur_const DonneesType::begin() const
{
	return m_donnees.rbegin();
}

DonneesType::iterateur_const DonneesType::end() const
{
	return m_donnees.rend();
}

DonneesType DonneesType::derefence() const
{
	auto donnees = DonneesType{};

	for (size_t i = 1; i < m_donnees.size(); ++i) {
		donnees.pousse(m_donnees[i]);
	}

	return donnees;
}

std::string chaine_type(DonneesType const &donnees_type, ContexteGenerationCode const &contexte)
{
	std::stringstream os;

	if (donnees_type.est_invalide()) {
		os << "type invalide";
	}
	else {
		auto debut = donnees_type.end() - 1;
		auto fin = donnees_type.begin() - 1;

		for (;debut != fin; --debut) {
			auto donnee = *debut;
			switch (donnee & 0xff) {
				case id_morceau::TROIS_POINTS:
				{
					os << "...";
					break;
				}
				case id_morceau::POINTEUR:
					os << '*';
					break;
				case id_morceau::TABLEAU:
					os << '[';

					if (static_cast<size_t>(donnee >> 8) != 0) {
						os << static_cast<size_t>(donnee >> 8);
					}

					os << ']';
					break;
				case id_morceau::N8:
					os << "n8";
					break;
				case id_morceau::N16:
					os << "n16";
					break;
				case id_morceau::N32:
					os << "n32";
					break;
				case id_morceau::N64:
					os << "n64";
					break;
				case id_morceau::R16:
					os << "r16";
					break;
				case id_morceau::R32:
					os << "r32";
					break;
				case id_morceau::R64:
					os << "r64";
					break;
				case id_morceau::Z8:
					os << "z8";
					break;
				case id_morceau::Z16:
					os << "z16";
					break;
				case id_morceau::Z32:
					os << "z32";
					break;
				case id_morceau::Z64:
					os << "z64";
					break;
				case id_morceau::BOOL:
					os << "bool";
					break;
				case id_morceau::CHAINE:
					os << "chaine";
					break;
				case id_morceau::FONC:
					os << "fonc";
					break;
				case id_morceau::COROUT:
					os << "corout";
					break;
				case id_morceau::PARENTHESE_OUVRANTE:
					os << '(';
					break;
				case id_morceau::PARENTHESE_FERMANTE:
					os << ')';
					break;
				case id_morceau::VIRGULE:
					os << ',';
					break;
				case id_morceau::EINI:
					os << "eini";
					break;
				case id_morceau::RIEN:
					os << "rien";
					break;
				case id_morceau::OCTET:
					os << "octet";
					break;
				case id_morceau::NUL:
					os << "nul";
					break;
				case id_morceau::CHAINE_CARACTERE:
				{
					auto id = static_cast<size_t>(donnee >> 8);
					os << contexte.nom_struct(id);
					break;
				}
				default:
				{
					os << "INVALIDE";
					break;
				}
			}
		}
	}

	return os.str();
}

/* ************************************************************************** */

struct DonneesTypeCommun {
	int val_enum;
	DonneesType dt;
};

static const DonneesTypeCommun donnees_types_communs[] = {
	{ TYPE_N8, DonneesType(id_morceau::N8) },
	{ TYPE_N16, DonneesType(id_morceau::N16) },
	{ TYPE_N32, DonneesType(id_morceau::N32) },
	{ TYPE_N64, DonneesType(id_morceau::N64) },
	{ TYPE_Z8, DonneesType(id_morceau::Z8) },
	{ TYPE_Z16, DonneesType(id_morceau::Z16) },
	{ TYPE_Z32, DonneesType(id_morceau::Z32) },
	{ TYPE_Z64, DonneesType(id_morceau::Z64) },
	{ TYPE_R16, DonneesType(id_morceau::R16) },
	{ TYPE_R32, DonneesType(id_morceau::R32) },
	{ TYPE_R64, DonneesType(id_morceau::R64) },
	{ TYPE_EINI, DonneesType(id_morceau::EINI) },
	{ TYPE_CHAINE, DonneesType(id_morceau::CHAINE) },
	{ TYPE_RIEN, DonneesType(id_morceau::RIEN) },
	{ TYPE_BOOL, DonneesType(id_morceau::BOOL) },
	{ TYPE_OCTET, DonneesType(id_morceau::OCTET) },

	{ TYPE_PTR_N8, DonneesType(id_morceau::POINTEUR, id_morceau::N8) },
	{ TYPE_PTR_N16, DonneesType(id_morceau::POINTEUR, id_morceau::N16) },
	{ TYPE_PTR_N32, DonneesType(id_morceau::POINTEUR, id_morceau::N32) },
	{ TYPE_PTR_N64, DonneesType(id_morceau::POINTEUR, id_morceau::N64) },
	{ TYPE_PTR_Z8, DonneesType(id_morceau::POINTEUR, id_morceau::Z8) },
	{ TYPE_PTR_Z16, DonneesType(id_morceau::POINTEUR, id_morceau::Z16) },
	{ TYPE_PTR_Z32, DonneesType(id_morceau::POINTEUR, id_morceau::Z32) },
	{ TYPE_PTR_Z64, DonneesType(id_morceau::POINTEUR, id_morceau::Z64) },
	/* À FAIRE : type R16 */
	{ TYPE_PTR_R16, DonneesType(id_morceau::POINTEUR, id_morceau::R32) },
	{ TYPE_PTR_R32, DonneesType(id_morceau::POINTEUR, id_morceau::R32) },
	{ TYPE_PTR_R64, DonneesType(id_morceau::POINTEUR, id_morceau::R64) },
	{ TYPE_PTR_EINI, DonneesType(id_morceau::POINTEUR, id_morceau::EINI) },
	{ TYPE_PTR_CHAINE, DonneesType(id_morceau::POINTEUR, id_morceau::CHAINE) },
	{ TYPE_PTR_RIEN, DonneesType(id_morceau::POINTEUR, id_morceau::RIEN) },
	{ TYPE_PTR_NUL, DonneesType(id_morceau::POINTEUR, id_morceau::NUL) },
	{ TYPE_PTR_BOOL, DonneesType(id_morceau::POINTEUR, id_morceau::BOOL) },

	{ TYPE_TABL_N8, DonneesType(id_morceau::TABLEAU, id_morceau::N8) },
	{ TYPE_TABL_N16, DonneesType(id_morceau::TABLEAU, id_morceau::N16) },
	{ TYPE_TABL_N32, DonneesType(id_morceau::TABLEAU, id_morceau::N32) },
	{ TYPE_TABL_N64, DonneesType(id_morceau::TABLEAU, id_morceau::N64) },
	{ TYPE_TABL_Z8, DonneesType(id_morceau::TABLEAU, id_morceau::Z8) },
	{ TYPE_TABL_Z16, DonneesType(id_morceau::TABLEAU, id_morceau::Z16) },
	{ TYPE_TABL_Z32, DonneesType(id_morceau::TABLEAU, id_morceau::Z32) },
	{ TYPE_TABL_Z64, DonneesType(id_morceau::TABLEAU, id_morceau::Z64) },
	/* À FAIRE : type R16 */
	{ TYPE_TABL_R16, DonneesType(id_morceau::TABLEAU, id_morceau::R32) },
	{ TYPE_TABL_R32, DonneesType(id_morceau::TABLEAU, id_morceau::R32) },
	{ TYPE_TABL_R64, DonneesType(id_morceau::TABLEAU, id_morceau::R64) },
	{ TYPE_TABL_EINI, DonneesType(id_morceau::TABLEAU, id_morceau::EINI) },
	{ TYPE_TABL_CHAINE, DonneesType(id_morceau::TABLEAU, id_morceau::CHAINE) },
	{ TYPE_TABL_BOOL, DonneesType(id_morceau::TABLEAU, id_morceau::BOOL) },
	{ TYPE_TABL_OCTET, DonneesType(id_morceau::TABLEAU, id_morceau::OCTET) },
};

MagasinDonneesType::MagasinDonneesType()
{
	/* initialise les types communs */
	index_types_communs.resize(TYPES_TOTAUX);

	for (auto donnees : donnees_types_communs) {
		auto const idx = static_cast<size_t>(donnees.val_enum);
		index_types_communs[idx] = ajoute_type(donnees.dt);
	}
}

size_t MagasinDonneesType::ajoute_type(const DonneesType &donnees)
{
	auto iter = donnees_type_index.find(donnees);

	if (iter != donnees_type_index.end()) {
		return iter->second;
	}

	auto index = donnees_types.size();
	donnees_types.push_back(donnees);

	donnees_type_index.insert({donnees, index});

	return index;
}

void MagasinDonneesType::declare_structures_C(
		ContexteGenerationCode &contexte,
		std::ostream &os)
{
	os << "typedef struct chaine { char *pointeur; long taille; } chaine;\n\n";
	os << "typedef struct eini { void *pointeur; struct InfoType *info; } eini;\n\n";
	os << "typedef unsigned char bool;\n\n";
	os << "typedef unsigned char octet;\n\n";

	for (auto &donnees : donnees_types) {
		if (donnees.type_base() == id_morceau::TABLEAU) {
			os << "typedef struct Tableau_";

			converti_type_C(contexte, "", donnees.derefence(), os, true);

			os << "{\n\t";

			converti_type_C(contexte, "", donnees.derefence(), os, false, true);

			os << " *pointeur;\n\tint taille;\n} Tableau_";

			converti_type_C(contexte, "", donnees.derefence(), os, true);

			os << ";\n\n";
		}
	}
}

size_t MagasinDonneesType::operator[](int type)
{
	return index_types_communs[static_cast<size_t>(type)];
}

static bool converti_type_simple_C(
		ContexteGenerationCode &contexte,
		std::ostream &os,
		std::string_view const &nom_variable,
		id_morceau id,
		bool echappe,
		bool echappe_struct)
{
	bool est_tableau = false;

	switch (id & 0xff) {
		case id_morceau::POINTEUR:
		{
			if (echappe) {
				os << "_ptr_";
			}
			else {
				os << '*';
			}

			break;
		}
		case id_morceau::TABLEAU:
		{
			auto taille = static_cast<size_t>(id >> 8);
			if (!est_tableau) {
				os << " " << nom_variable;
			}

			est_tableau = true;

			os << '[' << taille << ']';

			break;
		}
		case id_morceau::OCTET:
		{
			os << "octet";
			break;
		}
		case id_morceau::BOOL:
		{
			os << "bool";
			break;
		}
		case id_morceau::N8:
		{
			if (echappe) {
				os << "unsigned_char";
			}
			else {
				os << "unsigned char";
			}

			break;
		}
		case id_morceau::N16:
		{
			if (echappe) {
				os << "unsigned_short";
			}
			else {
				os << "unsigned short";
			}

			break;
		}
		case id_morceau::N32:
		{
			if (echappe) {
				os << "unsigned_int";
			}
			else {
				os << "unsigned int";
			}

			break;
		}
		case id_morceau::N64:
		{
			if (echappe) {
				os << "unsigned_long";
			}
			else {
				os << "unsigned long";
			}

			break;
		}
		case id_morceau::R16:
		{
			os << "float";
			break;
		}
		case id_morceau::R32:
		{
			os << "float";
			break;
		}
		case id_morceau::R64:
		{
			os << "double";
			break;
		}
		case id_morceau::Z8:
		{
			os << "char";
			break;
		}
		case id_morceau::Z16:
		{
			os << "short";
			break;
		}
		case id_morceau::Z32:
		{
			os << "int";
			break;
		}
		case id_morceau::Z64:
		{
			os << "long";
			break;
		}
		case id_morceau::CHAINE:
		{
			os << "chaine";
			break;
		}
		case id_morceau::COROUT:
		case id_morceau::FONC:
		{
			break;
		}
		case id_morceau::PARENTHESE_OUVRANTE:
		{
			os << '(';
			break;
		}
		case id_morceau::PARENTHESE_FERMANTE:
		{
			os << ')';
			break;
		}
		case id_morceau::VIRGULE:
		{
			os << ',';
			break;
		}
		case id_morceau::NUL: /* pour par exemple quand on retourne nul */
		case id_morceau::RIEN:
		{
			os << "void";
			break;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto id_struct = static_cast<size_t>(id >> 8);
			auto &donnees_struct = contexte.donnees_structure(id_struct);
			auto nom_structure = contexte.nom_struct(id_struct);

			if (donnees_struct.est_enum) {
				auto &dt = contexte.magasin_types.donnees_types[donnees_struct.noeud_decl->index_type];
				converti_type_simple_C(contexte, os, "", dt.type_base(), false, false);
			}
			else {
				if (echappe_struct || donnees_struct.est_externe) {
					os << "struct ";
				}

				os << broye_nom_simple(nom_structure);
			}

			break;
		}
		case id_morceau::EINI:
		{
			os << "eini";
			break;
		}
		default:
		{
			assert(false);
			break;
		}
	}

	return est_tableau;
}

bool MagasinDonneesType::converti_type_C(
		ContexteGenerationCode &contexte,
		std::string_view const &nom_variable,
		DonneesType const &donnees,
		std::ostream &os,
		bool echappe,
		bool echappe_struct)
{
	if (donnees.est_invalide()) {
		return false;
	}

	if (donnees.type_base() == id_morceau::TABLEAU || donnees.type_base() == id_morceau::TROIS_POINTS) {
		if (echappe_struct) {
			os << "struct ";
		}

		os << "Tableau_";
		return this->converti_type_C(contexte, nom_variable, donnees.derefence(), os, true);
	}

	/* cas spécial pour convertir les types complexes comme []*z8 */
	if (donnees.type_base() == id_morceau::POINTEUR) {
		auto tabl = this->converti_type_C(contexte, nom_variable, donnees.derefence(), os, echappe, echappe_struct);

		if (echappe) {
			os << "_ptr_";
		}
		else {
			os << '*';
		}

		return tabl;
	}

	if (donnees.type_base() == id_morceau::FONC || donnees.type_base() == id_morceau::COROUT) {
		std::vector<std::stack<id_morceau>> liste_pile_type;

		auto debut = donnees.end() - 1;
		auto fin = donnees.begin() - 1;

		/* saute l'id fonction */
		--debut;

		std::stack<id_morceau> pile;

		for (; debut != fin; --debut) {
			if (*debut == id_morceau::PARENTHESE_OUVRANTE) {
				/* rien à faire */
			}
			else if (*debut == id_morceau::PARENTHESE_FERMANTE) {
				liste_pile_type.push_back(pile);
				pile = std::stack<id_morceau>{};
			}
			else if (*debut == id_morceau::VIRGULE) {
				liste_pile_type.push_back(pile);
				pile = std::stack<id_morceau>{};
			}
			else {
				pile.push(*debut);
			}
		}

		/* la pile contient le type retour */
		while (!pile.empty()) {
			converti_type_simple_C(
						contexte,
						os,
						"",
						pile.top(),
						echappe,
						echappe_struct);

			pile.pop();
		}

		os << "(*" << nom_variable << ")";

		auto virgule = '(';

		if (liste_pile_type.size() == 1) {
			os << virgule;
		}
		else {
			for (auto i = 0ul; i < liste_pile_type.size(); ++i) {
				os << virgule;

				auto &pile_type = liste_pile_type[i];

				while (!pile_type.empty()) {
					converti_type_C(
								contexte,
								"",
								pile_type.top(),
								os,
								echappe,
								echappe_struct);

					pile_type.pop();
				}

				virgule = ',';
			}
		}

		os << ')';

		return true;
	}

	auto debut = donnees.begin();
	auto fin = donnees.end();

	bool est_tableau = false;

	for (;debut != fin; ++debut) {
		auto donnee = *debut;
		est_tableau = converti_type_simple_C(contexte, os, nom_variable, donnee, echappe, echappe_struct);
	}

	return est_tableau;
}

#ifdef AVEC_LLVM
llvm::Type *MagasinDonneesType::converti_type(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees)
{
	auto index = ajoute_type(donnees);
	auto &dt = donnees_types[index];
	return ::converti_type(contexte, dt);
}

llvm::Type *MagasinDonneesType::converti_type(
		ContexteGenerationCode &contexte,
		size_t index)
{
	auto &dt = donnees_types[index];
	return ::converti_type(contexte, dt);
}
#endif

/* ************************************************************************** */

#ifdef AVEC_LLVM
llvm::Type *converti_type_simple(
		ContexteGenerationCode &contexte,
		const id_morceau &identifiant,
		llvm::Type *type_entree)
{
	llvm::Type *type = nullptr;

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
			type = llvm::Type::getInt1Ty(contexte.contexte);
			break;
		case id_morceau::N8:
		case id_morceau::Z8:
			type = llvm::Type::getInt8Ty(contexte.contexte);
			break;
		case id_morceau::N16:
		case id_morceau::Z16:
			type = llvm::Type::getInt16Ty(contexte.contexte);
			break;
		case id_morceau::N32:
		case id_morceau::Z32:
			type = llvm::Type::getInt32Ty(contexte.contexte);
			break;
		case id_morceau::N64:
		case id_morceau::Z64:
			type = llvm::Type::getInt64Ty(contexte.contexte);
			break;
		case id_morceau::R16:
			/* À FAIRE : type R16 */
			//type = llvm::Type::getHalfTy(contexte.contexte);
			type = llvm::Type::getFloatTy(contexte.contexte);
			break;
		case id_morceau::R32:
			type = llvm::Type::getFloatTy(contexte.contexte);
			break;
		case id_morceau::R64:
			type = llvm::Type::getDoubleTy(contexte.contexte);
			break;
		case id_morceau::RIEN:
			type = llvm::Type::getVoidTy(contexte.contexte);
			break;
		case id_morceau::POINTEUR:
			type = llvm::PointerType::get(type_entree, 0);
			break;
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			if (donnees_structure.type_llvm == nullptr) {
				std::vector<llvm::Type *> types_membres;
				types_membres.resize(donnees_structure.donnees_types.size());

				std::transform(donnees_structure.donnees_types.begin(),
							   donnees_structure.donnees_types.end(),
							   types_membres.begin(),
							   [&](const size_t index_type)
				{
					auto &dt = contexte.magasin_types.donnees_types[index_type];
					return converti_type(contexte, dt);
				});

				auto nom = "struct." + contexte.nom_struct(donnees_structure.id);

				donnees_structure.type_llvm = llvm::StructType::create(
												  contexte.contexte,
												  types_membres,
												  nom,
												  false);
			}

			type = donnees_structure.type_llvm;
			break;
		}
		case id_morceau::TABLEAU:
		{
			auto const taille = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;

			if (taille != 0) {
				type = llvm::ArrayType::get(type_entree, taille);
			}
			else {
				/* type = structure { *type, n64 } */
				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::PointerType::get(type_entree, 0);
				types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.tableau",
						   false);
			}

			break;
		}
		case id_morceau::EINI:
		{
			auto dt = DonneesType{};
			dt.pousse(id_morceau::EINI);

			auto index_eini = contexte.magasin_types.ajoute_type(dt);
			auto &type_eini = contexte.magasin_types.donnees_types[index_eini];

			if (type_eini.type_llvm() == nullptr) {
				/* type = structure { *z8, *InfoType } */

				auto index_struct_info = contexte.donnees_structure("InfoType").id;

				auto dt_info = DonneesType{};
				dt_info.pousse(id_morceau::POINTEUR);
				dt_info.pousse(id_morceau::CHAINE_CARACTERE | (static_cast<int>(index_struct_info << 8)));

				index_struct_info = contexte.magasin_types.ajoute_type(dt_info);
				auto &ref_dt_info = contexte.magasin_types.donnees_types[index_struct_info];

				auto type_struct_info = converti_type(contexte, ref_dt_info);

				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
				types_membres[1] = type_struct_info;

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.eini",
						   false);

				type_eini.type_llvm(type);
			}

			type = type_eini.type_llvm();
			break;
		}
		case id_morceau::CHAINE:
		{
			auto dt = DonneesType{};
			dt.pousse(id_morceau::CHAINE);

			auto index_chaine = contexte.magasin_types.ajoute_type(dt);
			auto &type_chaine = contexte.magasin_types.donnees_types[index_chaine];

			if (type_chaine.type_llvm() == nullptr) {
				/* type = structure { *z8, z64 } */
				std::vector<llvm::Type *> types_membres(2ul);
				types_membres[0] = llvm::Type::getInt8PtrTy(contexte.contexte);
				types_membres[1] = llvm::Type::getInt64Ty(contexte.contexte);

				type = llvm::StructType::create(
						   contexte.contexte,
						   types_membres,
						   "struct.chaine",
						   false);

				type_chaine.type_llvm(type);
			}

			type = type_chaine.type_llvm();
			break;
		}
		default:
			assert(false);
	}

	return type;
}
#endif

/**
 * Retourne un vecteur contenant les DonneesType de chaque paramètre et du type
 * de retour d'un DonneesType d'un pointeur fonction. Si le DonneesType passé en
 * paramètre n'est pas un pointeur fonction, retourne un vecteur vide.
 */
[[nodiscard]] auto donnees_types_parametres(
		const DonneesType &donnees_type) noexcept(false) -> std::vector<DonneesType>
{
	if (donnees_type.type_base() != id_morceau::FONC) {
		return {};
	}

	auto dt = DonneesType{};
	std::vector<DonneesType> donnees_types;

	auto debut = donnees_type.end() - 1;
	auto fin   = donnees_type.begin() - 1;

	--debut; /* fonction */
	--debut; /* ( */

	/* type paramètres */
	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(*debut--);

			if (*debut == id_morceau::VIRGULE) {
				--debut;
				break;
			}
		}

		donnees_types.push_back(dt);

		dt = DonneesType{};
	}

	--debut; /* ) */

	/* type retour */
	while (debut != fin) {
		dt.pousse(*debut--);
	}

	donnees_types.push_back(dt);

	return donnees_types;
}

#ifdef AVEC_LLVM
llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		DonneesType &donnees_type)
{
	/* Pointeur vers une fonction, seulement valide lors d'assignement, ou en
	 * paramètre de fonction. */
	if (donnees_type.type_base() == id_morceau::FONC) {
		if (donnees_type.type_llvm() != nullptr) {
			return llvm::PointerType::get(donnees_type.type_llvm(), 0);
		}

		llvm::Type *type = nullptr;
		auto dt = DonneesType{};
		std::vector<llvm::Type *> parametres;

		auto dt_params = donnees_types_parametres(donnees_type);

		for (size_t i = 0; i < dt_params.size() - 1; ++i) {
			type = converti_type(contexte, dt_params[i]);
			parametres.push_back(type);
		}

		type = converti_type(contexte, dt_params.back());
		type = llvm::FunctionType::get(
					type,
					parametres,
					false);

		donnees_type.type_llvm(type);

		return llvm::PointerType::get(type, 0);
	}

	if (donnees_type.type_llvm() != nullptr) {
		return donnees_type.type_llvm();
	}

	llvm::Type *type = nullptr;

	for (id_morceau identifiant : donnees_type) {
		type = converti_type_simple(contexte, identifiant, type);
	}

	donnees_type.type_llvm(type);

	return type;
}
#endif

unsigned alignement(
		ContexteGenerationCode &contexte,
		const DonneesType &donnees_type)
{
	id_morceau identifiant = donnees_type.type_base();

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
			return 1;
		case id_morceau::R16:
		case id_morceau::N16:
		case id_morceau::Z16:
			return 2;
		case id_morceau::R32:
		case id_morceau::N32:
		case id_morceau::Z32:
			return 4;
		case id_morceau::TABLEAU:
		{
			if (size_t(identifiant >> 8) == 0) {
				return 8;
			}

			return alignement(contexte, donnees_type.derefence());
		}
		case id_morceau::FONC:
		case id_morceau::POINTEUR:
		case id_morceau::EINI:
		case id_morceau::R64:
		case id_morceau::N64:
		case id_morceau::Z64:
		case id_morceau::CHAINE:
			return 8;
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &ds = contexte.donnees_structure(id_structure);

			if (ds.est_enum) {
				auto dt_enum = contexte.magasin_types.donnees_types[ds.noeud_decl->index_type];
				return alignement(contexte, dt_enum);
			}

			auto a = 0u;

			for (auto const &donnees : ds.donnees_types) {
				auto const &dt = contexte.magasin_types.donnees_types[donnees];
				a = std::max(a, alignement(contexte, dt));
			}

			return a;
		}
		default:
			assert(false);
	}

	return 0;
}

/* ************************************************************************** */

bool est_type_entier(id_morceau type)
{
	switch (type) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::POINTEUR:  /* À FAIRE : sépare ça. */
		case id_morceau::OCTET:
			return true;
		default:
			return false;
	}
}

bool est_type_entier_naturel(id_morceau type)
{
	switch (type) {
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::POINTEUR:  /* À FAIRE : sépare ça. */
			return true;
		default:
			return false;
	}
}

bool est_type_entier_relatif(id_morceau type)
{
	switch (type) {
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
			return true;
		default:
			return false;
	}
}

bool est_type_reel(id_morceau type)
{
	switch (type) {
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
			return true;
		default:
			return false;
	}
}

niveau_compat sont_compatibles(const DonneesType &type1, const DonneesType &type2, type_noeud type_droite)
{
	if (type1 == type2) {
		return niveau_compat::ok;
	}

	if (type_droite == type_noeud::NOMBRE_ENTIER && est_type_entier(type1.type_base())) {
		return niveau_compat::ok;
	}

	if (type_droite == type_noeud::NOMBRE_REEL && est_type_reel(type1.type_base())) {
		return niveau_compat::ok;
	}

	if (type1.type_base() == id_morceau::EINI) {
		if ((type2.type_base() & 0xff) == id_morceau::TABLEAU && (type2.type_base() != id_morceau::TABLEAU)) {
			return niveau_compat::converti_eini | niveau_compat::converti_tableau;
		}

		return niveau_compat::converti_eini;
	}

	if (type2.type_base() == id_morceau::EINI) {
		return niveau_compat::extrait_eini;
	}

	if (type1.type_base() == id_morceau::FONC) {
		/* x : fonc()rien = nul; */
		if (type2.type_base() == id_morceau::POINTEUR && type2.derefence().type_base() == id_morceau::NUL) {
			return niveau_compat::ok;
		}

		/* Nous savons que les types sont différents, donc si l'un des deux est un
		 * pointeur fonction, nous pouvons retourner faux. */
		return niveau_compat::aucune;
	}

	if (type1.type_base() == id_morceau::TABLEAU) {
		if (type1.derefence().type_base() == id_morceau::OCTET) {
			return niveau_compat::converti_tableau_octet;
		}

		if ((type2.type_base() & 0xff) != id_morceau::TABLEAU) {
			return niveau_compat::aucune;
		}

		if (type1.derefence() == type2.derefence()) {
			return niveau_compat::converti_tableau;
		}

		return niveau_compat::aucune;
	}

	if (type1.type_base() == id_morceau::POINTEUR) {
		if (type2.type_base() == id_morceau::POINTEUR) {
			/* x = nul; */
			if (type2.derefence().type_base() == id_morceau::NUL) {
				return niveau_compat::ok;
			}

			/* x : *rien = y; */
			if (type1.derefence().type_base() == id_morceau::RIEN) {
				return niveau_compat::ok;
			}

			/* x : *octet = y; */
			if (type1.derefence().type_base() == id_morceau::OCTET) {
				return niveau_compat::ok;
			}
		}

		if (type1.derefence().type_base() == id_morceau::Z8) {
			if (type2.type_base() == id_morceau::CHAINE) {
				return niveau_compat::extrait_chaine_c;
			}
		}
	}

	return niveau_compat::aucune;
}

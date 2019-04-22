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

#include "arbre_syntactic.h"
#include "contexte_generation_code.h"
#include "morceaux.h"

void DonneesType::pousse(id_morceau identifiant)
{
	m_donnees.push_back(identifiant);
}

void DonneesType::pousse(const DonneesType &autre)
{
	auto const taille = m_donnees.size();
	m_donnees.resize(taille + autre.m_donnees.size());
	std::copy(autre.m_donnees.begin(), autre.m_donnees.end(), m_donnees.begin() + taille);
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

std::ostream &operator<<(std::ostream &os, const DonneesType &donnees_type)
{
	if (donnees_type.est_invalide()) {
		os << "type invalide";
	}
	else {
		auto debut = donnees_type.end() - 1;
		auto fin = donnees_type.begin() - 1;

		for (;debut != fin; --debut) {
			auto donnee = *debut;
			switch (donnee & 0xff) {
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
					os << "chaîne";
					break;
				case id_morceau::FONCTION:
					os << "fonction";
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
				default:
					os << chaine_identifiant(donnee & 0xff);
					break;
			}
		}
	}

	return os;
}

/* ************************************************************************** */

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

	if (donnees.type_base() == id_morceau::TABLEAU) {
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

	auto debut = donnees.begin();
	auto fin = donnees.end();

	bool est_tableau = false;

	for (;debut != fin; ++debut) {
		auto donnee = *debut;
		switch (donnee & 0xff) {
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
				auto taille = static_cast<size_t>(donnee >> 8);
				if (!est_tableau) {
					os << " " << nom_variable;
				}

				est_tableau = true;

				os << '[' << taille << ']';

				break;
			}
			case id_morceau::BOOL:
			case id_morceau::N8:
			{
				os << "unsigned char";
				break;
			}
			case id_morceau::N16:
			{
				os << "unsigned short";
				break;
			}
			case id_morceau::N32:
				os << "unsigned int";
				break;
			case id_morceau::N64:
			{
				os << "unsigned long";
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
			case id_morceau::FONCTION:
			{
				/* À FAIRE */;
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
			case id_morceau::RIEN:
			{
				os << "void";
				break;
			}
			case id_morceau::CHAINE_CARACTERE:
			{
				auto id = static_cast<size_t>(donnee >> 8);
				auto nom_structure = contexte.nom_struct(id);
				if (echappe_struct) {
					os << "struct ";
				}
				os << nom_structure;
				break;
			}
			case id_morceau::EINI:
			{
				os << "eini";
				break;
			}
			default:
				break;
		}
	}

	return est_tableau;
}

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

/* ************************************************************************** */

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
		default:
			assert(false);
	}

	return type;
}

/**
 * Retourne un vecteur contenant les DonneesType de chaque paramètre et du type
 * de retour d'un DonneesType d'un pointeur fonction. Si le DonneesType passé en
 * paramètre n'est pas un pointeur fonction, retourne un vecteur vide.
 */
[[nodiscard]] auto donnees_types_parametres(
		const DonneesType &donnees_type) noexcept(false) -> std::vector<DonneesType>
{
	if (donnees_type.type_base() != id_morceau::FONCTION) {
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

llvm::Type *converti_type(
		ContexteGenerationCode &contexte,
		DonneesType &donnees_type)
{
	/* Pointeur vers une fonction, seulement valide lors d'assignement, ou en
	 * paramètre de fonction. */
	if (donnees_type.type_base() == id_morceau::FONCTION) {
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
		case id_morceau::FONCTION:
		case id_morceau::POINTEUR:
		case id_morceau::EINI:
		case id_morceau::R64:
		case id_morceau::N64:
		case id_morceau::Z64:
			return 8;
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<uint64_t>(identifiant) & 0xffffff00) >> 8;
			auto &donnees_structure = contexte.donnees_structure(id_structure);

			auto a = 0u;

			for (auto const &donnees : donnees_structure.donnees_types) {
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

/* À FAIRE : déduplique ça. */
static bool est_type_entier(id_morceau type)
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
			return true;
		default:
			return false;
	}
}

static bool est_type_reel(id_morceau type)
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

	/* Nous savons que les types sont différents, donc si l'un des deux est un
	 * pointeur fonction, nous pouvons retourner faux. */
	if (type1.type_base() == id_morceau::FONCTION) {
		return niveau_compat::aucune;
	}

	if (type1.type_base() == id_morceau::TABLEAU) {
		if ((type2.type_base() & 0xff) != id_morceau::TABLEAU) {
			return niveau_compat::aucune;
		}

		if (type1.derefence() == type2.derefence()) {
			return niveau_compat::converti_tableau;
		}

		return niveau_compat::aucune;
	}

	/* À FAIRE : C-strings */
	if (type1.type_base() == id_morceau::POINTEUR) {
		if (type1.derefence().type_base() != id_morceau::Z8) {
			return niveau_compat::aucune;
		}

		if (type2.type_base() == id_morceau::CHAINE) {
			return niveau_compat::extrait_chaine_c;
		}
	}

	return niveau_compat::aucune;
}

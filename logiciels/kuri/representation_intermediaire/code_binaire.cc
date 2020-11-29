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

#include "code_binaire.hh"

#include <iomanip>

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "compilation/arbre_syntaxique.hh"
#include "compilation/compilatrice.hh"
#include "compilation/operateurs.hh"
#include "compilation/outils_lexemes.hh"

#include "machine_virtuelle.hh"

const char *chaine_code_operation(octet_t code_operation)
{
	switch (code_operation) {
#define ENUMERE_CODE_OPERATION_EX(code) case code: return #code;
		ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
	}

	return "ERREUR";
}

/* ************************************************************************** */

Chunk::~Chunk()
{
	detruit();
}

void Chunk::initialise()
{
	code = nullptr;
	compte = 0;
	capacite = 0;
}

void Chunk::detruit()
{
	memoire::deloge_tableau("Chunk::code", code, capacite);
	initialise();
}

void Chunk::emets(octet_t o)
{
	agrandis_si_necessaire(1);
	code[compte] = o;
	compte += 1;
}

void Chunk::agrandis_si_necessaire(long taille)
{
	if (capacite < compte + taille) {
		auto nouvelle_capacite = capacite < 8 ? 8 : capacite * 2;
		memoire::reloge_tableau("Chunk::code", code, capacite, nouvelle_capacite);
		capacite = nouvelle_capacite;
	}
}

int Chunk::emets_allocation(NoeudExpression *site, Type *type, IdentifiantCode *ident)
{
	// XXX - À FAIRE : normalise les entiers constants
	if (type->genre == GenreType::ENTIER_CONSTANT) {
		type->taille_octet = 4;
	}
	assert(type->taille_octet);
	emets(OP_ALLOUE);
	emets(site);
	emets(type);
	emets(ident);

	auto decalage = taille_allouee;
	taille_allouee += static_cast<int>(type->taille_octet);
	return decalage;
}

void Chunk::emets_assignation(NoeudExpression *site, Type *type)
{
	assert(type->taille_octet);
	emets(OP_ASSIGNE);
	emets(site);
	emets(type->taille_octet);
}

void Chunk::emets_charge(NoeudExpression *site, Type *type)
{
	assert(type->taille_octet);
	emets(OP_CHARGE);
	emets(site);
	emets(type->taille_octet);
}

void Chunk::emets_charge_variable(NoeudExpression *site, int pointeur, Type *type)
{
	assert(type->taille_octet);
	emets_reference_variable(site, pointeur);
	emets_charge(site, type);
}

void Chunk::emets_reference_globale(NoeudExpression *site, int pointeur)
{
	emets(OP_REFERENCE_GLOBALE);
	emets(site);
	emets(pointeur);
}

void Chunk::emets_reference_variable(NoeudExpression *site, int pointeur)
{
	emets(OP_REFERENCE_VARIABLE);
	emets(site);
	emets(pointeur);
}

void Chunk::emets_reference_membre(NoeudExpression *site, unsigned decalage)
{
	emets(OP_REFERENCE_MEMBRE);
	emets(site);
	emets(decalage);
}

void Chunk::emets_appel(NoeudExpression *site, AtomeFonction *fonction, unsigned taille_arguments, InstructionAppel *inst_appel)
{
	emets(OP_APPEL);
	emets(site);
	emets(fonction);
	emets(taille_arguments);
	emets(inst_appel);
}

void Chunk::emets_appel_externe(NoeudExpression *site, AtomeFonction *fonction, unsigned taille_arguments, InstructionAppel *inst_appel)
{
	emets(OP_APPEL_EXTERNE);
	emets(site);
	emets(fonction);
	emets(taille_arguments);
	emets(inst_appel);
}

void Chunk::emets_appel_pointeur(NoeudExpression *site, unsigned taille_arguments, InstructionAppel *inst_appel)
{
	emets(OP_APPEL_POINTEUR);
	emets(site);
	emets(taille_arguments);
	emets(inst_appel);
}

void Chunk::emets_acces_index(NoeudExpression *site, Type *type)
{
	assert(type->taille_octet);
	emets(OP_ACCEDE_INDEX);
	emets(site);
	emets(type->taille_octet);
}

void Chunk::emets_branche(NoeudExpression *site, dls::tableau<PatchLabel> &patchs_labels, int index)
{
	emets(OP_BRANCHE);
	emets(site);
	emets(0);

	auto patch = PatchLabel();
	patch.index_label = index;
	patch.adresse = static_cast<int>(compte - 4);
	patchs_labels.pousse(patch);
}

void Chunk::emets_branche_condition(NoeudExpression *site, dls::tableau<PatchLabel> &patchs_labels, int index_label_si_vrai, int index_label_si_faux)
{
	emets(OP_BRANCHE_CONDITION);
	emets(site);
	emets(0);
	emets(0);

	auto patch = PatchLabel();
	patch.index_label = index_label_si_vrai;
	patch.adresse = static_cast<int>(compte - 8);
	patchs_labels.pousse(patch);

	patch = PatchLabel();
	patch.index_label = index_label_si_faux;
	patch.adresse = static_cast<int>(compte - 4);
	patchs_labels.pousse(patch);
}

void Chunk::emets_label(NoeudExpression *site, int index)
{
	if (decalages_labels.taille() <= index) {
		decalages_labels.redimensionne(index + 1);
	}

	decalages_labels[index] = static_cast<int>(compte);
	emets(OP_LABEL);
	emets(site);
	emets(index);
}

/* ************************************************************************** */

static octet_t converti_op_binaire(OperateurBinaire::Genre genre)
{
	switch (genre) {
		case OperateurBinaire::Genre::Addition:
		{
			return OP_AJOUTE;
		}
		case OperateurBinaire::Genre::Addition_Reel:
		{
			return OP_AJOUTE_REEL;
		}
		case OperateurBinaire::Genre::Soustraction:
		{
			return OP_SOUSTRAIT;
		}
		case OperateurBinaire::Genre::Soustraction_Reel:
		{
			return OP_SOUSTRAIT_REEL;
		}
		case OperateurBinaire::Genre::Multiplication:
		{
			return OP_MULTIPLIE;
		}
		case OperateurBinaire::Genre::Multiplication_Reel:
		{
			return OP_MULTIPLIE_REEL;
		}
		case OperateurBinaire::Genre::Division_Naturel:
		{
			return OP_DIVISE;
		}
		case OperateurBinaire::Genre::Division_Relatif:
		{
			return OP_DIVISE_RELATIF;
		}
		case OperateurBinaire::Genre::Division_Reel:
		{
			return OP_DIVISE_REEL;
		}
		case OperateurBinaire::Genre::Reste_Naturel:
		{
			return OP_RESTE_NATUREL;
		}
		case OperateurBinaire::Genre::Reste_Relatif:
		{
			return OP_RESTE_RELATIF;
		}
		case OperateurBinaire::Genre::Comp_Egal:
		{
			return OP_COMP_EGAL;
		}
		case OperateurBinaire::Genre::Comp_Inegal:
		{
			return OP_COMP_INEGAL;
		}
		case OperateurBinaire::Genre::Comp_Inf:
		{
			return OP_COMP_INF;
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal:
		{
			return OP_COMP_INF_EGAL;
		}
		case OperateurBinaire::Genre::Comp_Sup:
		{
			return OP_COMP_SUP;
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal:
		{
			return OP_COMP_SUP_EGAL;
		}
		case OperateurBinaire::Genre::Comp_Inf_Nat:
		{
			return OP_COMP_INF_NATUREL;
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal_Nat:
		{
			return OP_COMP_INF_EGAL_NATUREL;
		}
		case OperateurBinaire::Genre::Comp_Sup_Nat:
		{
			return OP_COMP_SUP_NATUREL;
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal_Nat:
		{
			return OP_COMP_SUP_EGAL_NATUREL;
		}
		case OperateurBinaire::Genre::Comp_Egal_Reel:
		{
			return OP_COMP_EGAL_REEL;
		}
		case OperateurBinaire::Genre::Comp_Inegal_Reel:
		{
			return OP_COMP_INEGAL_REEL;
		}
		case OperateurBinaire::Genre::Comp_Inf_Reel:
		{
			return OP_COMP_INF_REEL;
		}
		case OperateurBinaire::Genre::Comp_Inf_Egal_Reel:
		{
			return OP_COMP_INF_EGAL_REEL;
		}
		case OperateurBinaire::Genre::Comp_Sup_Reel:
		{
			return OP_COMP_SUP_REEL;
		}
		case OperateurBinaire::Genre::Comp_Sup_Egal_Reel:
		{
			return OP_COMP_EGAL_REEL;
		}
		case OperateurBinaire::Genre::Et_Logique:
		{
			return OP_ET_LOGIQUE;
		}
		case OperateurBinaire::Genre::Ou_Logique:
		{
			return OP_OU_LOGIQUE;
		}
		case OperateurBinaire::Genre::Et_Binaire:
		{
			return OP_ET_BINAIRE;
		}
		case OperateurBinaire::Genre::Ou_Binaire:
		{
			return OP_OU_BINAIRE;
		}
		case OperateurBinaire::Genre::Ou_Exclusif:
		{
			return OP_OU_EXCLUSIF;
		}
		case OperateurBinaire::Genre::Dec_Gauche:
		{
			return OP_DEC_GAUCHE;
		}
		case OperateurBinaire::Genre::Dec_Droite_Arithm:
		{
			return OP_DEC_DROITE_ARITHM;
		}
		case OperateurBinaire::Genre::Dec_Droite_Logique:
		{
			return OP_DEC_DROITE_LOGIQUE;
		}
		case OperateurBinaire::Genre::Invalide:
		{
			return static_cast<octet_t>(-1);
		}
	}

	return static_cast<octet_t>(-1);
}

static long instruction_simple(const char *nom, long decalage, std::ostream &os)
{
	os << nom << '\n';
	return decalage + 1;
}

template <typename T>
static long instruction_1d(Chunk const &chunk, const char *nom, long decalage, std::ostream &os)
{
	decalage += 1;
	auto index = *reinterpret_cast<T *>(&chunk.code[decalage]);
	os << nom << ' ' << index << '\n';
	return decalage + static_cast<long >(sizeof(T));
}

template <typename T1, typename T2>
static long instruction_2d(Chunk const &chunk, const char *nom, long decalage, std::ostream &os)
{
	decalage += 1;
	auto v1 = *reinterpret_cast<T1 *>(&chunk.code[decalage]);
	decalage += static_cast<long >(sizeof(T1));
	auto v2 = *reinterpret_cast<T2 *>(&chunk.code[decalage]);
	os << nom << ' ' << v1 << ", " << v2 << "\n";
	return decalage + static_cast<long >(sizeof(T2));
}

template <typename T1, typename T2, typename T3>
static long instruction_3d(Chunk const &chunk, const char *nom, long decalage, std::ostream &os)
{
	decalage += 1;
	auto v1 = *reinterpret_cast<T1 *>(&chunk.code[decalage]);
	decalage += static_cast<long >(sizeof(T1));
	auto v2 = *reinterpret_cast<T2 *>(&chunk.code[decalage]);
	decalage += static_cast<long >(sizeof(T2));
	auto v3 = *reinterpret_cast<T3 *>(&chunk.code[decalage]);
	os << nom << ' ' << v1 << ", " << v2 << ", " << v3 << "\n";
	return decalage + static_cast<long >(sizeof(T3));
}

long desassemble_instruction(Chunk const &chunk, long decalage, std::ostream &os)
{
	os << std::setfill('0') << std::setw(4) << decalage << ' ';

	auto instruction = chunk.code[decalage];
	/* ignore le site */
	decalage += 8;

	switch (instruction) {
		case OP_RETOURNE:
		{
			return instruction_simple("OP_RETOURNE", decalage, os);
		}
		case OP_CONSTANTE:
		{
			decalage += 1;
			auto drapeaux = chunk.code[decalage];
			decalage += 1;
			os << "OP_CONSTANTE " << ' ';

#define LIS_CONSTANTE(type) \
			type v = *(reinterpret_cast<type *>(&chunk.code[decalage])); \
			os << v; \
			decalage += (drapeaux >> 3);

			switch (drapeaux) {
				case CONSTANTE_ENTIER_RELATIF | BITS_8:
				{
					LIS_CONSTANTE(char);
					break;
				}
				case CONSTANTE_ENTIER_RELATIF | BITS_16:
				{
					LIS_CONSTANTE(short);
					break;
				}
				case CONSTANTE_ENTIER_RELATIF | BITS_32:
				{
					LIS_CONSTANTE(int);
					break;
				}
				case CONSTANTE_ENTIER_RELATIF | BITS_64:
				{
					LIS_CONSTANTE(long);
					break;
				}
				case CONSTANTE_ENTIER_NATUREL | BITS_8:
				{
					// erreur de compilation pour transtype inutile avec drapeaux stricts
					os << static_cast<long>(chunk.code[decalage]);
					decalage += 1;
					break;
				}
				case CONSTANTE_ENTIER_NATUREL | BITS_16:
				{
					LIS_CONSTANTE(unsigned short);
					break;
				}
				case CONSTANTE_ENTIER_NATUREL | BITS_32:
				{
					LIS_CONSTANTE(unsigned int);
					break;
				}
				case CONSTANTE_ENTIER_NATUREL | BITS_64:
				{
					LIS_CONSTANTE(unsigned long);
					break;
				}
				case CONSTANTE_NOMBRE_REEL | BITS_32:
				{
					LIS_CONSTANTE(float);
					break;
				}
				case CONSTANTE_NOMBRE_REEL | BITS_64:
				{
					LIS_CONSTANTE(double);
					break;
				}
			}

#undef LIS_CONSTANTE
			os << '\n';
			return decalage;
		}
		case OP_CHAINE_CONSTANTE:
		{
			os << "OP_CHAINE_CONSTANTE\n";
			return decalage + 17;
		}
		case OP_AJOUTE:
		case OP_AJOUTE_REEL:
		case OP_SOUSTRAIT:
		case OP_SOUSTRAIT_REEL:
		case OP_MULTIPLIE:
		case OP_MULTIPLIE_REEL:
		case OP_DIVISE:
		case OP_DIVISE_RELATIF:
		case OP_DIVISE_REEL:
		case OP_RESTE_NATUREL:
		case OP_RESTE_RELATIF:
		case OP_COMP_EGAL:
		case OP_COMP_INEGAL:
		case OP_COMP_INF:
		case OP_COMP_INF_EGAL:
		case OP_COMP_SUP:
		case OP_COMP_SUP_EGAL:
		case OP_COMP_INF_NATUREL:
		case OP_COMP_INF_EGAL_NATUREL:
		case OP_COMP_SUP_NATUREL:
		case OP_COMP_SUP_EGAL_NATUREL:
		case OP_COMP_EGAL_REEL:
		case OP_COMP_INEGAL_REEL:
		case OP_COMP_INF_REEL:
		case OP_COMP_INF_EGAL_REEL:
		case OP_COMP_SUP_REEL:
		case OP_COMP_SUP_EGAL_REEL:
		case OP_ET_LOGIQUE:
		case OP_OU_LOGIQUE:
		case OP_ET_BINAIRE:
		case OP_OU_BINAIRE:
		case OP_OU_EXCLUSIF:
		case OP_DEC_GAUCHE:
		case OP_DEC_DROITE_ARITHM:
		case OP_DEC_DROITE_LOGIQUE:
		case OP_LABEL:
		case OP_BRANCHE:
		case OP_ASSIGNE:
		case OP_CHARGE:
		case OP_REFERENCE_GLOBALE:
		case OP_REFERENCE_VARIABLE:
		case OP_REFERENCE_MEMBRE:
		case OP_ACCEDE_INDEX:
		case OP_APPEL_POINTEUR:
		case OP_COMPLEMENT_REEL:
		case OP_COMPLEMENT_ENTIER:
		case OP_NON_BINAIRE:
		{
			return instruction_1d<int>(chunk, chaine_code_operation(instruction), decalage, os);
		}
		case OP_BRANCHE_CONDITION:
		{
			return instruction_2d<int, int>(chunk, chaine_code_operation(instruction), decalage, os);
		}
		case OP_ALLOUE:
		{
			decalage += 1;
			auto v1 = *reinterpret_cast<Type **>(&chunk.code[decalage]);
			decalage += static_cast<long >(sizeof(Type *));
			auto v2 = *reinterpret_cast<IdentifiantCode **>(&chunk.code[decalage]);
			os << chaine_code_operation(instruction) << ' ' << chaine_type(v1) << ", " << v2 << "\n";
			return decalage + static_cast<long >(sizeof(IdentifiantCode *));
		}
		case OP_APPEL:
		case OP_APPEL_EXTERNE:
		{
			return instruction_3d<void *, int, void *>(chunk, chaine_code_operation(instruction), decalage, os);
		}
		case OP_AUGMENTE_NATUREL:
		case OP_DIMINUE_NATUREL:
		case OP_AUGMENTE_RELATIF:
		case OP_DIMINUE_RELATIF:
		case OP_AUGMENTE_REEL:
		case OP_DIMINUE_REEL:
		{
			return instruction_2d<int, int>(chunk, chaine_code_operation(instruction), decalage, os);
		}
		default:
		{
			os << "Code Opération inconnu : " << instruction << '\n';
			return decalage + 1;
		}
	}
}

void desassemble(const Chunk &chunk, const char *nom, std::ostream &os)
{
	os << "== " << nom << " ==\n";
	for (auto decalage = 0l; decalage < chunk.compte;) {
		decalage = desassemble_instruction(chunk, decalage, os);
	}
}

ffi_type *converti_type_ffi(Type *type)
{
	switch (type->genre) {
		case GenreType::POLYMORPHIQUE:
		{
			assert_rappel(false, [&]() { std::cerr << "Type polymorphique dans la conversion FFI\n"; });
			return static_cast<ffi_type *>(nullptr);
		}
		case GenreType::BOOL:
		case GenreType::OCTET:
		{
			return &ffi_type_uint8;
		}
		case GenreType::CHAINE:
		{
			static ffi_type *types_elements[] = {
				&ffi_type_pointer,
				&ffi_type_sint64,
				nullptr
			};

			static ffi_type type_ffi_chaine = {
				0,
				0,
				FFI_TYPE_STRUCT,
				types_elements
			};

			return &type_ffi_chaine;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			static ffi_type *types_elements[] = {
				&ffi_type_pointer,
				&ffi_type_sint64,
				&ffi_type_sint64,
				nullptr
			};

			static ffi_type type_ffi_tableau = {
				0,
				0,
				FFI_TYPE_STRUCT,
				types_elements
			};

			return &type_ffi_tableau;
		}
		case GenreType::EINI:
		{
			static ffi_type *types_elements[] = {
				&ffi_type_pointer,
				&ffi_type_pointer,
				nullptr
			};

			static ffi_type type_ffi_eini = {
				0,
				0,
				FFI_TYPE_STRUCT,
				types_elements
			};

			return &type_ffi_eini;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			return &ffi_type_sint32;
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				return &ffi_type_uint8;
			}

			if (type->taille_octet == 2) {
				return &ffi_type_uint16;
			}

			if (type->taille_octet == 4) {
				return &ffi_type_uint32;
			}

			if (type->taille_octet == 8) {
				return &ffi_type_uint64;
			}

			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				return &ffi_type_sint8;
			}

			if (type->taille_octet == 2) {
				return &ffi_type_sint16;
			}

			if (type->taille_octet == 4) {
				return &ffi_type_sint32;
			}

			if (type->taille_octet == 8) {
				return &ffi_type_sint64;
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return &ffi_type_uint16;
			}

			if (type->taille_octet == 4) {
				return &ffi_type_float;
			}

			if (type->taille_octet == 8) {
				return &ffi_type_double;
			}

			break;
		}
		case GenreType::RIEN:
		{
			return &ffi_type_void;
		}
		case GenreType::POINTEUR:
		case GenreType::REFERENCE:
		case GenreType::FONCTION:
		{
			return &ffi_type_pointer;
		}
		case GenreType::STRUCTURE:
		{
			// non supporté pour le moment, nous devrions uniquement passer des pointeurs
			break;
		}
		case GenreType::OPAQUE:
		{
			// non supporté pour le moment
			break;
		}
		case GenreType::UNION:
		{
			auto type_union = type->comme_union();

			if (type_union->est_nonsure) {
				return converti_type_ffi(type_union->type_le_plus_grand);
			}

			// non supporté
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			return converti_type_ffi(type->comme_enum()->type_donnees);
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			return &ffi_type_sint64;
		}
		case GenreType::VARIADIQUE:
		case GenreType::TABLEAU_FIXE:
		{
			// ces types là ne sont pas supporté dans FFI
			break;
		}
	}

	return static_cast<ffi_type *>(nullptr);
}

static auto trouve_fonction_compilatrice(IdentifiantCode *ident)
{
#define COMPARE_IDENT(ident_fonction, fonction) \
	if (ident == ident_fonction) {\
		return reinterpret_cast<void(*)()>(fonction); \
	}

	COMPARE_IDENT(ID::ajoute_chaine_a_la_compilation, compilatrice_ajoute_chaine_compilation);
	COMPARE_IDENT(ID::ajoute_fichier_a_la_compilation, compilatrice_ajoute_fichier_compilation);
	COMPARE_IDENT(ID::ajoute_chaine_au_module, ajoute_chaine_au_module);
	COMPARE_IDENT(ID::compilatrice_ajourne_options, ajourne_options_compilation);
	COMPARE_IDENT(ID::compilatrice_obtiens_options, obtiens_options_compilation);
	COMPARE_IDENT(ID::fonction_test_variadique_externe, fonction_test_variadique_externe);
	COMPARE_IDENT(ID::compilatrice_attend_message, compilatrice_attend_message);
	COMPARE_IDENT(ID::compilatrice_commence_interception, compilatrice_commence_interception);
	COMPARE_IDENT(ID::compilatrice_termine_interception, compilatrice_termine_interception);
	COMPARE_IDENT(ID::compilatrice_rapporte_erreur, compilatrice_rapporte_erreur);
	COMPARE_IDENT(ID::compilatrice_lexe_fichier, compilatrice_lexe_fichier);
	COMPARE_IDENT(ID::compilatrice_espace_courant, compilatrice_espace_courant);
	COMPARE_IDENT(ID::demarre_un_espace_de_travail, demarre_un_espace_de_travail);
	COMPARE_IDENT(ID::espace_defaut_compilation, espace_defaut_compilation);

	std::cerr << "Fonction compilatrice inconnue !\n";
	return static_cast<void(*)()>(nullptr);
}

/* ************************************************************************** */

struct ConvertisseuseRI {
	MachineVirtuelle *mv = nullptr;
	dls::tableau<PatchLabel> patchs_labels{};
	dls::pile<int> pile_taille{};
	int dernier_decalage_pile = 0;

	ConvertisseuseRI(MachineVirtuelle *mv_);

	COPIE_CONSTRUCT(ConvertisseuseRI);

	void genere_code_binaire_pour_instruction(Instruction *instruction, Chunk &chunk, bool pour_operande);

	void genere_code_binaire_pour_constante(AtomeConstante *constante, Chunk &chunk);

	void genere_code_binaire_pour_initialisation_globale(AtomeConstante *constante, int decalage, int ou_patcher);

	void genere_code_binaire_pour_atome(Atome *atome, Chunk &chunk, bool pour_operande);
};

void genere_code_binaire_pour_fonction(AtomeFonction *fonction, MachineVirtuelle *mv)
{
	/* les fonctions implicites (p.e. initialisation de types) n'ont pas de déclaration */
	if (fonction->decl && fonction->decl->est_externe) {
		auto &donnees_externe = fonction->donnees_externe;

		if (fonction->decl->possede_drapeau(COMPILATRICE)) {
			donnees_externe.ptr_fonction = trouve_fonction_compilatrice(fonction->decl->ident);
		}
		else {
			donnees_externe.ptr_fonction = mv->trouve_symbole(fonction->decl->ident);
		}

		if (fonction->decl->est_variadique) {
			// les fonctions variadiques doivent être préparées pour chaque appel
			return;
		}

		auto type_fonction = fonction->type->comme_fonction();
		donnees_externe.types_entrees.reserve(type_fonction->types_entrees.taille);

		POUR (type_fonction->types_entrees) {
			donnees_externe.types_entrees.pousse(converti_type_ffi(it));
		}

		auto type_ffi_sortie = converti_type_ffi(type_fonction->types_sorties[0]);
		auto nombre_arguments = static_cast<unsigned>(donnees_externe.types_entrees.taille());
		auto ptr_types_entrees = donnees_externe.types_entrees.donnees();

		auto status = ffi_prep_cif(&donnees_externe.cif, FFI_DEFAULT_ABI, nombre_arguments, type_ffi_sortie, ptr_types_entrees);

		if (status != FFI_OK) {
			std::cerr << "Impossible de préparer pour FFI !\n";
			return;
		}

		return;
	}

	auto convertisseuse = ConvertisseuseRI{mv};

	auto &chunk = fonction->chunk;

	POUR (fonction->params_entrees) {
		auto alloc = it->comme_instruction()->comme_alloc();
		auto type_pointe = alloc->type->comme_pointeur()->type_pointe;
		auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
		alloc->index_locale = static_cast<int>(chunk.locales.taille());
		chunk.locales.pousse({ alloc->ident, alloc->type, adresse });
	}

	// À FAIRE : l'optimisation pour la réutilisation de la mémoire des locales en se basant sur la durée de vie de celles-ci ne fonctionne pas
	//           il existe des superposition partiells entre certaines variables
	//           lors de la dernière investigation, il semberait que les instructions de retours au milieu des fonctions y soient pour quelque chose
	//           pour le moment désactive cet optimisation et alloue de l'espace pour toutes les variables au début de chaque fonction.
#undef OPTIMISE_ALLOCS

#ifndef OPTIMISE_ALLOCS
	POUR (fonction->instructions) {
		if (it->est_alloc()) {
			auto alloc = it->comme_alloc();
			auto type_pointe = alloc->type->comme_pointeur()->type_pointe;
			auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
			alloc->index_locale = static_cast<int>(chunk.locales.taille());
			chunk.locales.pousse({ alloc->ident, alloc->type, adresse });
		}
	}
#endif

	POUR (fonction->instructions) {
		// génère le code binaire depuis les instructions « racines » (assignation, retour, allocation, appel, et controle de flux).
		auto est_inst_racine = dls::outils::est_element(
					it->genre,
					Instruction::Genre::ALLOCATION,
					Instruction::Genre::APPEL,
					Instruction::Genre::BRANCHE,
					Instruction::Genre::BRANCHE_CONDITION,
					Instruction::Genre::LABEL,
					Instruction::Genre::RETOUR,
					Instruction::Genre::STOCKE_MEMOIRE);

		if (!est_inst_racine) {
			continue;
		}

		convertisseuse.genere_code_binaire_pour_instruction(it, chunk, false);
	}

	POUR (convertisseuse.patchs_labels) {
		auto decalage = chunk.decalages_labels[it.index_label];
		*reinterpret_cast<int *>(&chunk.code[it.adresse]) = decalage;
	}

	//desassemble(chunk, fonction->nom.c_str(), std::cerr);
}

ConvertisseuseRI::ConvertisseuseRI(MachineVirtuelle *mv_)
	: mv(mv_)
{}

void ConvertisseuseRI::genere_code_binaire_pour_instruction(Instruction *instruction, Chunk &chunk, bool pour_operande)
{
	switch (instruction->genre) {
		case Instruction::Genre::INVALIDE:
		{
			return;
		}
		case Instruction::Genre::LABEL:
		{
			auto label = instruction->comme_label();
			chunk.emets_label(label->site, label->id);
			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto branche = instruction->comme_branche();
			chunk.emets_branche(branche->site, patchs_labels, branche->label->id);
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto branche = instruction->comme_branche_cond();
			genere_code_binaire_pour_atome(branche->condition, chunk, true);
			chunk.emets_branche_condition(branche->site, patchs_labels, branche->label_si_vrai->id, branche->label_si_faux->id);
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto alloc = instruction->comme_alloc();

			if (pour_operande) {
				chunk.emets_reference_variable(alloc->site, alloc->index_locale);
			}
#ifdef OPTIMISE_ALLOCS
			else {
				if (alloc->decalage_pile > dernier_decalage_pile) {
					dernier_decalage_pile = alloc->decalage_pile;
					pile_taille.empile(chunk.taille_allouee);
				}
				else if (alloc->decalage_pile < dernier_decalage_pile) {
					dernier_decalage_pile = alloc->decalage_pile;
					chunk.taille_allouee = pile_taille.depile();
				}

				auto type_pointe = alloc->type->comme_pointeur()->type_pointe;
				auto adresse = chunk.emets_allocation(alloc->site, type_pointe, alloc->ident);
				alloc->index_locale = static_cast<int>(chunk.locales.taille());
				chunk.locales.pousse({ alloc->ident, alloc->type, adresse });
			}
#endif

			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto charge = instruction->comme_charge();
			genere_code_binaire_pour_atome(charge->chargee, chunk, true);
			chunk.emets_charge(charge->site, charge->type);
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto stocke = instruction->comme_stocke_mem();
			genere_code_binaire_pour_atome(stocke->valeur, chunk, true);
			// l'adresse de la valeur doit être au sommet de la pile lors de l'assignation
			genere_code_binaire_pour_atome(stocke->ou, chunk, true);
			chunk.emets_assignation(stocke->site, stocke->valeur->type);
			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto appel = instruction->comme_appel();

			// évite de générer deux fois le code pour les appels : une fois dans la boucle sur les instructions, une fois pour l'opérande
			// les fonctions retournant « rien » ne peuvent être opérandes
			if (appel->type->genre != GenreType::RIEN && !pour_operande) {
				return;
			}

			auto appelee = appel->appele;
			auto taille_arguments = 0u;

			POUR (appel->args) {
				genere_code_binaire_pour_atome(it, chunk, true);

				if (it->type->genre == GenreType::ENTIER_CONSTANT) {
					taille_arguments += 4;
				}
				else {
					taille_arguments += it->type->taille_octet;
				}
			}

			if (appelee->genre_atome == Atome::Genre::FONCTION) {
				auto atome_appelee = static_cast<AtomeFonction *>(appelee);

				if (atome_appelee->est_externe) {
					chunk.emets_appel_externe(appel->site, atome_appelee, taille_arguments, appel);
				}
				else {
					chunk.emets_appel(appel->site, atome_appelee, taille_arguments, appel);
				}
			}
			else {
				genere_code_binaire_pour_atome(appelee, chunk, true);
				chunk.emets_appel_pointeur(appel->site, taille_arguments, appel);
			}

			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto retour = instruction->comme_retour();

			if (retour->valeur) {
				genere_code_binaire_pour_atome(retour->valeur, chunk, true);
			}

			chunk.emets(OP_RETOURNE);
			chunk.emets(retour->site);
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto transtype = instruction->comme_transtype();
			auto valeur = transtype->valeur;

			genere_code_binaire_pour_atome(valeur, chunk, true);

			switch (transtype->op) {
				case TypeTranstypage::BITS:
				case TypeTranstypage::DEFAUT:
				case TypeTranstypage::POINTEUR_VERS_ENTIER:
				case TypeTranstypage::ENTIER_VERS_POINTEUR:
				case TypeTranstypage::REEL_VERS_ENTIER:
				case TypeTranstypage::ENTIER_VERS_REEL:
				{
					break;
				}
				case TypeTranstypage::AUGMENTE_REEL:
				{
					chunk.emets(OP_AUGMENTE_REEL);
					chunk.emets(transtype->site);
					chunk.emets(4);
					chunk.emets(8);
					break;
				}
				case TypeTranstypage::AUGMENTE_NATUREL:
				{
					chunk.emets(OP_AUGMENTE_NATUREL);
					chunk.emets(transtype->site);
					chunk.emets(valeur->type->taille_octet);
					chunk.emets(transtype->type->taille_octet);
					break;
				}
				case TypeTranstypage::AUGMENTE_RELATIF:
				{
					chunk.emets(OP_AUGMENTE_RELATIF);
					chunk.emets(transtype->site);
					chunk.emets(valeur->type->taille_octet);
					chunk.emets(transtype->type->taille_octet);
					break;
				}
				case TypeTranstypage::DIMINUE_REEL:
				{
					chunk.emets(OP_DIMINUE_REEL);
					chunk.emets(transtype->site);
					chunk.emets(8);
					chunk.emets(4);
					break;
				}
				case TypeTranstypage::DIMINUE_NATUREL:
				{
					chunk.emets(OP_DIMINUE_NATUREL);
					chunk.emets(transtype->site);
					chunk.emets(valeur->type->taille_octet);
					chunk.emets(transtype->type->taille_octet);
					break;
				}
				case TypeTranstypage::DIMINUE_RELATIF:
				{
					chunk.emets(OP_DIMINUE_RELATIF);
					chunk.emets(transtype->site);
					chunk.emets(valeur->type->taille_octet);
					chunk.emets(transtype->type->taille_octet);
					break;
				}
			}

			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto index = instruction->comme_acces_index();
			auto type_pointeur = index->type->comme_pointeur();
			genere_code_binaire_pour_atome(index->index, chunk, true);
			genere_code_binaire_pour_atome(index->accede, chunk, true);

			if (index->accede->genre_atome == Atome::Genre::INSTRUCTION) {
				auto accede = index->accede->comme_instruction();
				auto type_accede = accede->type->comme_pointeur()->type_pointe;

				// l'accédé est le pointeur vers le pointeur, donc déréférence-le
				if (type_accede->genre == GenreType::POINTEUR) {
					chunk.emets_charge(index->site, type_pointeur);
				}
			}

			chunk.emets_acces_index(index->site, type_pointeur->type_pointe);
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto membre = instruction->comme_acces_membre();
			auto index_membre = static_cast<long>(static_cast<AtomeValeurConstante *>(membre->index)->valeur.valeur_entiere);

			auto type_pointeur = membre->accede->type->comme_pointeur();
			auto type_compose = static_cast<TypeCompose *>(type_pointeur->type_pointe);

			auto decalage = type_compose->membres[index_membre].decalage;
			genere_code_binaire_pour_atome(membre->accede, chunk, true);
			chunk.emets_reference_membre(membre->site, decalage);

			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto op_unaire = instruction->comme_op_unaire();
			auto type = op_unaire->valeur->type;

			genere_code_binaire_pour_atome(op_unaire->valeur, chunk, true);

			if (op_unaire->op == OperateurUnaire::Genre::Complement) {
				if (type->genre == GenreType::REEL) {
					chunk.emets(OP_COMPLEMENT_REEL);
					chunk.emets(op_unaire->site);
				}
				else {
					chunk.emets(OP_COMPLEMENT_ENTIER);
					chunk.emets(op_unaire->site);
				}
			}
			else if (op_unaire->op == OperateurUnaire::Genre::Non_Binaire) {
				chunk.emets(OP_NON_BINAIRE);
				chunk.emets(op_unaire->site);
			}


			if (type->genre == GenreType::ENTIER_CONSTANT) {
				chunk.emets(4);
			}
			else {
				chunk.emets(type->taille_octet);
			}

			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto op_binaire = instruction->comme_op_binaire();

			genere_code_binaire_pour_atome(op_binaire->valeur_gauche, chunk, true);
			genere_code_binaire_pour_atome(op_binaire->valeur_droite, chunk, true);

			auto op_comp = converti_op_binaire(op_binaire->op);
			chunk.emets(op_comp);
			chunk.emets(op_binaire->site);

			auto type_gauche = op_binaire->valeur_gauche->type;

			if (type_gauche->genre == GenreType::ENTIER_CONSTANT) {
				chunk.emets(4);
			}
			else {
				chunk.emets(type_gauche->taille_octet);
			}

			break;
		}
	}
}

void ConvertisseuseRI::genere_code_binaire_pour_constante(AtomeConstante *constante, Chunk &chunk)
{
	switch (constante->genre) {
		case AtomeConstante::Genre::VALEUR:
		{
			auto valeur_constante = static_cast<AtomeValeurConstante *>(constante);

			switch (valeur_constante->valeur.genre) {
				case AtomeValeurConstante::Valeur::Genre::NULLE:
				{
					chunk.emets_constante(0l);
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TYPE:
				{
					auto valeur_type = valeur_constante->valeur.type->index_dans_table_types;
					chunk.emets_constante(static_cast<long>(valeur_type));
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::ENTIERE:
				{
					auto valeur_entiere = valeur_constante->valeur.valeur_entiere;
					auto type = constante->type;

					if (type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
						if (type->taille_octet == 1) {
							chunk.emets_constante(static_cast<unsigned char>(valeur_entiere));
						}
						else if (type->taille_octet == 2) {
							chunk.emets_constante(static_cast<unsigned short>(valeur_entiere));
						}
						else if (type->taille_octet == 4) {
							chunk.emets_constante(static_cast<unsigned int>(valeur_entiere));
						}
						else if (type->taille_octet == 8) {
							chunk.emets_constante(static_cast<unsigned long>(valeur_entiere));
						}
					}
					else if (type->genre == GenreType::ENTIER_RELATIF) {
						if (type->taille_octet == 1) {
							chunk.emets_constante(static_cast<char>(valeur_entiere));
						}
						else if (type->taille_octet == 2) {
							chunk.emets_constante(static_cast<short>(valeur_entiere));
						}
						else if (type->taille_octet == 4) {
							chunk.emets_constante(static_cast<int>(valeur_entiere));
						}
						else if (type->taille_octet == 8) {
							chunk.emets_constante(static_cast<long>(valeur_entiere));
						}
					}
					else if (type->genre == GenreType::ENTIER_CONSTANT) {
						chunk.emets_constante(static_cast<int>(valeur_entiere));
					}
					else if (type->est_octet()) {
						chunk.emets_constante(static_cast<unsigned char>(valeur_entiere));
					}

					break;
				}
				case AtomeValeurConstante::Valeur::Genre::REELLE:
				{
					auto valeur_reele = valeur_constante->valeur.valeur_reelle;
					auto type = constante->type;

					if (type->taille_octet == 4) {
						chunk.emets_constante(static_cast<float>(valeur_reele));
					}
					else {
						chunk.emets_constante(valeur_reele);
					}

					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
				{
					auto valeur_bool = valeur_constante->valeur.valeur_booleenne;
					chunk.emets_constante(valeur_bool);
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::CARACTERE:
				{
					auto valeur_caractere = valeur_constante->valeur.valeur_entiere;
					chunk.emets_constante(static_cast<char>(valeur_caractere));
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
				{
					auto type = constante->type;
					auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

					if (type->genre == GenreType::CHAINE) {
						if (tableau_valeur[0]->genre == AtomeConstante::Genre::VALEUR) {
							// valeur nulle pour les chaines initilisées à zéro
							chunk.emets(OP_CHAINE_CONSTANTE);
							chunk.emets(nullptr); /* site */
							chunk.emets(nullptr);
							chunk.emets(0l);
						}
						else {
							auto acces_index = static_cast<AccedeIndexConstant *>(tableau_valeur[0]);
							auto globale_tableau = static_cast<AtomeGlobale *>(acces_index->accede);

							auto tableau = static_cast<AtomeValeurConstante *>(globale_tableau->initialisateur);

							auto chaine = kuri::chaine();
							chaine.pointeur = tableau->valeur.valeur_tdc.pointeur;
							chaine.taille = tableau->valeur.valeur_tdc.taille;

							chunk.emets(OP_CHAINE_CONSTANTE);
							chunk.emets(nullptr); /* site */
							chunk.emets(chaine.pointeur);
							chunk.emets(chaine.taille);

							// reférence globale, tableau
							// accède index
							// --> pointeur de la chaine
						}
					}
					else {
						auto type_compose = static_cast<TypeCompose *>(type);

						auto index_membre = 0;
						for (auto i = 0; i < type_compose->membres.taille; ++i) {
							if ((type_compose->membres[i].drapeaux & TypeCompose::Membre::EST_CONSTANT) != 0) {
								continue;
							}

							if (tableau_valeur[index_membre] != nullptr) {
								// À FAIRE : type tableau fixe
								genere_code_binaire_pour_atome(tableau_valeur[index_membre], chunk, true);
							}

							index_membre += 1;
						}
					}

					break;
				}
			}

			break;
		}
		case AtomeConstante::Genre::GLOBALE:
		{
			genere_code_binaire_pour_atome(constante, chunk, true);
			break;
		}
		case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
		{
			auto transtype = static_cast<TranstypeConstant *>(constante);
			genere_code_binaire_pour_constante(transtype->valeur, chunk);
			break;
		}
		default:
		{
			// À FAIRE
			break;
		}
	}
}

void ConvertisseuseRI::genere_code_binaire_pour_initialisation_globale(AtomeConstante *constante, int decalage, int ou_patcher)
{
	switch (constante->genre) {
		case AtomeConstante::Genre::VALEUR:
		{
			auto valeur_constante = static_cast<AtomeValeurConstante *>(constante);
			unsigned char *donnees = nullptr;

			if (ou_patcher == DONNEES_GLOBALES) {
				donnees = mv->donnees_globales.donnees() + decalage;
			}
			else {
				donnees = mv->donnees_constantes.donnees() + decalage;
			}

			switch (valeur_constante->valeur.genre) {
				case AtomeValeurConstante::Valeur::Genre::NULLE:
				{
					*reinterpret_cast<unsigned long *>(donnees) = 0;
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TYPE:
				{
					auto valeur_type = valeur_constante->valeur.type->index_dans_table_types;
					*reinterpret_cast<long *>(donnees) = static_cast<long>(valeur_type);
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::ENTIERE:
				{
					auto valeur_entiere = valeur_constante->valeur.valeur_entiere;
					auto type = constante->type;

					if (type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
						if (type->taille_octet == 1) {
							*donnees = static_cast<unsigned char>(valeur_entiere);
						}
						else if (type->taille_octet == 2) {
							*reinterpret_cast<unsigned short *>(donnees) = static_cast<unsigned short>(valeur_entiere);
						}
						else if (type->taille_octet == 4) {
							*reinterpret_cast<unsigned int *>(donnees) = static_cast<unsigned int>(valeur_entiere);
						}
						else if (type->taille_octet == 8) {
							*reinterpret_cast<unsigned long *>(donnees) = static_cast<unsigned long>(valeur_entiere);
						}
					}
					else if (type->genre == GenreType::ENTIER_RELATIF) {
						if (type->taille_octet == 1) {
							*reinterpret_cast<char *>(donnees) = static_cast<char>(valeur_entiere);
						}
						else if (type->taille_octet == 2) {
							*reinterpret_cast<short *>(donnees) = static_cast<short>(valeur_entiere);
						}
						else if (type->taille_octet == 4) {
							*reinterpret_cast<int *>(donnees) = static_cast<int>(valeur_entiere);
						}
						else if (type->taille_octet == 8) {
							*reinterpret_cast<long *>(donnees) = static_cast<long>(valeur_entiere);
						}
					}
					else if (type->genre == GenreType::ENTIER_CONSTANT) {
						*reinterpret_cast<int *>(donnees) = static_cast<int>(valeur_entiere);
					}

					break;
				}
				case AtomeValeurConstante::Valeur::Genre::REELLE:
				{
					auto valeur_reele = valeur_constante->valeur.valeur_reelle;
					auto type = constante->type;

					if (type->taille_octet == 4) {
						*reinterpret_cast<float *>(donnees) = static_cast<float>(valeur_reele);
					}
					else {
						*reinterpret_cast<double *>(donnees) = valeur_reele;
					}

					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
				{
					auto valeur_bool = valeur_constante->valeur.valeur_booleenne;
					*reinterpret_cast<char *>(donnees) = static_cast<char>(valeur_bool);
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::CARACTERE:
				{
					break;
				}
				case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
				{
					auto type = static_cast<TypeCompose *>(constante->type);
					auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

					auto index_membre = 0;
					for (auto i = 0; i < type->membres.taille; ++i) {
						if (type->membres[i].drapeaux & TypeCompose::Membre::EST_CONSTANT) {
							continue;
						}

						// les tableaux fixes ont une initialisation nulle
						if (tableau_valeur[index_membre] == nullptr) {
							index_membre += 1;
							continue;
						}

						//std::cerr << "Ajout du code pour le membre : " << type->membres[i].nom << '\n';

						auto type_membre = type->membres[i].type;

						auto decalage_membre = type->membres[i].decalage;

						if (type_membre->genre == GenreType::CHAINE) {
							auto valeur_chaine = static_cast<AtomeValeurConstante *>(tableau_valeur[index_membre]);
							auto acces_index = static_cast<AccedeIndexConstant *>(valeur_chaine->valeur.valeur_structure.pointeur[0]);
							auto globale_tableau = static_cast<AtomeGlobale *>(acces_index->accede);

							auto tableau = static_cast<AtomeValeurConstante *>(globale_tableau->initialisateur);

							auto chaine = kuri::chaine();
							chaine.pointeur = tableau->valeur.valeur_tdc.pointeur;
							chaine.taille = tableau->valeur.valeur_tdc.taille;

							auto donnees_ = mv->donnees_globales.donnees() + decalage + static_cast<int>(decalage_membre);
							*reinterpret_cast<char **>(donnees_) = chaine.pointeur;
							*reinterpret_cast<long *>(donnees_ + 8) = chaine.taille;
						}
						else if (type_membre->genre == GenreType::TABLEAU_DYNAMIQUE) {
							auto valeur_tableau = static_cast<AtomeValeurConstante *>(tableau_valeur[index_membre]);
							auto acces_index = static_cast<AccedeIndexConstant *>(valeur_tableau->valeur.valeur_structure.pointeur[0]);
							auto globale_tableau = static_cast<AtomeGlobale *>(acces_index->accede);

							auto tableau = static_cast<AtomeValeurConstante *>(globale_tableau->initialisateur);

							auto pointeur = tableau->valeur.valeur_tableau.pointeur;
							auto taille = tableau->valeur.valeur_tableau.taille;

							auto type_tableau = tableau->type->comme_tableau_fixe();
							auto type_pointe  = type_tableau->type_pointe;
							auto decalage_valeur = static_cast<int>(mv->donnees_constantes.taille());
							auto adresse_tableau = decalage_valeur;

							mv->donnees_constantes.redimensionne(mv->donnees_constantes.taille() + type_pointe->taille_octet * type_tableau->taille);

							for (auto j = 0; j < taille; ++j) {
								auto pointeur_valeur = pointeur[j];
								genere_code_binaire_pour_initialisation_globale(pointeur_valeur, decalage_valeur, DONNEES_CONSTANTES);
								decalage_valeur += static_cast<int>(type_pointe->taille_octet);
							}

							auto patch = PatchDonneesConstantes{};
							patch.ou = DONNEES_GLOBALES;
							patch.quoi = ADRESSE_CONSTANTE;
							patch.decalage_ou = decalage + static_cast<int>(decalage_membre);
							patch.decalage_quoi = adresse_tableau;

							mv->patchs_donnees_constantes.pousse(patch);

							auto donnees_ = mv->donnees_globales.donnees() + decalage + static_cast<int>(decalage_membre);
							*reinterpret_cast<long *>(donnees_ + 8) = taille;
						}
						else {
							genere_code_binaire_pour_initialisation_globale(tableau_valeur[index_membre], decalage + static_cast<int>(decalage_membre), ou_patcher);
						}

						index_membre += 1;
					}

					break;
				}
			}

			break;
		}
		case AtomeConstante::Genre::GLOBALE:
		{
			auto atome_globale = static_cast<AtomeGlobale *>(constante);

			if (atome_globale->index == -1) {
				auto type_globale = atome_globale->type->comme_pointeur()->type_pointe;
				atome_globale->index = mv->ajoute_globale(type_globale, atome_globale->ident);

				if (atome_globale->est_constante) {
					auto globale = mv->globales[atome_globale->index];
					auto initialisateur = atome_globale->initialisateur;
					genere_code_binaire_pour_initialisation_globale(initialisateur, globale.adresse, DONNEES_GLOBALES);
				}
			}

			auto globale = mv->globales[atome_globale->index];

			auto patch = PatchDonneesConstantes{};
			patch.ou = ou_patcher;
			patch.quoi = ADRESSE_GLOBALE;
			patch.decalage_ou = decalage;
			patch.decalage_quoi = globale.adresse;

			mv->patchs_donnees_constantes.pousse(patch);

			break;
		}
		case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
		{
			auto transtype = static_cast<TranstypeConstant *>(constante);
			genere_code_binaire_pour_initialisation_globale(transtype->valeur, decalage, ou_patcher);
			break;
		}
		case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
		{
			// À FAIRE
			//assert_rappel(false, []() { std::cerr << "Les opérations unaires constantes ne sont pas implémentées dans le code binaire\n"; });
			break;
		}
		case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
		{
			// À FAIRE
			//assert_rappel(false, []() { std::cerr << "Les opérations binaires constantes ne sont pas implémentées dans le code binaire\n"; });
			break;
		}
		case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
		{
			// À FAIRE
			//assert_rappel(false, []() { std::cerr << "Les indexages constants ne sont pas implémentés dans le code binaire\n"; });
			break;
		}
	}
}

void ConvertisseuseRI::genere_code_binaire_pour_atome(Atome *atome, Chunk &chunk, bool pour_operande)
{
	switch (atome->genre_atome) {
		case Atome::Genre::GLOBALE:
		{
			auto atome_globale = static_cast<AtomeGlobale *>(atome);

			if (atome_globale->index == -1) {
				auto type_globale = atome_globale->type->comme_pointeur()->type_pointe;
				atome_globale->index = mv->ajoute_globale(type_globale, atome_globale->ident);

				if (atome_globale->est_constante) {
					auto globale = mv->globales[atome_globale->index];
					auto initialisateur = atome_globale->initialisateur;
					genere_code_binaire_pour_initialisation_globale(initialisateur, globale.adresse, DONNEES_GLOBALES);
				}
			}

			chunk.emets_reference_globale(nullptr, atome_globale->index);
			break;
		}
		case Atome::Genre::FONCTION:
		{
			// l'adresse pour les pointeurs de fonctions
			if (pour_operande) {
				chunk.emets_constante(reinterpret_cast<long>(atome));
			}

			break;
		}
		case Atome::Genre::INSTRUCTION:
		{
			genere_code_binaire_pour_instruction(atome->comme_instruction(), chunk, pour_operande);
			break;
		}
		case Atome::Genre::CONSTANTE:
		{
			genere_code_binaire_pour_constante(static_cast<AtomeConstante *>(atome), chunk);
			break;
		}
	}
}

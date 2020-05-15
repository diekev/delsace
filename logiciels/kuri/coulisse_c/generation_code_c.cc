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

#include "generation_code_c.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/enchaineuse.hh"

#include "compilation/assembleuse_arbre.h"
#include "compilation/broyage.hh"
#include "compilation/erreur.h"

#include "representation_intermediaire/constructrice_ri.hh"

namespace noeud {

static inline char char_depuis_hex(char hex)
{
	return "0123456789ABCDEF"[static_cast<int>(hex)];
}

/* ************************************************************************** */

static void cree_typedef(Type *type, Enchaineuse &enchaineuse)
{
	auto const &nom_broye = nom_broye_type(type);

	enchaineuse << "// " << chaine_type(type) << '\n';

	switch (type->genre) {
		case GenreType::INVALIDE:
		{
			break;
		}
		case GenreType::ERREUR:
		case GenreType::ENUM:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			auto nom_broye_type_donnees = nom_broye_type(type_enum->type_donnees);

			enchaineuse << "typedef " << nom_broye_type_donnees << ' ' << nom_broye << ";\n";
			break;
		}
		case GenreType::BOOL:
		{
			enchaineuse << "typedef unsigned char " << nom_broye << ";\n";
			break;
		}
		case GenreType::OCTET:
		{
			enchaineuse << "typedef unsigned char " << nom_broye << ";\n";
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				enchaineuse << "typedef unsigned char " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 2) {
				enchaineuse << "typedef unsigned short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "typedef unsigned int " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "typedef unsigned long " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				enchaineuse << "typedef char " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 2) {
				enchaineuse << "typedef short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "typedef int " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "typedef long " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				enchaineuse << "typedef unsigned short " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "typedef float " << nom_broye << ";\n";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "typedef double " << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::REFERENCE:
		{
			auto type_pointe = static_cast<TypeReference *>(type)->type_pointe;
			enchaineuse << "typedef " << nom_broye_type(type_pointe) << "* " << nom_broye << ";\n";
			break;
		}
		case GenreType::POINTEUR:
		{
			auto type_pointe = static_cast<TypePointeur *>(type)->type_pointe;
			enchaineuse << "typedef " << nom_broye_type(type_pointe) << "* " << nom_broye << ";\n";
			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto nom_struct = broye_nom_simple(type_struct->nom);

			if (nom_struct != "pthread_mutex_t" && nom_struct != "pthread_cond_t" && nom_struct != "MY_CHARSET_INFO") {
				enchaineuse << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";
			}
			else {
				enchaineuse << "typedef " << nom_struct << ' ' << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = static_cast<TypeUnion *>(type);
			auto nom_struct = broye_nom_simple(type_struct->nom);
			auto decl = type_struct->decl;

			if (nom_struct != "pthread_mutex_t" && nom_struct != "pthread_cond_t") {
				if (decl->est_nonsure || decl->est_externe) {
					enchaineuse << "typedef union " << nom_struct << ' ' << nom_broye << ";\n";
				}
				else {
					enchaineuse << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";
				}
			}
			else {
				enchaineuse << "typedef " << nom_struct << ' ' << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_pointe = static_cast<TypeTableauFixe *>(type)->type_pointe;

			// [X][X]Type
			if (type_pointe->genre == GenreType::TABLEAU_FIXE) {
				auto type_tabl = static_cast<TypeTableauFixe *>(type_pointe);
				auto taille_tableau = type_tabl->taille;

				enchaineuse << "typedef " << nom_broye_type(type_tabl->type_pointe);
				enchaineuse << "(" << nom_broye << ')';
				enchaineuse << '[' << static_cast<TypeTableauFixe *>(type)->taille << ']';
				enchaineuse << '[' << taille_tableau << ']';
				enchaineuse << ";\n\n";
			}
			else {
				enchaineuse << "typedef " << nom_broye_type(type_pointe);
				enchaineuse << ' ' << nom_broye;
				enchaineuse << '[' << static_cast<TypeTableauFixe *>(type)->taille << ']';
				enchaineuse << ";\n\n";
			}

			break;
		}
		case GenreType::VARIADIQUE:
		{
			// les arguments variadiques sont transformés en tableaux, donc RÀF
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_pointe = static_cast<TypeTableauDynamique *>(type)->type_pointe;

			if (type_pointe == nullptr) {
				return;
			}

			enchaineuse << "typedef struct Tableau_" << nom_broye;

			enchaineuse << "{\n\t";

			if (type_pointe->genre == GenreType::TABLEAU_FIXE) {
				auto type_tabl = static_cast<TypeTableauFixe *>(type_pointe);
				auto taille_tableau = type_tabl->taille;
				enchaineuse << nom_broye_type(type_tabl->type_pointe) << " *pointeur[" << taille_tableau << "];";
			}
			else {
				enchaineuse << nom_broye_type(type_pointe) << " *pointeur;";
			}

			enchaineuse << "\n\tlong taille;\n" << "\tlong " << broye_nom_simple("capacité") << ";\n} " << nom_broye << ";\n\n";
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = static_cast<TypeFonction *>(type);

			auto prefixe = dls::chaine("");
			auto suffixe = dls::chaine("");

			auto nouveau_nom_broye = dls::chaine("Kf");
			nouveau_nom_broye += dls::vers_chaine(type_fonc->types_entrees.taille);

			if (type_fonc->types_sorties.taille > 1) {
				prefixe += "void (*";
			}
			else {
				auto const &nom_broye_dt = nom_broye_type(type_fonc->types_sorties[0]);
				prefixe += nom_broye_dt + " (*";
			}

			auto virgule = "(";

			POUR (type_fonc->types_entrees) {
				auto const &nom_broye_dt = nom_broye_type(it);

				suffixe += virgule;
				suffixe += nom_broye_dt;
				nouveau_nom_broye += nom_broye_dt;
				virgule = ",";
			}

			if (type_fonc->types_entrees.taille == 0) {
				suffixe += virgule;
				virgule = ",";
			}

			nouveau_nom_broye += dls::vers_chaine(type_fonc->types_sorties.taille);

			POUR (type_fonc->types_sorties) {
				auto const &nom_broye_dt = nom_broye_type(it);

				if (type_fonc->types_sorties.taille > 1) {
					suffixe += virgule;
					suffixe += nom_broye_dt;
				}

				nouveau_nom_broye += nom_broye_dt;
			}

			suffixe += ")";

			type->nom_broye = nouveau_nom_broye;

			nouveau_nom_broye = prefixe + nouveau_nom_broye + ")" + suffixe;

			enchaineuse << "typedef " << nouveau_nom_broye << ";\n\n";

			break;
		}
		case GenreType::EINI:
		{
			enchaineuse << "typedef eini " << nom_broye << ";\n";
			break;
		}
		case GenreType::RIEN:
		{
			enchaineuse << "typedef void " << nom_broye << ";\n";
			break;
		}
		case GenreType::CHAINE:
		{
			enchaineuse << "typedef chaine " << nom_broye << ";\n";
			break;
		}
	}
}

/* ************************************************************************** */

static void genere_declaration_structure(Enchaineuse &enchaineuse, NoeudStruct *decl)
{
	if (decl->est_externe) {
		return;
	}

	auto nom_broye = broye_nom_simple(decl->ident->nom);

	if (decl->est_union) {
		if (decl->est_nonsure) {
			enchaineuse << "typedef union " << nom_broye << "{\n";
		}
		else {
			enchaineuse << "typedef struct " << nom_broye << "{\n";
			enchaineuse << "int membre_actif;\n";
			enchaineuse << "union {\n";
		}
	}
	else {
		enchaineuse << "typedef struct " << nom_broye << "{\n";
	}

	auto type_compose = static_cast<TypeCompose *>(decl->type);

	POUR (type_compose->membres) {
		auto nom = broye_nom_simple(it.nom);
		enchaineuse << nom_broye_type(it.type) << ' ' << nom << ";\n";
	}

	if (decl->est_union && !decl->est_nonsure) {
		enchaineuse << "};\n";
	}

	enchaineuse << "} " << nom_broye << ";\n\n";
}

/* ************************************************************************** */

static bool peut_etre_dereference(Type *type)
{
	return type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::REFERENCE || type->genre == GenreType::POINTEUR;
}

static void genere_typedefs_recursifs(
		ContexteGenerationCode &contexte,
		Type *type,
		Enchaineuse &enchaineuse)
{
	if ((type->drapeaux & TYPEDEF_FUT_GENERE) != 0) {
		return;
	}

	if (peut_etre_dereference(type)) {
		auto type_deref = type_dereference_pour(type);

		/* argument variadique fonction externe */
		if (type_deref == nullptr) {
			return;
		}

		if (type_deref->genre == GenreType::VARIADIQUE) {
			type_deref = static_cast<TypeVariadique *>(type_deref)->type_pointe;

			/* dans la RI nous créons un pointeur vers le type des arguments ce
			 * qui peut inclure un pointeur vers un type variadique externe sans
			 * type */
			if (type_deref == nullptr) {
				return;
			}
		}

		if ((type_deref->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
			genere_typedefs_recursifs(contexte, type_deref, enchaineuse);
		}

		type_deref->drapeaux |= TYPEDEF_FUT_GENERE;
	}
	/* ajoute les types des paramètres et de retour des fonctions */
	else if (type->genre == GenreType::FONCTION) {
		auto type_fonc = static_cast<TypeFonction *>(type);

		POUR (type_fonc->types_entrees) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, enchaineuse);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}

		POUR (type_fonc->types_sorties) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(contexte, it, enchaineuse);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}
	}

	cree_typedef(type, enchaineuse);
	type->drapeaux |= TYPEDEF_FUT_GENERE;
}

static void genere_typedefs_pour_tous_les_types(
		ContexteGenerationCode &contexte,
		Enchaineuse &enchaineuse)
{
	POUR (contexte.typeuse.types_simples) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_structures) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_enums) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_tableaux_fixes) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_tableaux_dynamiques) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_pointeurs) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_references) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_fonctions) genere_typedefs_recursifs(contexte, it, enchaineuse);
	POUR (contexte.typeuse.types_unions) genere_typedefs_recursifs(contexte, it, enchaineuse);
}

// ----------------------------------------------

static void genere_code_debut_fichier(
		ContexteGenerationCode &contexte,
		Enchaineuse &enchaineuse,
		assembleuse_arbre const &arbre,
		dls::chaine const &racine_kuri)
{
	for (auto const &inc : arbre.inclusions) {
		enchaineuse << "#include <" << inc << ">\n";
	}

	enchaineuse << "\n";

	enchaineuse << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";

	enchaineuse <<
R"(
#define INITIALISE_TRACE_APPEL(_nom_fonction, _taille_nom, _fichier, _taille_fichier, _pointeur_fonction) \
	static KsInfoFonctionTraceAppel mon_info = { { .pointeur = _nom_fonction, .taille = _taille_nom }, { .pointeur = _fichier, .taille = _taille_fichier }, _pointeur_fonction }; \
	KsTraceAppel ma_trace = { 0 }; \
	ma_trace.info_fonction = &mon_info; \
	ma_trace.prxC3xA9cxC3xA9dente = contexte.trace_appel; \
	ma_trace.profondeur = contexte.trace_appel->profondeur + 1;

#define DEBUTE_RECORD_TRACE_APPEL_EX_EX(_index, _ligne, _colonne, _ligne_appel, _taille_ligne) \
	static KsInfoAppelTraceAppel info_appel##_index = { _ligne, _colonne, { .pointeur = _ligne_appel, .taille = _taille_ligne } }; \
	ma_trace.info_appel = &info_appel##_index; \
	contexte.trace_appel = &ma_trace;

#define DEBUTE_RECORD_TRACE_APPEL_EX(_index, _ligne, _colonne, _ligne_appel, _taille_ligne) \
	DEBUTE_RECORD_TRACE_APPEL_EX_EX(_index, _ligne, _colonne, _ligne_appel, _taille_ligne)

#define DEBUTE_RECORD_TRACE_APPEL(_ligne, _colonne, _ligne_appel, _taille_ligne) \
	DEBUTE_RECORD_TRACE_APPEL_EX(__COUNTER__, _ligne, _colonne, _ligne_appel, _taille_ligne)

#define TERMINE_RECORD_TRACE_APPEL \
   contexte.trace_appel = ma_trace.prxC3xA9cxC3xA9dente;
	)";

#if 0
	enchaineuse << "#include <signal.h>\n";
	enchaineuse << "static void gere_erreur_segmentation(int s)\n";
	enchaineuse << "{\n";
	enchaineuse << "    if (s == SIGSEGV) {\n";
	enchaineuse << "        " << contexte.interface_kuri.decl_panique->nom_broye << "(\"erreur de ségmentation dans une fonction\");\n";
	enchaineuse << "    }\n";
	enchaineuse << "    " << contexte.interface_kuri.decl_panique->nom_broye << "(\"erreur inconnue\");\n";
	enchaineuse << "}\n";
#endif

	/* déclaration des types de bases */
	enchaineuse << "typedef struct chaine { char *pointeur; long taille; } chaine;\n";
	enchaineuse << "typedef struct eini { void *pointeur; struct InfoType *info; } eini;\n";
	enchaineuse << "#ifndef bool // bool est défini dans stdbool.h\n";
	enchaineuse << "typedef unsigned char bool;\n";
	enchaineuse << "#endif\n";
	enchaineuse << "typedef unsigned char octet;\n";
	enchaineuse << "typedef void Ksnul;\n";
	enchaineuse << "typedef struct ContexteProgramme KsContexteProgramme;\n";
	/* À FAIRE : pas beau, mais un pointeur de fonction peut être un pointeur
	 * vers une fonction de LibC dont les arguments variadiques ne sont pas
	 * typés */
	enchaineuse << "#define Kv ...\n\n";
}

struct GeneratriceCodeC {
	dls::dico<Atome const *, dls::chaine> table_valeurs{};
	dls::dico<Atome const *, dls::chaine> table_globales{};
	ContexteGenerationCode &m_contexte;
	AtomeFonction const *m_fonction_courante = nullptr;

	// les atomes pour les chaines peuvent être générés plusieurs fois (notamment
	// pour celles des noms des fonctions pour les traces d'appel), utilisons un
	// index pour les rendre uniques
	int index_chaine = 0;

	GeneratriceCodeC(ContexteGenerationCode &contexte)
		: m_contexte(contexte)
	{}

	COPIE_CONSTRUCT(GeneratriceCodeC);

	dls::chaine genere_code_pour_atome(Atome *atome, Enchaineuse &os, bool pour_globale)
	{
		switch (atome->genre_atome) {
			case Atome::Genre::FONCTION:
			{
				auto atome_fonc = static_cast<AtomeFonction const *>(atome);
				return atome_fonc->nom;
			}
			case Atome::Genre::CONSTANTE:
			{
				auto atome_const = static_cast<AtomeConstante const *>(atome);

				switch (atome_const->genre) {
					case AtomeConstante::Genre::GLOBALE:
					{
						auto valeur_globale = static_cast<AtomeGlobale const *>(atome);

						if (valeur_globale->ident) {
							return valeur_globale->ident->nom;
						}

						return table_valeurs[valeur_globale];
					}
					case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
					{
						auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
						auto valeur = genere_code_pour_atome(transtype_const->valeur, os, pour_globale);
						return "(" + nom_broye_type(transtype_const->type) + ")(" + valeur + ")";
					}
					case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
					{
						break;
					}
					case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
					{
						break;
					}
					case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
					{
						auto inst_acces = static_cast<AccedeIndexConstant const *>(atome_const);
						auto valeur_accede = genere_code_pour_atome(inst_acces->accede, os, false);
						auto valeur_index = genere_code_pour_atome(inst_acces->index, os, false);
						return valeur_accede + "[" + valeur_index + "]";
					}
					case AtomeConstante::Genre::VALEUR:
					{
						auto valeur_const = static_cast<AtomeValeurConstante const *>(atome);

						switch (valeur_const->valeur.genre) {
							case AtomeValeurConstante::Valeur::Genre::NULLE:
							{
								return "0";
							}
							case AtomeValeurConstante::Valeur::Genre::REELLE:
							{
								return dls::vers_chaine(valeur_const->valeur.valeur_reelle);
							}
							case AtomeValeurConstante::Valeur::Genre::ENTIERE:
							{
								auto resultat = dls::vers_chaine(valeur_const->valeur.valeur_entiere);
								auto type = valeur_const->type;

								if (type->taille_octet == 8) {
									if (type->genre == GenreType::ENTIER_NATUREL) {
										resultat += "UL";
									}
									else {
										resultat += "L";
									}
								}

								return resultat;
							}
							case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
							{
								return dls::vers_chaine(valeur_const->valeur.valeur_booleenne);
							}
							case AtomeValeurConstante::Valeur::Genre::CARACTERE:
							{
								return dls::vers_chaine(valeur_const->valeur.valeur_entiere);
							}
							case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
							{
								return "";
							}
							case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
							{
								auto type = static_cast<TypeCompose *>(atome->type);
								auto tableau_valeur = valeur_const->valeur.valeur_structure.pointeur;
								auto resultat = dls::chaine();

								auto virgule = "{ ";

								for (auto i = 0; i < type->membres.taille; ++i) {
									// les tableaux fixes ont une initialisation nulle
									if (tableau_valeur[i] == nullptr) {
										continue;
									}

									resultat += virgule;

									resultat += ".";
									resultat += broye_nom_simple(type->membres[i].nom);
									resultat += " = ";
									resultat += genere_code_pour_atome(tableau_valeur[i], os, pour_globale);

									virgule = ", ";
								}

								resultat += " }";

								if (pour_globale) {
									return resultat;
								}

								auto nom = "val" + dls::vers_chaine(atome) + dls::vers_chaine(index_chaine++);
								os << "  " << nom_broye_type(atome->type) << " " << nom << " = " << resultat << ";\n";
								return nom;
							}
							case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
							{
								auto pointeur_tableau = valeur_const->valeur.valeur_tableau.pointeur;
								auto taille_tableau = valeur_const->valeur.valeur_tableau.taille;
								auto resultat = dls::chaine();

								auto virgule = "{ ";

								for (auto i = 0; i < taille_tableau; ++i) {
									resultat += virgule;
									resultat += genere_code_pour_atome(pointeur_tableau[i], os, pour_globale);
									virgule = ", ";
								}

								if (taille_tableau == 0) {
									resultat += "{";
								}

								resultat += " }";

								return resultat;
							}
							case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
							{
								auto pointeur_donnnees = valeur_const->valeur.valeur_tdc.pointeur;
								auto taille_donnees = valeur_const->valeur.valeur_tdc.taille;

								auto resultat = dls::chaine();

								auto virgule = "{ ";

								for (auto i = 0; i < taille_donnees; ++i) {
									auto octet = pointeur_donnnees[i];
									resultat += virgule;
									resultat += "0x";
									resultat.append(char_depuis_hex((octet & 0xf0) >> 4));
									resultat.append(char_depuis_hex(octet & 0x0f));
									virgule = ", ";
								}

								if (taille_donnees == 0) {
									resultat += "{";
								}

								resultat += " }";

								return resultat;
							}
						}
					}
				}

				return "";
			}
			case Atome::Genre::INSTRUCTION:
			{
				auto inst = static_cast<Instruction const *>(atome);
				return table_valeurs[inst];
			}
			case Atome::Genre::GLOBALE:
			{
				return table_globales[atome];
			}
		}

		return "";
	}

	void genere_code_pour_instruction(Instruction const *inst, Enchaineuse &os)
	{
		switch (inst->genre) {
			case Instruction::Genre::INVALIDE:
			{
				os << "  invalide\n";
				break;
			}
			case Instruction::Genre::ALLOCATION:
			{
				auto type_pointeur = static_cast<TypePointeur *>(inst->type);
				os << "  " << nom_broye_type(type_pointeur->type_pointe);

				// les portées ne sont plus respectées : deux variables avec le même nom dans deux portées différentes auront le même nom ici dans la même portée
				// donc nous ajoutons le numéro de l'instruction de la variable pour les différencier
				if (inst->ident != nullptr) {
					auto nom = broye_nom_simple(inst->ident->nom) + "_" + dls::vers_chaine(inst->numero);
					os << ' ' << nom << ";\n";
					table_valeurs[inst] = "&" + nom;
				}
				else {
					auto nom = "val" + dls::vers_chaine(inst->numero);
					os << ' ' << nom << ";\n";
					table_valeurs[inst] = "&" + nom;
				}

				break;
			}
			case Instruction::Genre::APPEL:
			{
				auto inst_appel = static_cast<InstructionAppel const *>(inst);

				auto const &lexeme = inst_appel->lexeme;
				auto fichier = m_contexte.fichier(static_cast<size_t>(lexeme->fichier));
				auto pos = position_lexeme(*lexeme);

				if (!m_fonction_courante->sanstrace) {
					os << "  DEBUTE_RECORD_TRACE_APPEL(";
					os << pos.numero_ligne << ",";
					os << pos.pos << ",";
					os << "\"";

					auto ligne = fichier->tampon[pos.index_ligne];

					POUR (ligne) {
						os << "\\x"
						   << char_depuis_hex((it & 0xf0) >> 4)
						   << char_depuis_hex(it & 0x0f);
					}

					os << "\",";
					os << ligne.taille();
					os << ");\n";
				}

				auto arguments = dls::tablet<dls::chaine, 10>();

				POUR (inst_appel->args) {
					arguments.pousse(genere_code_pour_atome(it, os, false));
				}

				os << "  ";

				if  (inst_appel->type->genre != GenreType::RIEN) {
					auto nom_ret = "__ret" + dls::vers_chaine(inst->numero);
					os << nom_broye_type(inst_appel->type) << ' ' << nom_ret << " = ";
					table_valeurs[inst] = nom_ret;
				}

				os << genere_code_pour_atome(inst_appel->appele, os, false);

				auto virgule = "(";

				POUR (arguments) {
					os << virgule;
					os << it;
					virgule = ", ";
				}

				if (inst_appel->args.taille == 0) {
					os << virgule;
				}

				os << ");\n";

				if (!m_fonction_courante->sanstrace) {
					os << "  TERMINE_RECORD_TRACE_APPEL;\n";
				}

				break;
			}
			case Instruction::Genre::BRANCHE:
			{
				auto inst_branche = static_cast<InstructionBranche const *>(inst);
				os << "  goto label" << inst_branche->label->id << ";\n";
				break;
			}
			case Instruction::Genre::BRANCHE_CONDITION:
			{
				auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
				auto condition = genere_code_pour_atome(inst_branche->condition, os, false);
				os << "  if (" << condition;
				os << ") goto label" << inst_branche->label_si_vrai->id << "; ";
				os << "else goto label" << inst_branche->label_si_faux->id << ";\n";
				break;
			}
			case Instruction::Genre::CHARGE_MEMOIRE:
			{
				auto inst_charge = static_cast<InstructionChargeMem const *>(inst);
				auto charge = inst_charge->chargee;
				auto valeur = dls::chaine();

				if (charge->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur = table_valeurs[charge];
				}
				else {
					valeur = table_globales[charge];
				}

				assert(valeur != "");

				if (valeur[0] == '&') {
					table_valeurs.insere({ inst_charge, valeur.sous_chaine(1) });
				}
				else {
					table_valeurs.insere({ inst_charge, "(*" + valeur + ")" });
				}

				break;
			}
			case Instruction::Genre::STOCKE_MEMOIRE:
			{
				auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
				auto type_valeur = inst_stocke->valeur->type;
				auto valeur = genere_code_pour_atome(inst_stocke->valeur, os, false);
				auto ou = inst_stocke->ou;
				auto valeur_ou = dls::chaine();

				if (ou->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur_ou = table_valeurs[ou];
				}
				else {
					valeur_ou = table_globales[ou];
				}

				if (valeur_ou[0] == '&') {
					valeur_ou = valeur_ou.sous_chaine(1);
				}
				else {
					valeur_ou = "(*" + valeur_ou + ")";
				}

				if (type_valeur->genre == GenreType::TABLEAU_FIXE) {
					os << "  memcpy(" << valeur_ou << ", " << valeur << ", " << type_valeur->taille_octet << ");\n";
				}
				else {
					os << "  " << valeur_ou << " = " << valeur << ";\n";
				}

				break;
			}
			case Instruction::Genre::LABEL:
			{
				auto inst_label = static_cast<InstructionLabel const *>(inst);
				os << "\nlabel" << inst_label->id << ":;\n";
				break;
			}
			case Instruction::Genre::OPERATION_UNAIRE:
			{
				auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
				auto valeur = genere_code_pour_atome(inst_un->valeur, os, false);

				os << "  " << nom_broye_type(inst_un->type) << " val" << inst->numero << " = ";

				switch (inst_un->op) {
					case OperateurUnaire::Genre::Positif:
					{
						break;
					}
					case OperateurUnaire::Genre::Invalide:
					{
						break;
					}
					case OperateurUnaire::Genre::Complement:
					{
						os << '-';
						break;
					}
					case OperateurUnaire::Genre::Non_Binaire:
					{
						os << '~';
						break;
					}
					case OperateurUnaire::Genre::Non_Logique:
					{
						os << '!';
						break;
					}
					case OperateurUnaire::Genre::Prise_Adresse:
					{
						os << '&';
						break;
					}
				}

				os << valeur;
				os << ";\n";

				table_valeurs[inst] = "val" + dls::vers_chaine(inst->numero);
				break;
			}
			case Instruction::Genre::OPERATION_BINAIRE:
			{
				auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);
				auto valeur_gauche = genere_code_pour_atome(inst_bin->valeur_gauche, os, false);
				auto valeur_droite = genere_code_pour_atome(inst_bin->valeur_droite, os, false);

				os << "  " << nom_broye_type(inst_bin->type) << " val" << inst->numero << " = ";

				os << valeur_gauche;

				switch (inst_bin->op) {
					case OperateurBinaire::Genre::Addition:
					case OperateurBinaire::Genre::Addition_Reel:
					{
						os << " + ";
						break;
					}
					case OperateurBinaire::Genre::Soustraction:
					case OperateurBinaire::Genre::Soustraction_Reel:
					{
						os << " - ";
						break;
					}
					case OperateurBinaire::Genre::Multiplication:
					case OperateurBinaire::Genre::Multiplication_Reel:
					{
						os << " * ";
						break;
					}
					case OperateurBinaire::Genre::Division_Naturel:
					case OperateurBinaire::Genre::Division_Relatif:
					case OperateurBinaire::Genre::Division_Reel:
					{
						os << " / ";
						break;
					}
					case OperateurBinaire::Genre::Reste_Naturel:
					case OperateurBinaire::Genre::Reste_Relatif:
					{
						os << " % ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Egal:
					case OperateurBinaire::Genre::Comp_Egal_Reel:
					{
						os << " == ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Inegal:
					case OperateurBinaire::Genre::Comp_Inegal_Reel:
					{
						os << " != ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Inf:
					case OperateurBinaire::Genre::Comp_Inf_Nat:
					case OperateurBinaire::Genre::Comp_Inf_Reel:
					{
						os << " < ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Inf_Egal:
					case OperateurBinaire::Genre::Comp_Inf_Egal_Nat:
					case OperateurBinaire::Genre::Comp_Inf_Egal_Reel:
					{
						os << " <= ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Sup:
					case OperateurBinaire::Genre::Comp_Sup_Nat:
					case OperateurBinaire::Genre::Comp_Sup_Reel:
					{
						os << " > ";
						break;
					}
					case OperateurBinaire::Genre::Comp_Sup_Egal:
					case OperateurBinaire::Genre::Comp_Sup_Egal_Nat:
					case OperateurBinaire::Genre::Comp_Sup_Egal_Reel:
					{
						os << " >= ";
						break;
					}
					case OperateurBinaire::Genre::Et_Logique:
					{
						os << " && ";
						break;
					}
					case OperateurBinaire::Genre::Ou_Logique:
					{
						os << " || ";
						break;
					}
					case OperateurBinaire::Genre::Et_Binaire:
					{
						os << " & ";
						break;
					}
					case OperateurBinaire::Genre::Ou_Binaire:
					{
						os << " | ";
						break;
					}
					case OperateurBinaire::Genre::Ou_Exclusif:
					{
						os << " ^ ";
						break;
					}
					case OperateurBinaire::Genre::Dec_Gauche:
					{
						os << " << ";
						break;
					}
					case OperateurBinaire::Genre::Dec_Droite_Arithm:
					case OperateurBinaire::Genre::Dec_Droite_Logique:
					{
						os << " >> ";
						break;
					}
					case OperateurBinaire::Genre::Invalide:
					{
						os << " invalide ";
						break;
					}
				}

				os << valeur_droite;
				os << ";\n";

				table_valeurs[inst] = "val" + dls::vers_chaine(inst->numero);

				break;
			}
			case Instruction::Genre::RETOUR:
			{
				auto inst_retour = static_cast<InstructionRetour const *>(inst);
				if (inst_retour->valeur != nullptr) {
					auto atome = inst_retour->valeur;
					auto valeur_retour = genere_code_pour_atome(atome, os, false);
					os << "  return";
					os << ' ';
					os << valeur_retour;
				}
				else {
					os << "  return";
				}
				os << ";\n";
				break;
			}
			case Instruction::Genre::ACCEDE_INDEX:
			{
				auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
				auto valeur_accede = genere_code_pour_atome(inst_acces->accede, os, false);
				auto valeur_index = genere_code_pour_atome(inst_acces->index, os, false);
				auto valeur = valeur_accede + "[" + valeur_index + "]";
				table_valeurs[inst] = valeur;
				break;
			}
			case Instruction::Genre::ACCEDE_MEMBRE:
			{
				auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);

				auto accede = inst_acces->accede;
				auto valeur_accede = dls::chaine();

				if (accede->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur_accede = broye_nom_simple(table_valeurs[accede]);
				}
				else {
					valeur_accede = broye_nom_simple(table_globales[accede]);
				}

				auto type_pointeur = static_cast<TypePointeur *>(inst_acces->accede->type);
				auto type_compose = static_cast<TypeCompose *>(type_pointeur->type_pointe);

				auto index_membre = static_cast<long>(static_cast<AtomeValeurConstante *>(inst_acces->index)->valeur.valeur_entiere);

				if (valeur_accede[0] == '&') {
					if (index_membre == type_compose->membres.taille) {
						valeur_accede = valeur_accede + ".membre_actif";
					}
					else {
						valeur_accede = valeur_accede + "." + broye_nom_simple(type_compose->membres[index_membre].nom);
					}
				}
				else {
					if (index_membre == type_compose->membres.taille) {
						valeur_accede = "&" + valeur_accede + "->membre_actif";
					}
					else {
						valeur_accede = "&" + valeur_accede + "->" + broye_nom_simple(type_compose->membres[index_membre].nom);
					}
				}

				table_valeurs[inst_acces] = valeur_accede;
				break;
			}
			case Instruction::Genre::TRANSTYPE:
			{
				auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
				auto valeur = genere_code_pour_atome(inst_transtype->valeur, os, false);
				valeur = "(" + nom_broye_type(inst_transtype->type) + ")(" + valeur + ")";
				table_valeurs[inst] = valeur;
				break;
			}
		}
	}

	void genere_code(ConstructriceRI &constructrice_ri, Enchaineuse &os)
	{
		// prédéclare les globales pour éviter les problèmes de références cycliques
		POUR (constructrice_ri.globales) {
			auto valeur_globale = static_cast<AtomeGlobale const *>(it);

			if (!valeur_globale->est_constante) {
				continue;
			}

			auto type = static_cast<TypePointeur *>(valeur_globale->type)->type_pointe;

			os << "static const " << nom_broye_type(type) << ' ';

			if (valeur_globale->ident) {
				auto nom_globale = broye_nom_simple(valeur_globale->ident->nom);
				os << nom_globale;
				table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
			}
			else {
				auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
				os << nom_globale;
				table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
			}

			os << ";\n";
		}

		POUR (constructrice_ri.globales) {
			auto valeur_globale = static_cast<AtomeGlobale const *>(it);

			auto valeur_initialisateur = dls::chaine();

			if (valeur_globale->initialisateur) {
				valeur_initialisateur = genere_code_pour_atome(valeur_globale->initialisateur, os, true);
			}

			auto type = static_cast<TypePointeur *>(valeur_globale->type)->type_pointe;

			os << "static ";

			if (valeur_globale->est_constante) {
				os << "const ";
			}

			os << nom_broye_type(type) << ' ';

			if (valeur_globale->ident) {
				auto nom_globale = broye_nom_simple(valeur_globale->ident->nom);
				os << nom_globale;
				table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
			}
			else {
				auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
				os << nom_globale;
				table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
			}

			if (valeur_globale->initialisateur) {
				os << " = " << valeur_initialisateur;
			}

			os << ";\n";
		}

		POUR (constructrice_ri.fonctions) {
			if (it->genre_atome != Atome::Genre::FONCTION) {
				continue;
			}

			auto atome_fonc = static_cast<AtomeFonction const *>(it);

			if (atome_fonc->instructions.taille == 0) {
				// ignore les fonctions externes
				continue;
			}

			auto type_fonction = static_cast<TypeFonction *>(atome_fonc->type);
			os << nom_broye_type(type_fonction->types_sorties[0]) << " " << atome_fonc->nom;

			auto virgule = "(";

			for (auto param : atome_fonc->params_entrees) {
				os << virgule;

				auto type_pointeur = static_cast<TypePointeur *>(param->type);
				os << nom_broye_type(type_pointeur->type_pointe) << ' ';
				os << broye_nom_simple(param->ident->nom);

				virgule = ", ";
			}

			if (atome_fonc->params_entrees.taille == 0) {
				os << virgule;
			}

			os << ");\n\n";
		}

		POUR (constructrice_ri.fonctions) {
			switch (it->genre_atome) {
				case Atome::Genre::FONCTION:
				{
					table_valeurs.efface();

					auto atome_fonc = static_cast<AtomeFonction const *>(it);

					if (atome_fonc->instructions.taille == 0) {
						// ignore les fonctions externes
						break;
					}

					//std::cerr << "Génère code pour : " << atome_fonc->nom << '\n';

					auto type_fonction = static_cast<TypeFonction *>(atome_fonc->type);
					os << nom_broye_type(type_fonction->types_sorties[0]) << " " << atome_fonc->nom;

					auto virgule = "(";

					for (auto param : atome_fonc->params_entrees) {
						os << virgule;

						auto type_pointeur = static_cast<TypePointeur *>(param->type);
						os << nom_broye_type(type_pointeur->type_pointe) << ' ';
						os << broye_nom_simple(param->ident->nom);

						table_valeurs.insere({ param, "&" + broye_nom_simple(param->ident->nom) });

						virgule = ", ";
					}

					if (atome_fonc->params_entrees.taille == 0) {
						os << virgule;
					}

					os << ")";

					if (atome_fonc->instructions.est_vide()) {
						os << ";\n\n";
						break;
					}

					os << "\n{\n";

					if (!atome_fonc->sanstrace) {
						os << "INITIALISE_TRACE_APPEL(\"";

						if (atome_fonc->lexeme != nullptr) {
							auto fichier = m_contexte.fichier(static_cast<size_t>(atome_fonc->lexeme->fichier));
							os << atome_fonc->lexeme->chaine << "\", "
							   << atome_fonc->lexeme->chaine.taille() << ", \""
							   << fichier->nom << ".kuri\", "
							   << fichier->nom.taille() + 5 << ", ";
						}
						else {
							os << atome_fonc->nom << "\", "
							   << atome_fonc->nom.taille() << ", "
							   << "\"???\", 3, ";
						}

						os << atome_fonc->nom << ");\n";
					}

					m_fonction_courante = atome_fonc;

					for (auto inst : atome_fonc->instructions) {
						genere_code_pour_instruction(static_cast<Instruction const *>(inst), os);
					}

					m_fonction_courante = nullptr;

					os << "}\n\n";

					break;
				}
				case Atome::Genre::CONSTANTE:
				{
					break;
				}
				case Atome::Genre::INSTRUCTION:
				{
					break;
				}
				case Atome::Genre::GLOBALE:
				{
					break;
				}
			}
		}
	}
};

void genere_code_C(
		assembleuse_arbre const &arbre,
		ConstructriceRI &constructrice_ri,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	auto debut_generation = dls::chrono::compte_seconde();

	Enchaineuse enchaineuse;

	genere_code_debut_fichier(constructrice_ri.contexte(), enchaineuse, arbre, racine_kuri);
	genere_typedefs_pour_tous_les_types(constructrice_ri.contexte(), enchaineuse);

	auto &typeuse = constructrice_ri.contexte().typeuse;

	POUR (typeuse.types_structures) {
		genere_declaration_structure(enchaineuse, it->decl);
	}

	POUR (typeuse.types_unions) {
		genere_declaration_structure(enchaineuse, it->decl);
	}

	auto generatrice = GeneratriceCodeC(constructrice_ri.contexte());
	generatrice.genere_code(constructrice_ri, enchaineuse);

	enchaineuse.imprime_dans_flux(fichier_sortie);

	constructrice_ri.contexte().temps_generation = debut_generation.temps();
}

void genere_code_pour_execution(
		ConstructriceRI &constructrice_ri,
		assembleuse_arbre const &arbre,
		NoeudExpression *noeud_appel,
		ContexteGenerationCode &contexte,
		dls::chaine const &racine_kuri,
		std::ostream &fichier_sortie)
{
	Enchaineuse enchaineuse;

	genere_code_debut_fichier(constructrice_ri.contexte(), enchaineuse, arbre, racine_kuri);
	genere_typedefs_pour_tous_les_types(constructrice_ri.contexte(), enchaineuse);

	auto &typeuse = constructrice_ri.contexte().typeuse;

	POUR (typeuse.types_structures) {
		genere_declaration_structure(enchaineuse, it->decl);
	}

	POUR (typeuse.types_unions) {
		genere_declaration_structure(enchaineuse, it->decl);
	}

	auto generatrice = GeneratriceCodeC(constructrice_ri.contexte());
	generatrice.genere_code(constructrice_ri, enchaineuse);

	//genere_code_programme(contexte, constructrice, noeud_fonction_principale, debut_generation);

	enchaineuse << "void lance_execution()\n";
	enchaineuse << "{\n";

	// À FAIRE : génère code pour l'expression
	//genere_code_creation_contexte(contexte, enchaineuse);

	enchaineuse << "    return;\n";
	enchaineuse << "}\n";

	enchaineuse.imprime_dans_flux(fichier_sortie);

	POUR (contexte.typeuse.types_simples) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_pointeurs) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_references) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_structures) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; it->deja_genere = false; }
	POUR (contexte.typeuse.types_enums) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_tableaux_fixes) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_tableaux_dynamiques) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_fonctions) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_variadiques) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; };
	POUR (contexte.typeuse.types_unions) { it->drapeaux &= ~TYPEDEF_FUT_GENERE; it->deja_genere = false; };
}

}  /* namespace noeud */

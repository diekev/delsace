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

#include "coulisse_c.hh"

#include <fstream>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/enchaineuse.hh"
#include "biblinternes/outils/numerique.hh"

#include "broyage.hh"
#include "compilatrice.hh"
#include "environnement.hh"
#include "erreur.h"
#include "table_hachage.hh"
#include "typage.hh"

#include "representation_intermediaire/constructrice_ri.hh"

/* ************************************************************************** */

static void cree_typedef(Type *type, Enchaineuse &enchaineuse)
{
	if (type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
		return;
	}

	auto const &nom_broye = nom_broye_type(type);

	enchaineuse << "// " << chaine_type(type) << '\n';

	switch (type->genre) {
		case GenreType::POLYMORPHIQUE:
		{
			break;
		}
		case GenreType::ERREUR:
		case GenreType::ENUM:
		{
			auto type_enum = type->comme_enum();
			auto nom_broye_type_donnees = nom_broye_type(type_enum->type_donnees);

			enchaineuse << "typedef " << nom_broye_type_donnees << ' ' << nom_broye << ";\n";
			break;
		}
		case GenreType::OPAQUE:
		{
			auto type_opaque = type->comme_opaque();
			auto nom_broye_type_opacifie = nom_broye_type(type_opaque->type_opacifie);

			enchaineuse << "typedef " << nom_broye_type_opacifie << ' ' << nom_broye << ";\n";
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
		case GenreType::TYPE_DE_DONNEES:
		{
			enchaineuse << "typedef long " << nom_broye << ";\n";
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
			auto type_pointe = type->comme_reference()->type_pointe;
			enchaineuse << "typedef " << nom_broye_type(type_pointe) << "* " << nom_broye << ";\n";
			break;
		}
		case GenreType::POINTEUR:
		{
			auto type_pointe = type->comme_pointeur()->type_pointe;

			if (type_pointe) {
				enchaineuse << "typedef " << nom_broye_type(type_pointe) << "* " << nom_broye << ";\n";
			}
			else {
				enchaineuse << "typedef Ksnul *" << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = type->comme_structure();

			if (type_struct->decl && type_struct->decl->est_polymorphe) {
				break;
			}

			auto nom_struct = broye_nom_simple(type_struct->nom_portable());

			// struct anomyme
			if (type_struct->est_anonyme) {
				enchaineuse << "typedef struct " << nom_struct << dls::vers_chaine(type_struct) << ' ' << nom_broye << ";\n";
				break;
			}

			if (type_struct->decl && type_struct->decl->est_monomorphisation) {
				nom_struct += dls::vers_chaine(type_struct);
			}

			enchaineuse << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";

			break;
		}
		case GenreType::UNION:
		{
			auto type_struct = type->comme_union();
			auto nom_struct = broye_nom_simple(type_struct->nom_portable());
			auto decl = type_struct->decl;

			// union anomyme
			if (type_struct->est_anonyme) {
				enchaineuse << "typedef struct " << nom_struct << dls::vers_chaine(type_struct) << ' ' << nom_broye << ";\n";
				break;
			}

			if (decl->est_nonsure || decl->est_externe) {
				enchaineuse << "typedef union " << nom_struct << ' ' << nom_broye << ";\n";
			}
			else {
				enchaineuse << "typedef struct " << nom_struct << ' ' << nom_broye << ";\n";
			}

			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_pointe = type->comme_tableau_fixe()->type_pointe;
			enchaineuse << "typedef struct { " << nom_broye_type(type_pointe);
			enchaineuse << " d[" << type->comme_tableau_fixe()->taille << "];";
			enchaineuse << " } " << nom_broye << ";\n\n";
			break;
		}
		case GenreType::VARIADIQUE:
		{
			// les arguments variadiques sont transformés en tableaux, donc RÀF
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_pointe = type->comme_tableau_dynamique()->type_pointe;

			if (type_pointe == nullptr) {
				return;
			}

			enchaineuse << "typedef struct Tableau_" << nom_broye;
			enchaineuse << "{\n\t";
			enchaineuse << nom_broye_type(type_pointe) << " *pointeur;";
			enchaineuse << "\n\tlong taille;\n" << "\tlong " << broye_nom_simple("capacité") << ";\n} " << nom_broye << ";\n\n";
			break;
		}
		case GenreType::FONCTION:
		{
			auto type_fonc = type->comme_fonction();

			auto prefixe = dls::chaine("");
			auto suffixe = dls::chaine("");

			auto nouveau_nom_broye = dls::chaine("Kf");
			nouveau_nom_broye += dls::vers_chaine(type_fonc->types_entrees.taille);

			auto const &nom_broye_sortie = nom_broye_type(type_fonc->type_sortie);
			prefixe += nom_broye_sortie + " (*";

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

			nouveau_nom_broye += dls::vers_chaine(1);
			nouveau_nom_broye += nom_broye_sortie;

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
		case GenreType::TUPLE:
		{
			enchaineuse << "typedef struct " << nom_broye << ' ' << nom_broye << ";\n";
			break;
		}
	}
}

/* ************************************************************************** */

enum {
	STRUCTURE,
	STRUCTURE_ANONYME,
};

static void genere_declaration_structure(Enchaineuse &enchaineuse, TypeStructure *type_compose, int quoi)
{
	auto nom_broye = broye_nom_simple(type_compose->nom_portable());

	if (type_compose->decl && type_compose->decl->est_monomorphisation) {
		nom_broye += dls::vers_chaine(type_compose);
	}

	if (quoi == STRUCTURE) {
		enchaineuse << "typedef struct " << nom_broye << "{\n";
	}
	else if (quoi == STRUCTURE_ANONYME) {
		enchaineuse << "typedef struct " << nom_broye;
		enchaineuse << dls::vers_chaine(type_compose);
		enchaineuse << "{\n";
	}

	POUR (type_compose->membres) {
		if (it.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
			continue;
		}

		enchaineuse << nom_broye_type(it.type) << ' ';

		/* Cas pour les structures vides. */
		if (it.nom == ID::chaine_vide) {
			enchaineuse << "membre_invisible" << ";\n";
		}
		else {
			enchaineuse << broye_nom_simple(it.nom->nom) << ";\n";
		}
	}

	enchaineuse << "} " << nom_broye;

	if (quoi == STRUCTURE_ANONYME) {
		enchaineuse << dls::vers_chaine(type_compose);
	}

	enchaineuse << ";\n\n";
}

/* ************************************************************************** */

static bool peut_etre_dereference(Type *type)
{
	return type->genre == GenreType::TABLEAU_FIXE || type->genre == GenreType::TABLEAU_DYNAMIQUE || type->genre == GenreType::REFERENCE || type->genre == GenreType::POINTEUR;
}

static void genere_typedefs_recursifs(
		Compilatrice &compilatrice,
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
			if (type->genre != GenreType::POINTEUR) {
				return;
			}
		}
		else {
			if (type_deref->genre == GenreType::VARIADIQUE) {
				type_deref = type_deref->comme_variadique()->type_pointe;

				/* dans la RI nous créons un pointeur vers le type des arguments ce
				 * qui peut inclure un pointeur vers un type variadique externe sans
				 * type */
				if (type_deref == nullptr) {
					return;
				}
			}

			if ((type_deref->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(compilatrice, type_deref, enchaineuse);
			}

			type_deref->drapeaux |= TYPEDEF_FUT_GENERE;
		}
	}
	/* ajoute les types des paramètres et de retour des fonctions */
	else if (type->genre == GenreType::FONCTION) {
		auto type_fonc = type->comme_fonction();

		POUR (type_fonc->types_entrees) {
			if ((it->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
				genere_typedefs_recursifs(compilatrice, it, enchaineuse);
			}

			it->drapeaux |= TYPEDEF_FUT_GENERE;
		}

		if ((type_fonc->type_sortie->drapeaux & TYPEDEF_FUT_GENERE) == 0) {
			genere_typedefs_recursifs(compilatrice, type_fonc->type_sortie, enchaineuse);
		}

		type_fonc->type_sortie->drapeaux |= TYPEDEF_FUT_GENERE;
	}

	cree_typedef(type, enchaineuse);
	type->drapeaux |= TYPEDEF_FUT_GENERE;
}

// ----------------------------------------------

static void genere_code_debut_fichier(
		Enchaineuse &enchaineuse,
		dls::chaine const &racine_kuri)
{
	enchaineuse << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";

	enchaineuse <<
R"(
#define INITIALISE_TRACE_APPEL(_nom_fonction, _taille_nom, _fichier, _taille_fichier, _pointeur_fonction) \
	static KsKuriInfoFonctionTraceAppel mon_info = { { .pointeur = _nom_fonction, .taille = _taille_nom }, { .pointeur = _fichier, .taille = _taille_fichier }, _pointeur_fonction }; \
	KsKuriTraceAppel ma_trace = { 0 }; \
	ma_trace.info_fonction = &mon_info; \
	ma_trace.prxC3xA9cxC3xA9dente = contexte.trace_appel; \
	ma_trace.profondeur = contexte.trace_appel->profondeur + 1;

#define DEBUTE_RECORD_TRACE_APPEL_EX_EX(_index, _ligne, _colonne, _ligne_appel, _taille_ligne) \
	static KsKuriInfoAppelTraceAppel info_appel##_index = { _ligne, _colonne, { .pointeur = _ligne_appel, .taille = _taille_ligne } }; \
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
	enchaineuse << "        " << compilatrice.interface_kuri.decl_panique->nom_broye << "(\"erreur de ségmentation dans une fonction\");\n";
	enchaineuse << "    }\n";
	enchaineuse << "    " << compilatrice.interface_kuri.decl_panique->nom_broye << "(\"erreur inconnue\");\n";
	enchaineuse << "}\n";
#endif

	/* déclaration des types de bases */
	enchaineuse << "typedef struct chaine { char *pointeur; long taille; } chaine;\n";
	enchaineuse << "typedef struct eini { void *pointeur; struct KuriInfoType *info; } eini;\n";
	enchaineuse << "#ifndef bool // bool est défini dans stdbool.h\n";
	enchaineuse << "typedef unsigned char bool;\n";
	enchaineuse << "#endif\n";
	enchaineuse << "typedef unsigned char octet;\n";
	enchaineuse << "typedef void Ksnul;\n";
	enchaineuse << "typedef struct KuriContexteProgramme KsKuriContexteProgramme;\n";
	/* pas beau, mais un pointeur de fonction peut être un pointeur vers une fonction
	 *  de LibC dont les arguments variadiques ne sont pas typés */
	enchaineuse << "#define Kv ...\n\n";
}

static bool est_type_tableau_fixe(Type *type)
{
	return type->est_tableau_fixe() || (type->est_opaque() && type->comme_opaque()->type_opacifie->est_tableau_fixe());
}

struct GeneratriceCodeC {
	table_hachage<Atome const *, dls::chaine> table_valeurs{};
	table_hachage<Atome const *, dls::chaine> table_globales{};
	EspaceDeTravail &m_espace;
	AtomeFonction const *m_fonction_courante = nullptr;

	// les atomes pour les chaines peuvent être générés plusieurs fois (notamment
	// pour celles des noms des fonctions pour les traces d'appel), utilisons un
	// index pour les rendre uniques
	int index_chaine = 0;

	GeneratriceCodeC(EspaceDeTravail &espace)
		: m_espace(espace)
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

						return table_valeurs.valeur_ou(valeur_globale, "");
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

						if (est_type_tableau_fixe(inst_acces->accede->type->comme_pointeur()->type_pointe)) {
							valeur_accede += ".d";
						}

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
							case AtomeValeurConstante::Valeur::Genre::TYPE:
							{
								return dls::vers_chaine(valeur_const->valeur.type->index_dans_table_types);
							}
							case AtomeValeurConstante::Valeur::Genre::REELLE:
							{
								return dls::vers_chaine(valeur_const->valeur.valeur_reelle);
							}
							case AtomeValeurConstante::Valeur::Genre::ENTIERE:
							{
								auto type = valeur_const->type;
								auto valeur_entiere = valeur_const->valeur.valeur_entiere;

								if (type->est_entier_naturel()) {
									if (type->taille_octet == 1) {
										return dls::vers_chaine(static_cast<unsigned char>(valeur_entiere));
									}
									else if (type->taille_octet == 2) {
										return dls::vers_chaine(static_cast<unsigned short>(valeur_entiere));
									}
									else if (type->taille_octet == 4) {
										return dls::vers_chaine(static_cast<unsigned int>(valeur_entiere));
									}
									else if (type->taille_octet == 8) {
										auto resultat = dls::vers_chaine(valeur_entiere);
										resultat += "UL";
										return resultat;
									}
								}
								else {
									if (type->taille_octet == 1) {
										return dls::vers_chaine(static_cast<char>(valeur_entiere));
									}
									else if (type->taille_octet == 2) {
										return dls::vers_chaine(static_cast<short>(valeur_entiere));
									}
									else if (type->taille_octet == 4 || type->taille_octet == 0) {
										return dls::vers_chaine(static_cast<int>(valeur_entiere));
									}
									else if (type->taille_octet == 8) {
										auto resultat = dls::vers_chaine(valeur_entiere);
										resultat += "L";
										return resultat;
									}
								}

								return "";
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
								// ceci car il peut n'y avoir qu'un seul membre de type tableau qui n'est pas initialisé
								auto virgule_placee = false;

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

									resultat += virgule;
									virgule_placee = true;

									resultat += ".";
									resultat += broye_nom_simple(type->membres[i].nom->nom);
									resultat += " = ";
									resultat += genere_code_pour_atome(tableau_valeur[index_membre], os, pour_globale);

									virgule = ", ";
									index_membre += 1;
								}

								if (!virgule_placee) {
									resultat += "{ 0";
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

								auto virgule = "{ .d = { ";

								for (auto i = 0; i < taille_tableau; ++i) {
									resultat += virgule;
									resultat += genere_code_pour_atome(pointeur_tableau[i], os, pour_globale);
									virgule = ", ";
								}

								if (taille_tableau == 0) {
									resultat += "{}";
								}
								else {
									resultat += " } }";
								}

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
									resultat.append(dls::num::char_depuis_hex((octet & 0xf0) >> 4));
									resultat.append(dls::num::char_depuis_hex(octet & 0x0f));
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
				auto inst = atome->comme_instruction();
				return table_valeurs.valeur_ou(inst, "");
			}
			case Atome::Genre::GLOBALE:
			{
				return table_globales.valeur_ou(atome, "");
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
				auto type_pointeur = inst->type->comme_pointeur();
				os << "  " << nom_broye_type(type_pointeur->type_pointe);

				// les portées ne sont plus respectées : deux variables avec le même nom dans deux portées différentes auront le même nom ici dans la même portée
				// donc nous ajoutons le numéro de l'instruction de la variable pour les différencier
				if (inst->ident != nullptr) {
					auto nom = broye_nom_simple(inst->ident->nom) + "_" + dls::vers_chaine(inst->numero);
					os << ' ' << nom << ";\n";
					table_valeurs.insere(inst, "&" + nom);
				}
				else {
					auto nom = "val" + dls::vers_chaine(inst->numero);
					os << ' ' << nom << ";\n";
					table_valeurs.insere(inst, "&" + nom);
				}

				break;
			}
			case Instruction::Genre::APPEL:
			{
				auto inst_appel = inst->comme_appel();

				auto const &lexeme = inst_appel->lexeme;
				auto fichier = m_espace.fichier(lexeme->fichier);
				auto pos = position_lexeme(*lexeme);

				if (!m_fonction_courante->sanstrace) {
					os << "  DEBUTE_RECORD_TRACE_APPEL(";
					os << pos.numero_ligne << ",";
					os << pos.pos << ",";
					os << "\"";

					auto ligne = fichier->tampon()[pos.index_ligne];

					POUR (ligne) {
						os << "\\x"
						   << dls::num::char_depuis_hex((it & 0xf0) >> 4)
						   << dls::num::char_depuis_hex(it & 0x0f);
					}

					os << "\",";
					os << ligne.taille();
					os << ");\n";
				}

				auto arguments = dls::tablet<dls::chaine, 10>();

				POUR (inst_appel->args) {
					arguments.ajoute(genere_code_pour_atome(it, os, false));
				}

				os << "  ";

				auto type_fonction = inst_appel->appele->type->comme_fonction();
				if  (!type_fonction->type_sortie->est_rien()) {
					auto nom_ret = "__ret" + dls::vers_chaine(inst->numero);
					os << nom_broye_type(inst_appel->type) << ' ' << nom_ret << " = ";
					table_valeurs.insere(inst, nom_ret);
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
				auto inst_branche = inst->comme_branche();
				os << "  goto label" << inst_branche->label->id << ";\n";
				break;
			}
			case Instruction::Genre::BRANCHE_CONDITION:
			{
				auto inst_branche = inst->comme_branche_cond();
				auto condition = genere_code_pour_atome(inst_branche->condition, os, false);
				os << "  if (" << condition;
				os << ") goto label" << inst_branche->label_si_vrai->id << "; ";
				os << "else goto label" << inst_branche->label_si_faux->id << ";\n";
				break;
			}
			case Instruction::Genre::CHARGE_MEMOIRE:
			{
				auto inst_charge = inst->comme_charge();
				auto charge = inst_charge->chargee;
				auto valeur = dls::chaine();

				if (charge->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur = table_valeurs.valeur_ou(charge, "");
				}
				else {
					valeur = table_globales.valeur_ou(charge, "");
				}

				assert(valeur != "");

				if (valeur[0] == '&') {
					table_valeurs.insere(inst_charge, valeur.sous_chaine(1));
				}
				else {
					table_valeurs.insere(inst_charge, "(*" + valeur + ")");
				}

				break;
			}
			case Instruction::Genre::STOCKE_MEMOIRE:
			{
				auto inst_stocke = inst->comme_stocke_mem();
				auto valeur = genere_code_pour_atome(inst_stocke->valeur, os, false);
				auto ou = inst_stocke->ou;
				auto valeur_ou = dls::chaine();

				if (ou->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur_ou = table_valeurs.valeur_ou(ou, "");
				}
				else {
					valeur_ou = table_globales.valeur_ou(ou, "");
				}

				if (valeur_ou[0] == '&') {
					valeur_ou = valeur_ou.sous_chaine(1);
				}
				else {
					valeur_ou = "(*" + valeur_ou + ")";
				}

				os << "  " << valeur_ou << " = " << valeur << ";\n";

				break;
			}
			case Instruction::Genre::LABEL:
			{
				auto inst_label = inst->comme_label();
				os << "\nlabel" << inst_label->id << ":;\n";
				break;
			}
			case Instruction::Genre::OPERATION_UNAIRE:
			{
				auto inst_un = inst->comme_op_unaire();
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

				table_valeurs.insere(inst, "val" + dls::vers_chaine(inst->numero));
				break;
			}
			case Instruction::Genre::OPERATION_BINAIRE:
			{
				auto inst_bin = inst->comme_op_binaire();
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
					case OperateurBinaire::Genre::Indexage:
					{
						os << " invalide ";
						break;
					}
				}

				os << valeur_droite;
				os << ";\n";

				table_valeurs.insere(inst, "val" + dls::vers_chaine(inst->numero));

				break;
			}
			case Instruction::Genre::RETOUR:
			{
				auto inst_retour = inst->comme_retour();
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
				auto inst_acces = inst->comme_acces_index();
				auto valeur_accede = genere_code_pour_atome(inst_acces->accede, os, false);
				auto valeur_index = genere_code_pour_atome(inst_acces->index, os, false);

				if (est_type_tableau_fixe(inst_acces->accede->type->comme_pointeur()->type_pointe)) {
					valeur_accede += ".d";
				}

				auto valeur = valeur_accede + "[" + valeur_index + "]";
				table_valeurs.insere(inst, valeur);
				break;
			}
			case Instruction::Genre::ACCEDE_MEMBRE:
			{
				auto inst_acces = inst->comme_acces_membre();

				auto accede = inst_acces->accede;
				auto valeur_accede = dls::chaine();

				if (accede->genre_atome == Atome::Genre::INSTRUCTION) {
					valeur_accede = broye_nom_simple(table_valeurs.valeur_ou(accede, ""));
				}
				else {
					valeur_accede = broye_nom_simple(table_globales.valeur_ou(accede, ""));
				}

				assert(valeur_accede != "");

				auto type_pointeur = inst_acces->accede->type->comme_pointeur();

				auto type_pointe = type_pointeur->type_pointe;
				if (type_pointe->est_opaque()) {
					type_pointe = type_pointe->comme_opaque()->type_opacifie;
				}

				auto type_compose = static_cast<TypeCompose *>(type_pointe);
				auto index_membre = static_cast<long>(static_cast<AtomeValeurConstante *>(inst_acces->index)->valeur.valeur_entiere);

				if (valeur_accede[0] == '&') {
					valeur_accede = valeur_accede + ".";
				}
				else {
					valeur_accede = "&" + valeur_accede + "->";
				}

				auto const &membre = type_compose->membres[index_membre];

				/* Cas pour les structures vides (dans leurs fonctions d'initialisation). */
				if (membre.nom == ID::chaine_vide) {
					valeur_accede += "membre_invisible";
				}
				else if (type_compose->est_tuple()) {
					valeur_accede += "_" + dls::vers_chaine(index_membre);
				}
				else {
					valeur_accede += broye_nom_simple(membre.nom->nom);
				}

				table_valeurs.insere(inst_acces, valeur_accede);
				break;
			}
			case Instruction::Genre::TRANSTYPE:
			{
				auto inst_transtype = inst->comme_transtype();
				auto valeur = genere_code_pour_atome(inst_transtype->valeur, os, false);
				valeur = "((" + nom_broye_type(inst_transtype->type) + ")(" + valeur + "))";
				table_valeurs.insere(inst, valeur);
				break;
			}
		}
	}

	void genere_code(tableau_page<AtomeGlobale> const &globales, kuri::tableau<AtomeFonction *> const &fonctions, Enchaineuse &os)
	{
		// prédéclare les globales pour éviter les problèmes de références cycliques
		POUR_TABLEAU_PAGE (globales) {
			auto valeur_globale = &it;

			if (!valeur_globale->est_constante) {
				continue;
			}

			auto type = valeur_globale->type->comme_pointeur()->type_pointe;

			os << "static const " << nom_broye_type(type) << ' ';

			if (valeur_globale->ident) {
				auto nom_globale = broye_nom_simple(valeur_globale->ident->nom);
				os << nom_globale;
				table_globales.insere(valeur_globale, "&" + broye_nom_simple(nom_globale));
			}
			else {
				auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
				os << nom_globale;
				table_globales.insere(valeur_globale, "&" + broye_nom_simple(nom_globale));
			}

			os << ";\n";
		}

		// prédéclare ensuite les fonction pour éviter les problèmes de
		// dépendances cycliques, mais aussi pour prendre en compte les cas où
		// les globales utilises des fonctions dans leurs initialisations
		POUR (fonctions) {
			auto atome_fonc = it;

			auto type_fonction = atome_fonc->type->comme_fonction();
			os << nom_broye_type(type_fonction->type_sortie) << " ";

			os << atome_fonc->nom;

			auto virgule = "(";

			for (auto param : atome_fonc->params_entrees) {
				os << virgule;

				auto type_pointeur = param->type->comme_pointeur();
				auto type_param = type_pointeur->type_pointe;
				os << nom_broye_type(type_param) << ' ';

				// dans le cas des fonctions variadiques externes, si le paramètres n'est pas typé (void fonction(...)), n'imprime pas de nom
				if (type_param->est_variadique() && type_param->comme_variadique()->type_pointe == nullptr) {
					continue;
				}

				os << broye_nom_simple(param->ident->nom);

				virgule = ", ";
			}

			if (atome_fonc->params_entrees.taille == 0) {
				os << virgule;
			}

			os << ");\n\n";
		}

		// définis ensuite les globales
		POUR_TABLEAU_PAGE (globales) {
			auto valeur_globale = &it;

			auto valeur_initialisateur = dls::chaine();

			if (valeur_globale->initialisateur) {
				valeur_initialisateur = genere_code_pour_atome(valeur_globale->initialisateur, os, true);
			}

			auto type = valeur_globale->type->comme_pointeur()->type_pointe;

			os << "static ";

			if (valeur_globale->est_constante) {
				os << "const ";
			}

			os << nom_broye_type(type) << ' ';

			if (valeur_globale->ident) {
				auto nom_globale = broye_nom_simple(valeur_globale->ident->nom);
				os << nom_globale;
				table_globales.insere(valeur_globale, "&" + broye_nom_simple(nom_globale));
			}
			else {
				auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
				os << nom_globale;
				table_globales.insere(valeur_globale, "&" + broye_nom_simple(nom_globale));
			}

			if (valeur_globale->initialisateur) {
				os << " = " << valeur_initialisateur;
			}

			os << ";\n";
		}

		// définis enfin les fonction
		POUR (fonctions) {
			if (it->nombre_utilisations == 0) {
				continue;
			}

			auto atome_fonc = it;

			if (atome_fonc->instructions.taille == 0) {
				// ignore les fonctions externes
				continue;
			}

			//std::cerr << "Génère code pour : " << atome_fonc->nom << '\n';

			auto type_fonction = atome_fonc->type->comme_fonction();
			os << nom_broye_type(type_fonction->type_sortie) << " ";

			os << atome_fonc->nom;

			auto virgule = "(";

			for (auto param : atome_fonc->params_entrees) {
				os << virgule;

				auto type_pointeur = param->type->comme_pointeur();
				os << nom_broye_type(type_pointeur->type_pointe) << ' ';
				os << broye_nom_simple(param->ident->nom);

				table_valeurs.insere(param, "&" + broye_nom_simple(param->ident->nom));

				virgule = ", ";
			}

			if (atome_fonc->params_entrees.taille == 0) {
				os << virgule;
			}

			os << ")\n{\n";

			if (!atome_fonc->sanstrace) {
				os << "INITIALISE_TRACE_APPEL(\"";

				if (atome_fonc->lexeme != nullptr) {
					auto fichier = m_espace.fichier(atome_fonc->lexeme->fichier);
					os << atome_fonc->lexeme->chaine << "\", "
					   << atome_fonc->lexeme->chaine.taille() << ", \""
					   << fichier->nom() << ".kuri\", "
					   << fichier->nom().taille() + 5 << ", ";
				}
				else {
					os << atome_fonc->nom << "\", "
					   << atome_fonc->nom.taille() << ", "
					   << "\"???\", 3, ";
				}

				os << atome_fonc->nom << ");\n";
			}

			m_fonction_courante = atome_fonc;

			auto numero_inst = static_cast<int>(atome_fonc->params_entrees.taille);

			/* crée une variable local pour la valeur de sortie */
			if (!type_fonction->type_sortie->est_rien()) {
				auto param = atome_fonc->param_sortie;
				auto type_pointeur = param->type->comme_pointeur();
				os << nom_broye_type(type_pointeur->type_pointe) << ' ';
				os << broye_nom_simple(param->ident->nom);
				os << ";\n";

				table_valeurs.insere(param, "&" + broye_nom_simple(param->ident->nom));
			}

			/* Génère le code pour les accès de membres des retours mutliples. */
			if (atome_fonc->decl && atome_fonc->decl->params_sorties.taille > 1) {
				for (auto &param : atome_fonc->decl->params_sorties) {
					genere_code_pour_instruction(param->atome->comme_instruction(), os);
				}
			}

			for (auto inst : atome_fonc->instructions) {
				inst->numero = numero_inst++;
				genere_code_pour_instruction(inst, os);
			}

			m_fonction_courante = nullptr;

			os << "}\n\n";
		}
	}
};

static void genere_code_pour_types(Compilatrice &compilatrice, dls::outils::Synchrone<GrapheDependance> &graphe_, Enchaineuse &enchaineuse)
{
	auto graphe = graphe_.verrou_ecriture();

	POUR_TABLEAU_PAGE(graphe->noeuds) {
		if (!it.est_type()) {
			continue;
		}

		if (it.fut_visite) {
			continue;
		}

		graphe->traverse(&it, [&](NoeudDependance *noeud)
		{
			if (!noeud->est_type()) {
				return;
			}

			auto type = noeud->type();

			if (type && type->genre == GenreType::TYPE_DE_DONNEES) {
				return;
			}

			genere_typedefs_recursifs(compilatrice, type, enchaineuse);

			if (type) {
				if (type->est_structure()) {
					auto type_struct = type->comme_structure();

					if (type_struct->decl && type_struct->decl->est_polymorphe) {
						return;
					}

					for (auto &membre : type_struct->membres) {
						genere_typedefs_recursifs(compilatrice, membre.type, enchaineuse);
					}

					auto quoi = type_struct->est_anonyme ? STRUCTURE_ANONYME : STRUCTURE;
					genere_declaration_structure(enchaineuse, type_struct, quoi);
				}
				else if (type->est_tuple()) {
					auto type_tuple = type->comme_tuple();

					if (type_tuple->drapeaux & TYPE_EST_POLYMORPHIQUE) {
						return;
					}

					for (auto &membre : type_tuple->membres) {
						genere_typedefs_recursifs(compilatrice, membre.type, enchaineuse);
					}

					auto nom_broye = nom_broye_type(type_tuple);

					enchaineuse << "typedef struct " << nom_broye << " {\n";

					auto index_membre = 0;
					for (auto &membre : type_tuple->membres) {
						enchaineuse << nom_broye_type(membre.type) << " _" << index_membre++ << ";\n";
					}

					enchaineuse << "} " << nom_broye << ";\n";
				}
			}
		});
	}

	POUR_TABLEAU_PAGE(graphe->noeuds) {
		it.fut_visite = false;
	}
}

static void genere_code_C_depuis_fonction_principale(
		Compilatrice &compilatrice,
		ConstructriceRI &constructrice_ri,
		EspaceDeTravail &espace,
		std::ostream &fichier_sortie)
{
	Enchaineuse enchaineuse;

	espace.typeuse.construit_table_types();

	// NOTE : on ne prend pas de verrou ici car genere_ri_pour_fonction_main reprendra un verrou du graphe via la Typeuse -> verrou mort
	auto &graphe = espace.graphe_dependance;
	auto fonction_principale = espace.fonction_principale;

	if (fonction_principale == nullptr) {
		erreur::fonction_principale_manquante(espace);
	}

	genere_code_debut_fichier(enchaineuse, compilatrice.racine_kuri);

	genere_code_pour_types(compilatrice, graphe, enchaineuse);

	dls::ensemble<AtomeFonction *> utilises;
	kuri::tableau<AtomeFonction *> fonctions;
	graphe->rassemble_fonctions_utilisees(fonction_principale->noeud_dependance, fonctions, utilises);
	graphe->rassemble_fonctions_utilisees(espace.fonction_point_d_entree->noeud_dependance, fonctions, utilises);

	// génère finalement la fonction __principale qui sers de pont entre __point_d_entree_systeme et principale
	auto atome_principale = constructrice_ri.genere_ri_pour_fonction_principale(&espace);
	fonctions.ajoute(atome_principale);

	// fais en sors que point_d_entree_systeme est utilisée, et renomme en « main » pour ne pas avoir à créer une autre fonction
	auto atome_fonc = static_cast<AtomeFonction *>(espace.fonction_point_d_entree->atome);
	atome_fonc->nombre_utilisations = 1;
	atome_fonc->nom = "main";

	auto generatrice = GeneratriceCodeC(espace);
	generatrice.genere_code(espace.globales, fonctions, enchaineuse);

	enchaineuse.imprime_dans_flux(fichier_sortie);
}

static void genere_code_C_depuis_fonctions_racines(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		std::ostream &fichier_sortie)
{
	Enchaineuse enchaineuse;

	espace.typeuse.construit_table_types();

	auto &graphe = espace.graphe_dependance;
	genere_code_debut_fichier(enchaineuse, compilatrice.racine_kuri);
	genere_code_pour_types(compilatrice, graphe, enchaineuse);

	kuri::tableau<AtomeFonction *> fonctions_racines;
	fonctions_racines.reserve(espace.fonctions.taille());

	POUR_TABLEAU_PAGE (espace.fonctions) {
		if (it.decl && it.decl->possede_drapeau(EST_RACINE)) {
			it.nombre_utilisations = 1;
			fonctions_racines.ajoute(&it);
		}
	}

	if (fonctions_racines.est_vide()) {
		rapporte_erreur_sans_site(&espace, "Aucune fonction racine trouvée pour générer le code !\n");
		return;
	}

	kuri::tableau<AtomeFonction *> fonctions;

	// À FAIRE : parfois les fonctions peuvent être incluses plusieurs fois ? c'est pourquoi nous avons un ensemble « utilises »
	dls::ensemble<AtomeFonction *> utilises;
	POUR (fonctions_racines) {
		auto noeud_dep = it->decl->noeud_dependance;
		graphe->rassemble_fonctions_utilisees(noeud_dep, fonctions, utilises);
	}

	auto generatrice = GeneratriceCodeC(espace);
	generatrice.genere_code(espace.globales, fonctions, enchaineuse);

	enchaineuse.imprime_dans_flux(fichier_sortie);
}

static void genere_code_C(
		Compilatrice &compilatrice,
		ConstructriceRI &constructrice_ri,
		EspaceDeTravail &espace,
		std::ostream &fichier_sortie)
{
	if (espace.options.objet_genere == ObjetGenere::Executable) {
		genere_code_C_depuis_fonction_principale(compilatrice, constructrice_ri, espace, fichier_sortie);
	}
	else {
		genere_code_C_depuis_fonctions_racines(compilatrice, espace, fichier_sortie);
	}
}

static dls::chaine genere_commande_fichier_objet(Compilatrice &compilatrice, OptionsCompilation const &ops)
{
	auto commande = dls::chaine("/usr/bin/gcc-9 -c /tmp/compilation_kuri.c ");

	// À FAIRE : comment lié les tables pour un fichier objet ?
//	if (ops.objet_genere == ObjetGenere::FichierObjet) {
//		commande += "/tmp/tables_r16.o ";
//	}

	/* désactivation des erreurs concernant le manque de "const" quand
	 * on passe des variables générés temporairement par la coulisse à
	 * des fonctions qui dont les paramètres ne sont pas constants */
	commande += "-Wno-discarded-qualifiers ";
	/* désactivation des avertissements de passage d'une variable au
	 * lieu d'une chaine littérale à printf et al. */
	commande += "-Wno-format-security ";

	if (ops.objet_genere == ObjetGenere::FichierObjet) {
		/* À FAIRE : désactivation temporaire du protecteur de pile en attendant d'avoir une manière de le faire depuis les métaprogrammes */
		commande += "-fno-stack-protector ";
	}

	switch (ops.niveau_optimisation) {
		case NiveauOptimisation::AUCUN:
		case NiveauOptimisation::O0:
		{
			commande += "-O0 ";
			break;
		}
		case NiveauOptimisation::O1:
		{
			commande += "-O1 ";
			break;
		}
		case NiveauOptimisation::O2:
		{
			commande += "-O2 ";
			break;
		}
		case NiveauOptimisation::Os:
		{
			commande += "-Os ";
			break;
		}
		/* Oz est spécifique à LLVM, prend O3 car c'est le plus élevé le
		 * plus proche. */
		case NiveauOptimisation::Oz:
		case NiveauOptimisation::O3:
		{
			commande += "-O3 ";
			break;
		}
	}

	if (ops.architecture_cible == ArchitectureCible::X86) {
		commande += "-m32 ";
	}

	for (auto const &def : *compilatrice.definitions.verrou_lecture()) {
		commande += " -D" + dls::chaine(def);
	}

	for (auto const &chm : *compilatrice.chemins.verrou_lecture()) {
		commande += " -L";
		commande += chm;
	}

	if (ops.objet_genere == ObjetGenere::FichierObjet) {
		commande += " -o ";
		commande += dls::chaine(ops.nom_sortie.pointeur, ops.nom_sortie.taille);
		commande += ".o";
	}
	else {
		commande += " -o /tmp/compilation_kuri.o";
	}

	return commande;
}

bool CoulisseC::cree_fichier_objet(Compilatrice &compilatrice, EspaceDeTravail &espace, ConstructriceRI &constructrice_ri)
{
	std::ofstream of;
	of.open("/tmp/compilation_kuri.c");

	std::cout << "Génération du code..." << std::endl;
	auto debut_generation_code = dls::chrono::compte_seconde();
	genere_code_C(compilatrice, constructrice_ri, espace, of);
	temps_generation_code = debut_generation_code.temps();

	of.close();

	auto debut_fichier_objet = dls::chrono::compte_seconde();

	auto commande = genere_commande_fichier_objet(compilatrice, espace.options);

	std::cout << "Exécution de la commande '" << commande << "'..." << std::endl;

	auto err = system(commande.c_str());

	temps_fichier_objet = debut_fichier_objet.temps();

	if (err != 0) {
		std::cerr << "Ne peut pas créer le fichier objet !\n";
		compilatrice.possede_erreur = true;
		return false;
	}

	return true;
}

bool CoulisseC::cree_executable(Compilatrice &compilatrice, EspaceDeTravail &espace)
{
	compile_objet_r16(compilatrice.racine_kuri.c_str(), espace.options.architecture_cible);

	auto debut_executable = dls::chrono::compte_seconde();
	auto commande = dls::chaine("/usr/bin/g++-9 /tmp/compilation_kuri.o ");

	if (espace.options.architecture_cible == ArchitectureCible::X86) {
		commande += " /tmp/r16_tables_x86.o ";
	}
	else {
		commande += " /tmp/r16_tables_x64.o ";
	}

	for (auto const &chm : *compilatrice.chemins.verrou_lecture()) {
		commande += " -L";
		commande += chm;
	}

	for (auto const &bib : *compilatrice.bibliotheques_statiques.verrou_lecture()) {
		commande += " " + bib;
	}

	for (auto const &bib : *compilatrice.bibliotheques_dynamiques.verrou_lecture()) {
		commande += " -l" + bib;
	}

	if (espace.options.architecture_cible == ArchitectureCible::X86) {
		commande += " -m32 ";
	}

	commande += " -o ";
	commande += dls::chaine(espace.options.nom_sortie.pointeur, espace.options.nom_sortie.taille);

	std::cout << "Exécution de la commande '" << commande << "'..." << std::endl;

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas créer l'exécutable !\n";
		compilatrice.possede_erreur = true;
		return false;
	}

	temps_executable = debut_executable.temps();
	return true;
}

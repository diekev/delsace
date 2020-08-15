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

#include "broyage.hh"

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/outils/numerique.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "compilatrice.hh"
#include "modules.hh"

// À FAIRE : supprime les derniers appels à broye_nom_simple dans la génération des noms broyés pour les fonctions
//           il faudra trouver comment calculer la taille de la chaine pour la préfixer (peut-être en prébroyant
//           le nom broyé et en le stockant dans IdentifiantCode ?)

static void broye_nom_simple(Enchaineuse &enchaineuse, dls::vue_chaine_compacte const &nom)
{
	auto debut = &nom[0];
	auto fin   = &nom[nom.taille()];

	while (debut < fin) {
		auto no = lng::nombre_octets(debut);

		switch (no) {
			case 0:
			{
				debut += 1;
				break;
			}
			case 1:
			{
				enchaineuse.pousse_caractere(*debut);
				break;
			}
			default:
			{
				for (int i = 0; i < no; ++i) {
					enchaineuse.pousse_caractere('x');
					enchaineuse.pousse_caractere(dls::num::char_depuis_hex(static_cast<char>((debut[i] & 0xf0) >> 4)));
					enchaineuse.pousse_caractere(dls::num::char_depuis_hex(static_cast<char>(debut[i] & 0x0f)));
				}

				break;
			}
		}

		debut += no;
	}
}

dls::chaine broye_nom_simple(dls::vue_chaine_compacte const &nom)
{
	Enchaineuse enchaineuse;
	broye_nom_simple(enchaineuse, nom);
	return enchaineuse.chaine();
}

/* Broye le nom d'un type.
 *
 * Convention :
 * Kv : argument variadique
 * KP : pointeur
 * KR : référence
 * KT : tableau fixe, suivi de la taille
 * Kt : tableau dynamique
 * Ks : introduit un type scalaire, suivi de la chaine du type
 * Kf : fonction
 * Kc : coroutine
 *
 * Un type scalaire est un type de base, ou un type du programme.
 *
 * Exemples :
 * *z8 devient KPKsz8
 * &[]Foo devient KRKtKsFoo
 */
static void nom_broye_type(Enchaineuse &enchaineuse, Type *type)
{
	switch (type->genre) {
		case GenreType::POLYMORPHIQUE:
		{
			std::cerr << "Obtenu un type polymorphique dans le broyage !\n";
			assert(false);
			break;
		}
		case GenreType::INVALIDE:
		{
			enchaineuse << "Ksinvalide";
			break;
		}
		case GenreType::EINI:
		{
			enchaineuse << "Kseini";
			break;
		}
		case GenreType::CHAINE:
		{
			enchaineuse << "Kschaine";
			break;
		}
		case GenreType::RIEN:
		{
			enchaineuse << "Ksrien";
			break;
		}
		case GenreType::BOOL:
		{
			enchaineuse << "Ksbool";
			break;
		}
		case GenreType::OCTET:
		{
			enchaineuse << "Ksoctet";
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			enchaineuse << "Ksz32";
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			if (type->taille_octet == 1) {
				enchaineuse << "Ksn8";
			}
			else if (type->taille_octet == 2) {
				enchaineuse << "Ksn16";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "Ksn32";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "Ksn64";
			}

			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			if (type->taille_octet == 1) {
				enchaineuse << "Ksz8";
			}
			else if (type->taille_octet == 2) {
				enchaineuse << "Ksz16";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "Ksz32";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "Ksz64";
			}

			break;
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				enchaineuse << "Ksr16";
			}
			else if (type->taille_octet == 4) {
				enchaineuse << "Ksr32";
			}
			else if (type->taille_octet == 8) {
				enchaineuse << "Ksr64";
			}

			break;
		}
		case GenreType::REFERENCE:
		{
			enchaineuse << "KR";
			nom_broye_type(enchaineuse, type->comme_reference()->type_pointe);
			break;
		}
		case GenreType::POINTEUR:
		{
			enchaineuse << "KP";

			auto type_pointe = type->comme_pointeur()->type_pointe;

			if (type_pointe == nullptr) {
				enchaineuse << "nul";
			}
			else {
				nom_broye_type(enchaineuse, type_pointe);
			}

			break;
		}
		case GenreType::UNION:
		{
			auto type_union = static_cast<TypeUnion const *>(type);
			enchaineuse << "Ks";
			broye_nom_simple(enchaineuse, static_cast<TypeUnion const *>(type)->nom);

			// ajout du pointeur au nom afin de différencier les différents types anonymes
			if (type_union->est_anonyme) {
				enchaineuse << type_union;
			}

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_structure = static_cast<TypeStructure const *>(type);
			enchaineuse << "Ks";
			broye_nom_simple(enchaineuse, static_cast<TypeStructure const *>(type)->nom);

			// ajout du pointeur au nom afin de différencier les différents types anonymes
			if (type_structure->est_anonyme) {
				enchaineuse << type_structure;
			}

			break;
		}
		case GenreType::VARIADIQUE:
		{
			auto type_pointe = type->comme_variadique()->type_pointe;

			// les arguments variadiques sont transformés en tableaux, donc utilise Kt
			if (type_pointe != nullptr) {
				enchaineuse << "Kt";
				nom_broye_type(enchaineuse, type_pointe);
			}
			else {
				enchaineuse << "Kv";
			}

			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			enchaineuse << "Kt";
			nom_broye_type(enchaineuse, type->comme_tableau_dynamique()->type_pointe);
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tabl = type->comme_tableau_fixe();

			enchaineuse << "KT";
			enchaineuse << type_tabl->taille;
			nom_broye_type(enchaineuse, type_tabl->type_pointe);
			break;
		}
		case GenreType::FONCTION:
		{
			enchaineuse << "Kf";
			enchaineuse << type;
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum const *>(type);
			enchaineuse << "Ks";
			broye_nom_simple(enchaineuse, type_enum->nom);
			break;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			enchaineuse << "Kstype_de_donnees";
			break;
		}
	}
}

dls::chaine const &nom_broye_type(Type *type)
{
	if (type->nom_broye != "") {
		return type->nom_broye;
	}

	Enchaineuse enchaineuse;
	nom_broye_type(enchaineuse, type);

	type->nom_broye = enchaineuse.chaine();

	return type->nom_broye;
}

/* format :
 * _K préfixe pour tous les noms de Kuri
 * C, E, F, S, U : coroutine, énum, fonction, structure, ou union
 * paire(s) : longueur + chaine ascii pour module et nom
 *
 * pour les fonctions :
 * _E[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 * paire(s) : noms des paramètres et de leurs types, préfixés par leurs tailles
 * _S[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 *
 * types des paramètres pour les fonctions
 *
 * fonc test(x : z32) : z32 (module Test)
 * -> _KF4Test4test_P2_E1_1x3z32_S1_3z32
 */
dls::chaine broye_nom_fonction(
		NoeudDeclarationEnteteFonction *decl,
		dls::chaine const &nom_module)
{
	Enchaineuse enchaineuse;
	auto type_fonc = decl->type->comme_fonction();
	enchaineuse << "_K";

	enchaineuse << (decl->est_coroutine ? "C" : "F");

	/* module */
	auto nom_ascii = broye_nom_simple(nom_module);

	enchaineuse << nom_ascii.taille();
	enchaineuse << nom_ascii;

	/* nom de la fonction */
	if (decl->est_operateur) {
		// XXX - ici ce devrait être nom_ascii, mais sauvons quelques allocations
		enchaineuse << "operateur";

		switch (decl->lexeme->genre) {
			default:
			{
				assert(0);
				break;
			}
			case GenreLexeme::INFERIEUR:
			{
				enchaineuse << "inf";
				break;
			}
			case GenreLexeme::INFERIEUR_EGAL:
			{
				enchaineuse << "infeg";
				break;
			}
			case GenreLexeme::SUPERIEUR:
			{
				enchaineuse << "sup";
				break;
			}
			case GenreLexeme::SUPERIEUR_EGAL:
			{
				enchaineuse << "supeg";
				break;
			}
			case GenreLexeme::DIFFERENCE:
			{
				enchaineuse << "dif";
				break;
			}
			case GenreLexeme::EGALITE:
			{
				enchaineuse << "egl";
				break;
			}
			case GenreLexeme::PLUS:
			{
				enchaineuse << "plus";
				break;
			}
			case GenreLexeme::PLUS_UNAIRE:
			{
				enchaineuse << "pls_unr";
				break;
			}
			case GenreLexeme::MOINS:
			{
				enchaineuse << "moins";
				break;
			}
			case GenreLexeme::MOINS_UNAIRE:
			{
				enchaineuse << "mns_unr";
				break;
			}
			case GenreLexeme::FOIS:
			{
				enchaineuse << "mul";
				break;
			}
			case GenreLexeme::DIVISE:
			{
				enchaineuse << "div";
				break;
			}
			case GenreLexeme::DECALAGE_DROITE:
			{
				enchaineuse << "dcd";
				break;
			}
			case GenreLexeme::DECALAGE_GAUCHE:
			{
				enchaineuse << "dcg";
				break;
			}
			case GenreLexeme::POURCENT:
			{
				enchaineuse << "mod";
				break;
			}
			case GenreLexeme::ESPERLUETTE:
			{
				enchaineuse << "et";
				break;
			}
			case GenreLexeme::BARRE:
			{
				enchaineuse << "ou";
				break;
			}
			case GenreLexeme::TILDE:
			{
				enchaineuse << "non";
				break;
			}
			case GenreLexeme::EXCLAMATION:
			{
				enchaineuse << "excl";
				break;
			}
			case GenreLexeme::CHAPEAU:
			{
				enchaineuse << "oux";
				break;
			}
		}
	}
	else {
		nom_ascii = broye_nom_simple(decl->lexeme->chaine);
	}

	enchaineuse << nom_ascii.taille();
	enchaineuse << nom_ascii;

	/* paramètres */

	/* entrées */
	enchaineuse << "_E";
	enchaineuse << decl->params.taille;
	enchaineuse << "_";

	/* À FAIRE(réusinage arbre) : ajout du contexte */
	POUR (decl->params) {
		auto param = static_cast<NoeudDeclarationVariable *>(nullptr);

		if (it->est_empl()) {
			param = it->comme_empl()->expr->comme_decl_var();
		}
		else {
			param = it->comme_decl_var();
		}

		nom_ascii = broye_nom_simple(param->valeur->ident->nom);
		enchaineuse << nom_ascii.taille();
		enchaineuse << nom_ascii;

		auto const &nom_broye = nom_broye_type(it->type);
		enchaineuse << nom_broye.taille();
		enchaineuse << nom_broye;
	}

	/* sorties */
	enchaineuse << "_S";
	enchaineuse << type_fonc->types_sorties.taille;
	enchaineuse << "_";

	POUR (type_fonc->types_sorties) {
		auto const &nom_broye = nom_broye_type(it);
		enchaineuse << nom_broye.taille();
		enchaineuse << nom_broye;
	}

	return enchaineuse.chaine();
}

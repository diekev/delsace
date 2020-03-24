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

#include "generation_code_asm.hh"

#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntactic.h"
#include "assembleuse_arbre.h"
#include "graphe_dependance.hh"
#include "contexte_generation_code.h"
#include "erreur.h"

// tutorial https://cs.lmu.edu/~ray/notes/nasmtutorial/

// trois sections
// .text pour les fonctions
// .data pour les constantes
// .bss pour réserver de l'espace (p.e. stock_temp: resb 16384)

struct GeneratriceCodeASM {
	// pour les fonctions ou symboles définis dans d'autres objets
	dls::tableau<dls::vue_chaine_compacte> externes{};

	// pour les fonctions ou symboles que nous exportons
	dls::tableau<dls::vue_chaine_compacte> globales{};

	void ajoute_externe(dls::vue_chaine_compacte const &nom)
	{
		externes.pousse(nom);
	}

	void ajoute_globale(dls::vue_chaine_compacte const &nom)
	{
		globales.pousse(nom);
	}

	void ajoute_constante(dls::vue_chaine_compacte const &nom)
	{

	}
};

void genere_code_asm(
		ContexteGenerationCode &contexte,
		NoeudBase *b,
		bool expr_gauche,
		dls::flux_chaine &os)
{
	auto gener = GeneratriceCodeASM();

	switch (b->genre) {
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::INSTRUCTION_SINON:
		case GenreNoeud::RACINE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		{
			break;
		}
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto const est_externe = dls::outils::possede_drapeau(b->drapeaux, EST_EXTERNE);

			if (est_externe) {
				gener.ajoute_externe(b->lexeme->chaine);
				return;
			}

			os << b->lexeme->chaine << ":\n";

			// il faut sauvegarder les registres, et les remettre en place
			// fais de la place pour tous les arguments sur la pile
			os << "\tsub rsp, 8\n";

			// réinitialise la pile
			os << "\tadd rsp, 8\n";

			os << "\tret";

			break;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			// met les arguments dans les registres, ou la pile, selon l'ABI

			// fais de la place pour les valeurs de retours
			os << "\tsub rsp, 8";

			// appel la fonction
			//os << "\tcall " << b->nom_fonction_appel << '\n';

			// s'il y a des arguments sur la pile, il faudra les appelé

			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			break;
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			break;
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_INDICE:
		{
			break;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_TRANSTYPE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_PLAGE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			break;
		}
		case GenreNoeud::DECLARATION_ENUM:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			break;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			break;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			break;
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			break;
		}
	}
}

void genere_code_asm(const assembleuse_arbre &arbre, ContexteGenerationCode &contexte, const dls::chaine &racine_kuri, std::ostream &fichier_sortie)
{
	auto racine = arbre.bloc_courant();

	if (racine == nullptr) {
		return;
	}

	if (racine->genre != GenreNoeud::INSTRUCTION_COMPOSEE) {
		return;
	}

	dls::flux_chaine os;

	POUR (racine->expressions) {
		genere_code_asm(contexte, it, false, os);
	}

	fichier_sortie << os.chn();
}

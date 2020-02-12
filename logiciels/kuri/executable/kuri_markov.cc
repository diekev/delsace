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

#include <filesystem>
#include <iostream>

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/matrice_eparse.hh"

#include "compilation/contexte_generation_code.h"
#include "compilation/lexeuse.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/outils_lexemes.hh"

#include "options.hh"

using type_scalaire = double;
using type_matrice_ep = matrice_colonne_eparse<double>;

static auto idx_depuis_id(GenreLexeme id)
{
	return static_cast<int>(id);
}

static auto id_depuis_idx(int id)
{
	return static_cast<GenreLexeme>(id);
}

static void imprime_mot(GenreLexeme id, std::ostream &os)
{
	switch (id) {
		case GenreLexeme::RETOUR_TYPE:
			os << "->";
			return;
		case GenreLexeme::EXCLAMATION:
			os << "!";
			return;
		case GenreLexeme::GUILLEMET:
			os << "\"";
			return;
		case GenreLexeme::DOLLAR:
			os << "$";
			return;
		case GenreLexeme::POURCENT:
			os << "%";
			return;
		case GenreLexeme::ESPERLUETTE:
			os << "&";
			return;
		case GenreLexeme::APOSTROPHE:
			os << "'";
			return;
		case GenreLexeme::PARENTHESE_OUVRANTE:
			os << "(";
			return;
		case GenreLexeme::PARENTHESE_FERMANTE:
			os << ")";
			return;
		case GenreLexeme::FOIS:
			os << "*";
			return;
		case GenreLexeme::PLUS:
			os << "+";
			return;
		case GenreLexeme::VIRGULE:
			os << ",";
			return;
		case GenreLexeme::MOINS:
			os << "-";
			return;
		case GenreLexeme::POINT:
			os << ".";
			return;
		case GenreLexeme::DIVISE:
			os << "/";
			return;
		case GenreLexeme::DOUBLE_POINTS:
			os << ":";
			return;
		case GenreLexeme::DECLARATION_VARIABLE:
			os << ":=";
			return;
		case GenreLexeme::POINT_VIRGULE:
			os << ";";
			return;
		case GenreLexeme::INFERIEUR:
			os << "<";
			return;
		case GenreLexeme::EGAL:
			os << "=";
			return;
		case GenreLexeme::SUPERIEUR:
			os << ">";
			return;
		case GenreLexeme::AROBASE:
			os << "@";
			return;
		case GenreLexeme::CROCHET_OUVRANT:
			os << "[";
			return;
		case GenreLexeme::CROCHET_FERMANT:
			os << "]";
			return;
		case GenreLexeme::CHAPEAU:
			os << "^";
			return;
		case GenreLexeme::ACCOLADE_OUVRANTE:
			os << "{";
			return;
		case GenreLexeme::BARRE:
			os << "|";
			return;
		case GenreLexeme::ACCOLADE_FERMANTE:
			os << "}";
			return;
		case GenreLexeme::TILDE:
			os << "~";
			return;
		case GenreLexeme::DIFFERENCE:
			os << "!=";
			return;
		case GenreLexeme::DIRECTIVE:
			os << "#";
			return;
		case GenreLexeme::MODULO_EGAL:
			os << "%=";
			return;
		case GenreLexeme::ESP_ESP:
			os << "&&";
			return;
		case GenreLexeme::ET_EGAL:
			os << "&=";
			return;
		case GenreLexeme::MULTIPLIE_EGAL:
			os << "*=";
			return;
		case GenreLexeme::PLUS_EGAL:
			os << "+=";
			return;
		case GenreLexeme::MOINS_EGAL:
			os << "-=";
			return;
		case GenreLexeme::DIVISE_EGAL:
			os << "/=";
			return;
		case GenreLexeme::DECALAGE_GAUCHE:
			os << "<<";
			return;
		case GenreLexeme::INFERIEUR_EGAL:
			os << "<=";
			return;
		case GenreLexeme::EGALITE:
			os << "==";
			return;
		case GenreLexeme::SUPERIEUR_EGAL:
			os << ">=";
			return;
		case GenreLexeme::DECALAGE_DROITE:
			os << ">>=";
			return;
		case GenreLexeme::OUX_EGAL:
			os << "^=";
			return;
		case GenreLexeme::OU_EGAL:
			os << "|=";
			return;
		case GenreLexeme::BARRE_BARRE:
			os << "||";
			return;
		case GenreLexeme::TROIS_POINTS:
			os << "...";
			return;
		case GenreLexeme::DEC_GAUCHE_EGAL:
			os << "<<=";
			return;
		case GenreLexeme::DEC_DROITE_EGAL:
			os << ">>";
			return;
		case GenreLexeme::ARRETE:
			os << "arrête";
			return;
		case GenreLexeme::DISCR:
			os << "discr";
			return;
		case GenreLexeme::BOOL:
			os << "bool";
			return;
		case GenreLexeme::BOUCLE:
			os << "boucle";
			return;
		case GenreLexeme::CHAINE:
			os << "chaine";
			return;
		case GenreLexeme::CONTINUE:
			os << "continue";
			return;
		case GenreLexeme::COROUT:
			os << "corout";
			return;
		case GenreLexeme::DANS:
			os << "dans";
			return;
		case GenreLexeme::DIFFERE:
			os << "diffère";
			return;
		case GenreLexeme::DYN:
			os << "dyn";
			return;
		case GenreLexeme::DELOGE:
			os << "déloge";
			return;
		case GenreLexeme::EINI:
			os << "eini";
			return;
		case GenreLexeme::EMPL:
			os << "empl";
			return;
		case GenreLexeme::EXTERNE:
			os << "externe";
			return;
		case GenreLexeme::FAUX:
			os << "faux";
			return;
		case GenreLexeme::FONC:
			os << "fonc";
			return;
		case GenreLexeme::GARDE:
			os << "garde";
			return;
		case GenreLexeme::IMPORTE:
			os << "importe";
			return;
		case GenreLexeme::INFO_DE:
			os << "info_de";
			return;
		case GenreLexeme::LOGE:
			os << "loge";
			return;
		case GenreLexeme::MEMOIRE:
			os << "mémoire";
			return;
		case GenreLexeme::N16:
			os << "n16";
			return;
		case GenreLexeme::N32:
			os << "n32";
			return;
		case GenreLexeme::N64:
			os << "n64";
			return;
		case GenreLexeme::N8:
			os << "n8";
			return;
		case GenreLexeme::NONSUR:
			os << "nonsûr";
			return;
		case GenreLexeme::NUL:
			os << "nul";
			return;
		case GenreLexeme::OCTET:
			os << "octet";
			return;
		case GenreLexeme::POUR:
			os << "pour";
			return;
		case GenreLexeme::R16:
			os << "r16";
			return;
		case GenreLexeme::R32:
			os << "r32";
			return;
		case GenreLexeme::R64:
			os << "r64";
			return;
		case GenreLexeme::RELOGE:
			os << "reloge";
			return;
		case GenreLexeme::REPETE:
			os << "répète";
			return;
		case GenreLexeme::RETIENS:
			os << "retiens";
			return;
		case GenreLexeme::RETOURNE:
			os << "retourne";
			return;
		case GenreLexeme::RIEN:
			os << "rien";
			return;
		case GenreLexeme::SANSARRET:
			os << "sansarrêt";
			return;
		case GenreLexeme::SAUFSI:
			os << "saufsi";
			return;
		case GenreLexeme::SI:
			os << "si";
			return;
		case GenreLexeme::SINON:
			os << "sinon";
			return;
		case GenreLexeme::STRUCT:
			os << "struct";
			return;
		case GenreLexeme::TAILLE_DE:
			os << "taille_de";
			return;
		case GenreLexeme::TANTQUE:
			os << "tantque";
			return;
		case GenreLexeme::TRANSTYPE:
			os << "transtype";
			return;
		case GenreLexeme::TYPE_DE:
			os << "type_de";
			return;
		case GenreLexeme::UNION:
			os << "union";
			return;
		case GenreLexeme::VRAI:
			os << "vrai";
			return;
		case GenreLexeme::Z16:
			os << "z16";
			return;
		case GenreLexeme::Z32:
			os << "z32";
			return;
		case GenreLexeme::Z64:
			os << "z64";
			return;
		case GenreLexeme::Z8:
			os << "z8";
			return;
		case GenreLexeme::ENUM:
			os << "énum";
			return;
		case GenreLexeme::ENUM_DRAPEAU:
			os << "énum_drapeau";
			return;
		case GenreLexeme::NOMBRE_REEL:
			os << "0.0";
			return;
		case GenreLexeme::NOMBRE_ENTIER:
			os << "0";
			return;
		case GenreLexeme::NOMBRE_HEXADECIMAL:
			os << "0x0";
			return;
		case GenreLexeme::NOMBRE_OCTAL:
			os << "0o0";
			return;
		case GenreLexeme::NOMBRE_BINAIRE:
			os << "0b0";
			return;
		case GenreLexeme::PLUS_UNAIRE:
			os << "+";
			return;
		case GenreLexeme::MOINS_UNAIRE:
			os << "-";
			return;
		case GenreLexeme::CHAINE_CARACTERE:
			os << "chaine_de_caractère";
			return;
		case GenreLexeme::CHAINE_LITTERALE:
			os << "\"chaine littérale\"";
			return;
		case GenreLexeme::CARACTERE:
			os << "'a'";
			return;
		case GenreLexeme::POINTEUR:
			os << "*";
			return;
		case GenreLexeme::TABLEAU:
			os << "[]";
			return;
		case GenreLexeme::REFERENCE:
			os << "&";
			return;
		case GenreLexeme::CHARGE:
			os << "charge";
			return;
		case GenreLexeme::INCONNU:
			os << "inconnu";
			return;
		case GenreLexeme::CARACTERE_BLANC:
			os << " ";
			return;
		case GenreLexeme::COMMENTAIRE:
			os << "# commentaire\n";
			return;
		case GenreLexeme::OPERATEUR:
		{
			os << "opérateur";
			return;
		}
		case GenreLexeme::DEBUT_BLOC_COMMENTAIRE:
		{
			os << "/*";
			return;
		}
		case GenreLexeme::FIN_BLOC_COMMENTAIRE:
		{
			os << "*/";
			return;
		}
		case GenreLexeme::DEBUT_LIGNE_COMMENTAIRE:
		{
			os << "//";
			return;
		}
		case GenreLexeme::DECLARATION_CONSTANTE:
		{
			os << "::";
			return;
		}
		case GenreLexeme::POUSSE_CONTEXTE:
		{
			os << "pousse_contexte";
			return;
		}
	};

	os << "ERREUR";
}

void test_markov_id_simple(dls::tableau<DonneesLexeme> const &lexemes)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

	/* construction de la matrice */
	auto nombre_id = static_cast<int>(GenreLexeme::COMMENTAIRE) + 1;
	auto matrice = type_matrice_ep(type_ligne(nombre_id), type_colonne(nombre_id));

	for (auto i = 0; i < lexemes.taille() - 1; ++i) {
		auto idx0 = idx_depuis_id(lexemes[i].genre);
		auto idx1 = idx_depuis_id(lexemes[i + 1].genre);

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	tri_lignes_matrice(matrice);
	converti_fonction_repartition(matrice);

	auto gna = GNA();
	auto mot_courant = GenreLexeme::STRUCT;
	auto nombre_phrases = 5;

	while (nombre_phrases > 0) {
		imprime_mot(mot_courant, std::cerr);

		if (est_mot_cle(mot_courant)) {
			std::cerr << ' ';
		}

		if (mot_courant == GenreLexeme::ACCOLADE_FERMANTE) {
			std::cerr << '\n';
			nombre_phrases--;
		}

		/* prend le vecteur du mot_courant */
		auto idx = idx_depuis_id(mot_courant);

		/* génère un mot */
		auto prob = gna.uniforme(_0, _1);

		auto &ligne = matrice.lignes[idx];

		for (auto n : ligne) {
			if (prob <= n->valeur) {
				mot_courant = id_depuis_idx(static_cast<int>(n->colonne));
				break;
			}
		}
	}
}

int main(int argc, char **argv)
{
	std::ios::sync_with_stdio(false);

	auto const ops = genere_options_compilation(argc, argv);

	if (ops.erreur) {
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	auto const chemin_fichier = ops.chemin_fichier;

	if (chemin_fichier == nullptr) {
		std::cerr << "Aucun fichier spécifié !\n";
		return 1;
	}

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	try {
		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto contexte = ContexteGenerationCode{};
		auto tampon = charge_fichier(chemin.c_str(), contexte, {});
		auto fichier = contexte.cree_fichier("", chemin.c_str());
		fichier->tampon = lng::tampon_source(tampon);

		auto lexeuse = Lexeuse(fichier);
		lexeuse.performe_lexage();

		test_markov_id_simple(fichier->lexemes);
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}

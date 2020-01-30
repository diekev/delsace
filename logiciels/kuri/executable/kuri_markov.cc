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

static auto idx_depuis_id(TypeLexeme id)
{
	return static_cast<int>(id);
}

static auto id_depuis_idx(int id)
{
	return static_cast<TypeLexeme>(id);
}

static void imprime_mot(TypeLexeme id, std::ostream &os)
{
	switch (id) {
		case TypeLexeme::EXCLAMATION:
			os << "!";
			return;
		case TypeLexeme::GUILLEMET:
			os << "\"";
			return;
		case TypeLexeme::DIESE:
			os << "#";
			return;
		case TypeLexeme::DOLLAR:
			os << "$";
			return;
		case TypeLexeme::POURCENT:
			os << "%";
			return;
		case TypeLexeme::ESPERLUETTE:
			os << "&";
			return;
		case TypeLexeme::APOSTROPHE:
			os << "'";
			return;
		case TypeLexeme::PARENTHESE_OUVRANTE:
			os << "(";
			return;
		case TypeLexeme::PARENTHESE_FERMANTE:
			os << ")";
			return;
		case TypeLexeme::FOIS:
			os << "*";
			return;
		case TypeLexeme::PLUS:
			os << "+";
			return;
		case TypeLexeme::VIRGULE:
			os << ",";
			return;
		case TypeLexeme::MOINS:
			os << "-";
			return;
		case TypeLexeme::POINT:
			os << ".";
			return;
		case TypeLexeme::DIVISE:
			os << "/";
			return;
		case TypeLexeme::DOUBLE_POINTS:
			os << ":";
			return;
		case TypeLexeme::POINT_VIRGULE:
			os << ";";
			return;
		case TypeLexeme::INFERIEUR:
			os << "<";
			return;
		case TypeLexeme::EGAL:
			os << "=";
			return;
		case TypeLexeme::SUPERIEUR:
			os << ">";
			return;
		case TypeLexeme::AROBASE:
			os << "@";
			return;
		case TypeLexeme::CROCHET_OUVRANT:
			os << "[";
			return;
		case TypeLexeme::CROCHET_FERMANT:
			os << "]";
			return;
		case TypeLexeme::CHAPEAU:
			os << "^";
			return;
		case TypeLexeme::ACCOLADE_OUVRANTE:
			os << "{";
			return;
		case TypeLexeme::BARRE:
			os << "|";
			return;
		case TypeLexeme::ACCOLADE_FERMANTE:
			os << "}";
			return;
		case TypeLexeme::TILDE:
			os << "~";
			return;
		case TypeLexeme::DIFFERENCE:
			os << "!=";
			return;
		case TypeLexeme::DIRECTIVE:
			os << "#!";
			return;
		case TypeLexeme::MODULO_EGAL:
			os << "%=";
			return;
		case TypeLexeme::ESP_ESP:
			os << "&&";
			return;
		case TypeLexeme::ET_EGAL:
			os << "&=";
			return;
		case TypeLexeme::MULTIPLIE_EGAL:
			os << "*=";
			return;
		case TypeLexeme::PLUS_EGAL:
			os << "+=";
			return;
		case TypeLexeme::MOINS_EGAL:
			os << "-=";
			return;
		case TypeLexeme::DIVISE_EGAL:
			os << "/=";
			return;
		case TypeLexeme::DECALAGE_GAUCHE:
			os << "<<";
			return;
		case TypeLexeme::INFERIEUR_EGAL:
			os << "<=";
			return;
		case TypeLexeme::EGALITE:
			os << "==";
			return;
		case TypeLexeme::SUPERIEUR_EGAL:
			os << ">=";
			return;
		case TypeLexeme::DECALAGE_DROITE:
			os << ">>=";
			return;
		case TypeLexeme::OUX_EGAL:
			os << "^=";
			return;
		case TypeLexeme::OU_EGAL:
			os << "|=";
			return;
		case TypeLexeme::BARRE_BARRE:
			os << "||";
			return;
		case TypeLexeme::TROIS_POINTS:
			os << "...";
			return;
		case TypeLexeme::DEC_GAUCHE_EGAL:
			os << "<<=";
			return;
		case TypeLexeme::DEC_DROITE_EGAL:
			os << ">>";
			return;
		case TypeLexeme::ARRETE:
			os << "arrête";
			return;
		case TypeLexeme::DISCR:
			os << "discr";
			return;
		case TypeLexeme::BOOL:
			os << "bool";
			return;
		case TypeLexeme::BOUCLE:
			os << "boucle";
			return;
		case TypeLexeme::CHAINE:
			os << "chaine";
			return;
		case TypeLexeme::CONTINUE:
			os << "continue";
			return;
		case TypeLexeme::COROUT:
			os << "corout";
			return;
		case TypeLexeme::DANS:
			os << "dans";
			return;
		case TypeLexeme::DIFFERE:
			os << "diffère";
			return;
		case TypeLexeme::DYN:
			os << "dyn";
			return;
		case TypeLexeme::DELOGE:
			os << "déloge";
			return;
		case TypeLexeme::EINI:
			os << "eini";
			return;
		case TypeLexeme::EMPL:
			os << "empl";
			return;
		case TypeLexeme::EXTERNE:
			os << "externe";
			return;
		case TypeLexeme::FAUX:
			os << "faux";
			return;
		case TypeLexeme::FONC:
			os << "fonc";
			return;
		case TypeLexeme::GARDE:
			os << "garde";
			return;
		case TypeLexeme::IMPORTE:
			os << "importe";
			return;
		case TypeLexeme::INFO_DE:
			os << "info_de";
			return;
		case TypeLexeme::LOGE:
			os << "loge";
			return;
		case TypeLexeme::MEMOIRE:
			os << "mémoire";
			return;
		case TypeLexeme::N128:
			os << "n128";
			return;
		case TypeLexeme::N16:
			os << "n16";
			return;
		case TypeLexeme::N32:
			os << "n32";
			return;
		case TypeLexeme::N64:
			os << "n64";
			return;
		case TypeLexeme::N8:
			os << "n8";
			return;
		case TypeLexeme::NONSUR:
			os << "nonsûr";
			return;
		case TypeLexeme::NUL:
			os << "nul";
			return;
		case TypeLexeme::OCTET:
			os << "octet";
			return;
		case TypeLexeme::POUR:
			os << "pour";
			return;
		case TypeLexeme::R128:
			os << "r128";
			return;
		case TypeLexeme::R16:
			os << "r16";
			return;
		case TypeLexeme::R32:
			os << "r32";
			return;
		case TypeLexeme::R64:
			os << "r64";
			return;
		case TypeLexeme::RELOGE:
			os << "reloge";
			return;
		case TypeLexeme::REPETE:
			os << "répète";
			return;
		case TypeLexeme::RETIENS:
			os << "retiens";
			return;
		case TypeLexeme::RETOURNE:
			os << "retourne";
			return;
		case TypeLexeme::RIEN:
			os << "rien";
			return;
		case TypeLexeme::SANSARRET:
			os << "sansarrêt";
			return;
		case TypeLexeme::SAUFSI:
			os << "saufsi";
			return;
		case TypeLexeme::SI:
			os << "si";
			return;
		case TypeLexeme::SINON:
			os << "sinon";
			return;
		case TypeLexeme::SOIT:
			os << "soit";
			return;
		case TypeLexeme::STRUCT:
			os << "struct";
			return;
		case TypeLexeme::TAILLE_DE:
			os << "taille_de";
			return;
		case TypeLexeme::TANTQUE:
			os << "tantque";
			return;
		case TypeLexeme::TRANSTYPE:
			os << "transtype";
			return;
		case TypeLexeme::TYPE_DE:
			os << "type_de";
			return;
		case TypeLexeme::UNION:
			os << "union";
			return;
		case TypeLexeme::VRAI:
			os << "vrai";
			return;
		case TypeLexeme::Z128:
			os << "z128";
			return;
		case TypeLexeme::Z16:
			os << "z16";
			return;
		case TypeLexeme::Z32:
			os << "z32";
			return;
		case TypeLexeme::Z64:
			os << "z64";
			return;
		case TypeLexeme::Z8:
			os << "z8";
			return;
		case TypeLexeme::ENUM:
			os << "énum";
			return;
		case TypeLexeme::ENUM_DRAPEAU:
			os << "énum_drapeau";
			return;
		case TypeLexeme::NOMBRE_REEL:
			os << "0.0";
			return;
		case TypeLexeme::NOMBRE_ENTIER:
			os << "0";
			return;
		case TypeLexeme::NOMBRE_HEXADECIMAL:
			os << "0x0";
			return;
		case TypeLexeme::NOMBRE_OCTAL:
			os << "0o0";
			return;
		case TypeLexeme::NOMBRE_BINAIRE:
			os << "0b0";
			return;
		case TypeLexeme::PLUS_UNAIRE:
			os << "+";
			return;
		case TypeLexeme::MOINS_UNAIRE:
			os << "-";
			return;
		case TypeLexeme::CHAINE_CARACTERE:
			os << "chaine_de_caractère";
			return;
		case TypeLexeme::CHAINE_LITTERALE:
			os << "\"chaine littérale\"";
			return;
		case TypeLexeme::CARACTERE:
			os << "'a'";
			return;
		case TypeLexeme::POINTEUR:
			os << "*";
			return;
		case TypeLexeme::TABLEAU:
			os << "[]";
			return;
		case TypeLexeme::REFERENCE:
			os << "&";
			return;
		case TypeLexeme::CHARGE:
			os << "charge";
			return;
		case TypeLexeme::INCONNU:
			os << "inconnu";
			return;
		case TypeLexeme::CARACTERE_BLANC:
			os << " ";
			return;
		case TypeLexeme::COMMENTAIRE:
			os << "# commentaire\n";
			return;
	};

	os << "ERREUR";
}

void test_markov_id_simple(dls::tableau<DonneesLexeme> const &morceaux)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

	/* construction de la matrice */
	auto nombre_id = static_cast<int>(TypeLexeme::COMMENTAIRE) + 1;
	auto matrice = type_matrice_ep(type_ligne(nombre_id), type_colonne(nombre_id));

	for (auto i = 0; i < morceaux.taille() - 1; ++i) {
		auto idx0 = idx_depuis_id(morceaux[i].identifiant);
		auto idx1 = idx_depuis_id(morceaux[i + 1].identifiant);

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	tri_lignes_matrice(matrice);
	converti_fonction_repartition(matrice);

	auto gna = GNA();
	auto mot_courant = TypeLexeme::STRUCT;
	auto nombre_phrases = 5;

	while (nombre_phrases > 0) {
		imprime_mot(mot_courant, std::cerr);

		if (est_mot_cle(mot_courant)) {
			std::cerr << ' ';
		}

		if (mot_courant == TypeLexeme::ACCOLADE_FERMANTE) {
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

		test_markov_id_simple(fichier->morceaux);
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}

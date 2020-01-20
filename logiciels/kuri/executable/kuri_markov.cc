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
#include "compilation/decoupeuse.h"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/outils_morceaux.hh"

#include "options.hh"

using type_scalaire = double;
using type_matrice_ep = matrice_colonne_eparse<double>;

static auto idx_depuis_id(id_morceau id)
{
	return static_cast<int>(id);
}

static auto id_depuis_idx(int id)
{
	return static_cast<id_morceau>(id);
}

static void imprime_mot(id_morceau id, std::ostream &os)
{
	switch (id) {
		case id_morceau::EXCLAMATION:
			os << "!";
			return;
		case id_morceau::GUILLEMET:
			os << "\"";
			return;
		case id_morceau::DIESE:
			os << "#";
			return;
		case id_morceau::DOLLAR:
			os << "$";
			return;
		case id_morceau::POURCENT:
			os << "%";
			return;
		case id_morceau::ESPERLUETTE:
			os << "&";
			return;
		case id_morceau::APOSTROPHE:
			os << "'";
			return;
		case id_morceau::PARENTHESE_OUVRANTE:
			os << "(";
			return;
		case id_morceau::PARENTHESE_FERMANTE:
			os << ")";
			return;
		case id_morceau::FOIS:
			os << "*";
			return;
		case id_morceau::PLUS:
			os << "+";
			return;
		case id_morceau::VIRGULE:
			os << ",";
			return;
		case id_morceau::MOINS:
			os << "-";
			return;
		case id_morceau::POINT:
			os << ".";
			return;
		case id_morceau::DIVISE:
			os << "/";
			return;
		case id_morceau::DOUBLE_POINTS:
			os << ":";
			return;
		case id_morceau::POINT_VIRGULE:
			os << ";";
			return;
		case id_morceau::INFERIEUR:
			os << "<";
			return;
		case id_morceau::EGAL:
			os << "=";
			return;
		case id_morceau::SUPERIEUR:
			os << ">";
			return;
		case id_morceau::AROBASE:
			os << "@";
			return;
		case id_morceau::CROCHET_OUVRANT:
			os << "[";
			return;
		case id_morceau::CROCHET_FERMANT:
			os << "]";
			return;
		case id_morceau::CHAPEAU:
			os << "^";
			return;
		case id_morceau::ACCOLADE_OUVRANTE:
			os << "{";
			return;
		case id_morceau::BARRE:
			os << "|";
			return;
		case id_morceau::ACCOLADE_FERMANTE:
			os << "}";
			return;
		case id_morceau::TILDE:
			os << "~";
			return;
		case id_morceau::DIFFERENCE:
			os << "!=";
			return;
		case id_morceau::DIRECTIVE:
			os << "#!";
			return;
		case id_morceau::MODULO_EGAL:
			os << "%=";
			return;
		case id_morceau::ESP_ESP:
			os << "&&";
			return;
		case id_morceau::ET_EGAL:
			os << "&=";
			return;
		case id_morceau::MULTIPLIE_EGAL:
			os << "*=";
			return;
		case id_morceau::PLUS_EGAL:
			os << "+=";
			return;
		case id_morceau::MOINS_EGAL:
			os << "-=";
			return;
		case id_morceau::DIVISE_EGAL:
			os << "/=";
			return;
		case id_morceau::DECALAGE_GAUCHE:
			os << "<<";
			return;
		case id_morceau::INFERIEUR_EGAL:
			os << "<=";
			return;
		case id_morceau::EGALITE:
			os << "==";
			return;
		case id_morceau::SUPERIEUR_EGAL:
			os << ">=";
			return;
		case id_morceau::DECALAGE_DROITE:
			os << ">>=";
			return;
		case id_morceau::OUX_EGAL:
			os << "^=";
			return;
		case id_morceau::OU_EGAL:
			os << "|=";
			return;
		case id_morceau::BARRE_BARRE:
			os << "||";
			return;
		case id_morceau::TROIS_POINTS:
			os << "...";
			return;
		case id_morceau::DEC_GAUCHE_EGAL:
			os << "<<=";
			return;
		case id_morceau::DEC_DROITE_EGAL:
			os << ">>";
			return;
		case id_morceau::ARRETE:
			os << "arrête";
			return;
		case id_morceau::DISCR:
			os << "discr";
			return;
		case id_morceau::BOOL:
			os << "bool";
			return;
		case id_morceau::BOUCLE:
			os << "boucle";
			return;
		case id_morceau::CHAINE:
			os << "chaine";
			return;
		case id_morceau::CONTINUE:
			os << "continue";
			return;
		case id_morceau::COROUT:
			os << "corout";
			return;
		case id_morceau::DANS:
			os << "dans";
			return;
		case id_morceau::DIFFERE:
			os << "diffère";
			return;
		case id_morceau::DYN:
			os << "dyn";
			return;
		case id_morceau::DELOGE:
			os << "déloge";
			return;
		case id_morceau::EINI:
			os << "eini";
			return;
		case id_morceau::EMPL:
			os << "empl";
			return;
		case id_morceau::EXTERNE:
			os << "externe";
			return;
		case id_morceau::FAUX:
			os << "faux";
			return;
		case id_morceau::FONC:
			os << "fonc";
			return;
		case id_morceau::GARDE:
			os << "garde";
			return;
		case id_morceau::IMPORTE:
			os << "importe";
			return;
		case id_morceau::INFO_DE:
			os << "info_de";
			return;
		case id_morceau::LOGE:
			os << "loge";
			return;
		case id_morceau::MEMOIRE:
			os << "mémoire";
			return;
		case id_morceau::N128:
			os << "n128";
			return;
		case id_morceau::N16:
			os << "n16";
			return;
		case id_morceau::N32:
			os << "n32";
			return;
		case id_morceau::N64:
			os << "n64";
			return;
		case id_morceau::N8:
			os << "n8";
			return;
		case id_morceau::NONSUR:
			os << "nonsûr";
			return;
		case id_morceau::NUL:
			os << "nul";
			return;
		case id_morceau::OCTET:
			os << "octet";
			return;
		case id_morceau::POUR:
			os << "pour";
			return;
		case id_morceau::R128:
			os << "r128";
			return;
		case id_morceau::R16:
			os << "r16";
			return;
		case id_morceau::R32:
			os << "r32";
			return;
		case id_morceau::R64:
			os << "r64";
			return;
		case id_morceau::RELOGE:
			os << "reloge";
			return;
		case id_morceau::REPETE:
			os << "répète";
			return;
		case id_morceau::RETIENS:
			os << "retiens";
			return;
		case id_morceau::RETOURNE:
			os << "retourne";
			return;
		case id_morceau::RIEN:
			os << "rien";
			return;
		case id_morceau::SANSARRET:
			os << "sansarrêt";
			return;
		case id_morceau::SAUFSI:
			os << "saufsi";
			return;
		case id_morceau::SI:
			os << "si";
			return;
		case id_morceau::SINON:
			os << "sinon";
			return;
		case id_morceau::SOIT:
			os << "soit";
			return;
		case id_morceau::STRUCT:
			os << "struct";
			return;
		case id_morceau::TAILLE_DE:
			os << "taille_de";
			return;
		case id_morceau::TANTQUE:
			os << "tantque";
			return;
		case id_morceau::TRANSTYPE:
			os << "transtype";
			return;
		case id_morceau::TYPE_DE:
			os << "type_de";
			return;
		case id_morceau::UNION:
			os << "union";
			return;
		case id_morceau::VRAI:
			os << "vrai";
			return;
		case id_morceau::Z128:
			os << "z128";
			return;
		case id_morceau::Z16:
			os << "z16";
			return;
		case id_morceau::Z32:
			os << "z32";
			return;
		case id_morceau::Z64:
			os << "z64";
			return;
		case id_morceau::Z8:
			os << "z8";
			return;
		case id_morceau::ENUM:
			os << "énum";
			return;
		case id_morceau::NOMBRE_REEL:
			os << "0.0";
			return;
		case id_morceau::NOMBRE_ENTIER:
			os << "0";
			return;
		case id_morceau::NOMBRE_HEXADECIMAL:
			os << "0x0";
			return;
		case id_morceau::NOMBRE_OCTAL:
			os << "0o0";
			return;
		case id_morceau::NOMBRE_BINAIRE:
			os << "0b0";
			return;
		case id_morceau::PLUS_UNAIRE:
			os << "+";
			return;
		case id_morceau::MOINS_UNAIRE:
			os << "-";
			return;
		case id_morceau::CHAINE_CARACTERE:
			os << "chaine_de_caractère";
			return;
		case id_morceau::CHAINE_LITTERALE:
			os << "\"chaine littérale\"";
			return;
		case id_morceau::CARACTERE:
			os << "'a'";
			return;
		case id_morceau::POINTEUR:
			os << "*";
			return;
		case id_morceau::TABLEAU:
			os << "[]";
			return;
		case id_morceau::REFERENCE:
			os << "&";
			return;
		case id_morceau::CHARGE:
			os << "charge";
			return;
		case id_morceau::INCONNU:
			os << "inconnu";
			return;
		case id_morceau::CARACTERE_BLANC:
			os << " ";
			return;
		case id_morceau::COMMENTAIRE:
			os << "# commentaire\n";
			return;
	};

	os << "ERREUR";
}

void test_markov_id_simple(dls::tableau<DonneesMorceau> const &morceaux)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

	/* construction de la matrice */
	auto nombre_id = static_cast<int>(id_morceau::COMMENTAIRE) + 1;
	auto matrice = type_matrice_ep(type_ligne(nombre_id), type_colonne(nombre_id));

	for (auto i = 0; i < morceaux.taille() - 1; ++i) {
		auto idx0 = idx_depuis_id(morceaux[i].identifiant);
		auto idx1 = idx_depuis_id(morceaux[i + 1].identifiant);

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	tri_lignes_matrice(matrice);
	converti_fonction_repartition(matrice);

	auto gna = GNA();
	auto mot_courant = id_morceau::STRUCT;
	auto nombre_phrases = 5;

	while (nombre_phrases > 0) {
		imprime_mot(mot_courant, std::cerr);

		if (est_mot_cle(mot_courant)) {
			std::cerr << ' ';
		}

		if (mot_courant == id_morceau::ACCOLADE_FERMANTE) {
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

		auto decoupeuse = decoupeuse_texte(fichier);
		decoupeuse.genere_morceaux();

		test_markov_id_simple(fichier->morceaux);
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}

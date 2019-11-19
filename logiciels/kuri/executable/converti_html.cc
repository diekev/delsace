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

#include "compilation/contexte_generation_code.h"
#include "compilation/decoupeuse.h"
#include "compilation/erreur.h"
#include "compilation/modules.hh"

#include "options.hh"

static auto est_mot_cle(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::FONC:
		case id_morceau::STRUCT:
		case id_morceau::DYN:
		case id_morceau::SOIT:
		case id_morceau::RETOURNE:
		case id_morceau::ENUM:
		case id_morceau::RETIENS:
		case id_morceau::DE:
		case id_morceau::EXTERNE:
		case id_morceau::IMPORTE:
		case id_morceau::POUR:
		case id_morceau::DANS:
		case id_morceau::BOUCLE:
		case id_morceau::TANTQUE:
		case id_morceau::SINON:
		case id_morceau::SI:
		case id_morceau::SAUFSI:
		case id_morceau::LOGE:
		case id_morceau::DELOGE:
		case id_morceau::RELOGE:
		case id_morceau::ASSOCIE:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::Z128:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::N128:
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::R128:
		case id_morceau::EINI:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::CHAINE:
		case id_morceau::OCTET:
		{
			return true;
		}
	}
}

static auto est_chaine_litterale(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::CHAINE_LITTERALE:
		case id_morceau::CARACTERE:
		{
			return true;
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

	std::ostream &os = std::cout;

	try {
		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto contexte = ContexteGenerationCode{};
		auto tampon = charge_fichier(chemin.c_str(), contexte, {});
		auto module = contexte.cree_module("", chemin.c_str());
		module->tampon = lng::tampon_source(tampon);

		auto decoupeuse = decoupeuse_texte(module, INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
		decoupeuse.genere_morceaux();

		for (auto const &morceau : module->morceaux) {
			if (est_mot_cle(morceau.identifiant)) {
				os << "<span class=mot-cle>" << morceau.chaine << "</span>";
			}
			else if (est_chaine_litterale(morceau.identifiant)) {
				os << "<span class=chn-lit>" << morceau.chaine << "</span>";
			}
			else if (morceau.identifiant == id_morceau::COMMENTAIRE) {
				os << "<span class=comment>" << morceau.chaine << "</span>";
			}
			else {
				os << morceau.chaine;
			}
		}
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}

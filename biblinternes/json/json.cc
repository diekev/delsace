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

#include "json.hh"

#include <fstream>

#include "biblinternes/flux/outils.h"
#include "biblinternes/langage/tampon_source.hh"

#include "analyseuse_grammaire.hh"
#include "decoupeuse.hh"
#include "erreur.hh"

namespace json {

static lng::tampon_source charge_fichier(const char *chemin_fichier)
{
	std::ifstream fichier;
	fichier.open(chemin_fichier);

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = fichier.tellg();
	fichier.seekg(0, fichier.beg);

	dls::chaine res;
	res.reserve(taille_fichier);

	dls::flux::pour_chaque_ligne(fichier, [&](dls::chaine const &ligne)
	{
		res += ligne;
		res.ajoute('\n');
	});

	return lng::tampon_source(std::move(res));
}

std::shared_ptr<tori::Objet> compile_script(const char *chemin)
try {
	auto tampon = charge_fichier(chemin);
	auto decoupeuse = decoupeuse_texte(tampon);
	decoupeuse.genere_morceaux();

	auto analyseuse = analyseuse_grammaire(decoupeuse.morceaux(), tampon);
	analyseuse.lance_analyse(std::cout);

	return analyseuse.objet();
}
catch (erreur::frappe const &e) {
	std::cerr << e.message() << '\n';
	return nullptr;
}

}  /* namespace json */

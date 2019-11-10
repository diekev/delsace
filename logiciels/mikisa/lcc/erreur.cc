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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "erreur.h"

#include "biblinternes/structures/flux_chaine.hh"

#include "contexte_generation_code.h"
#include "modules.hh"

namespace erreur {

void lance_erreur(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &morceau,
		type_erreur type)
{
	auto const ligne = static_cast<long>(morceau.ligne_pos >> 32);
	auto const pos_mot = static_cast<long>(morceau.ligne_pos & 0xffffffff);
	auto const identifiant = morceau.identifiant;
	auto const &chaine = morceau.chaine;

	auto module = contexte.module(static_cast<size_t>(morceau.module));
	auto ligne_courante = module->tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

	throw erreur::frappe(ss.chn().c_str(), type);
}

void lance_erreur_plage(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesMorceaux &premier_morceau,
		const DonneesMorceaux &dernier_morceau,
		type_erreur type)
{
	auto const ligne = static_cast<long>(premier_morceau.ligne_pos >> 32);
	auto const pos_premier = static_cast<long>(premier_morceau.ligne_pos & 0xffffffff);
	auto const pos_dernier = static_cast<long>(dernier_morceau.ligne_pos & 0xffffffff);

	auto module = contexte.module(static_cast<size_t>(premier_morceau.module));
	auto ligne_courante = module->tampon[ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	lng::erreur::imprime_caractere_vide(ss, pos_premier, ligne_courante);
	ss << '^';
	lng::erreur::imprime_tilde(ss, ligne_courante, pos_premier, pos_dernier + 1);
	ss << '\n';

	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), type);
}

}

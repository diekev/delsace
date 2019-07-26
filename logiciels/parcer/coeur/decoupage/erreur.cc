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

#include "erreur.hh"

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "arbre_syntactic.hh"
#include "contexte_generation_code.hh"
#include "donnees_type.hh"
#include "modules.hh"
#include "morceaux.hh"

namespace erreur {

frappe::frappe(const char *message, type_erreur type)
	: m_message(message)
	, m_type(type)
{}

type_erreur frappe::type() const
{
	return m_type;
}

const char *frappe::message() const
{
	return m_message.c_str();
}

static void imprime_caractere_vide(dls::flux_chaine &os, const long nombre, const dls::vue_chaine &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0l; i < std::min(nombre, chaine.taille());) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}

		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine)
{
	for (auto i = 0l; i < chaine.taille() - 1;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_tilde(dls::flux_chaine &os, const dls::vue_chaine &chaine, long debut, long fin)
{
	for (auto i = debut; i < fin;) {
		os << '~';
		i += lng::decalage_pour_caractere(chaine, i);
	}
}

static void imprime_ligne_entre(
		dls::flux_chaine &os,
		const dls::vue_chaine &chaine,
		long debut,
		long fin)
{
	for (auto i = debut; i < fin; ++i) {
		os << chaine[i];
	}
}

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
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	imprime_tilde(ss, chaine);
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
	ss << "Erreur : " << module->chemin << ':' << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_premier, ligne_courante);
	ss << '^';
	imprime_tilde(ss, ligne_courante, pos_premier, pos_dernier + 1);
	ss << '\n';

	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), type);
}

}

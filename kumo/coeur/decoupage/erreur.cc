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

#include <sstream>

#include "morceaux.hh"
#include "tampon_source.hh"
#include "unicode.hh"

namespace erreur {

frappe::frappe(const char *message, int type)
	: m_message(message)
	, m_type(type)
{}

int frappe::type() const
{
	return m_type;
}

const char *frappe::message() const
{
	return m_message.c_str();
}

static void imprime_caractere_vide(std::ostream &os, const size_t nombre, const std::string_view &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < nombre; i += static_cast<size_t>(nombre_octets(&chaine[i]))) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}
	}
}

static void imprime_tilde(std::ostream &os, const std::string_view &chaine)
{
	for (size_t i = 0; i < chaine.size() - 1; i += static_cast<size_t>(nombre_octets(&chaine[i]))) {
		os << '~';
	}
}

void lance_erreur(const std::string &quoi, const TamponSource &tampon, const DonneesMorceaux &morceau, int type)
{
	const auto ligne = morceau.ligne_pos >> 32;
	const auto pos_mot = morceau.ligne_pos & 0xffffffff;
	const auto identifiant = morceau.identifiant;
	const auto &chaine = morceau.chaine;

	auto ligne_courante = tampon[ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << ligne + 1 << ":\n";
	ss << ligne_courante;

	imprime_caractere_vide(ss, pos_mot, ligne_courante);
	ss << '^';
	imprime_tilde(ss, chaine);
	ss << '\n';

	ss << quoi;
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(int(identifiant)) << ')';

	throw erreur::frappe(ss.str().c_str(), type);
}

[[noreturn]] void lance_erreur_nombre_arguments(
		const size_t nombre_arguments,
		const size_t nombre_recus,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau)
{
	const auto numero_ligne = morceau.ligne_pos >> 32;
	const auto pos_mot = morceau.ligne_pos & 0xffffffff;
	const auto ligne = tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Le nombre d'arguments de la fonction est incorrect.\n";
	ss << "Requiers : " << nombre_arguments << '\n';
	ss << "Obtenu : " << nombre_recus << '\n';

	throw frappe(ss.str().c_str(), NOMBRE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_arguments(
		const int type_arg,
		const int type_enf,
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau)
{
	const auto numero_ligne = morceau.ligne_pos >> 32;
	const auto pos_mot = morceau.ligne_pos & 0xffffffff;
	const auto ligne = tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Fonction : '" << morceau.chaine << "', argument " << nom_arg << '\n';
	ss << "Les types d'arguments ne correspondent pas !\n";
	ss << "Requiers " << chaine_identifiant(type_arg) << '\n';
	ss << "Obtenu " << chaine_identifiant(type_enf) << '\n';
	throw frappe(ss.str().c_str(), TYPE_ARGUMENT);
}

[[noreturn]] void lance_erreur_argument_inconnu(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau)
{
	const auto numero_ligne = morceau.ligne_pos >> 32;
	const auto pos_mot = morceau.ligne_pos & 0xffffffff;
	const auto ligne = tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Fonction : '" << morceau.chaine
	   << "', argument nommé '" << nom_arg << "' inconnu !\n";

	throw frappe(ss.str().c_str(), ARGUMENT_INCONNU);
}

[[noreturn]] void lance_erreur_redeclaration_argument(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau)
{
	const auto numero_ligne = morceau.ligne_pos >> 32;
	const auto pos_mot = morceau.ligne_pos & 0xffffffff;
	const auto ligne = tampon[numero_ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << numero_ligne + 1 << ":\n";
	ss << ligne;

	imprime_caractere_vide(ss, pos_mot, ligne);
	ss << '^';
	imprime_tilde(ss, morceau.chaine);
	ss << '\n';

	ss << "Fonction : '" << morceau.chaine
	   << "', redéclaration de l'argument '" << nom_arg << "' !\n";

	throw frappe(ss.str().c_str(), ARGUMENT_REDEFINI);
}

}

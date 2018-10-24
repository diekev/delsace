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

#include <sstream>

#include "donnees_type.h"
#include "morceaux.h"
#include "tampon_source.h"
#include "unicode.h"

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

static void imprime_caractere_vide(std::ostream &os, const size_t nombre, const std::string_view &chaine)
{
	/* Le 'nombre' est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < nombre;) {
		if (chaine[i] == '\t') {
			os << '\t';
		}
		else {
			os << ' ';
		}

		/* il est possible que l'on reçoive un caractère unicode invalide, donc
		 * on incrémente au minimum de 1 pour ne pas être bloqué dans une
		 * boucle infinie. À FAIRE : trouver mieux */
		auto n = std::max(1, nombre_octets(&chaine[i]));
		i += static_cast<size_t>(n);
	}
}

static void imprime_tilde(std::ostream &os, const std::string_view &chaine)
{
	for (size_t i = 0; i < chaine.size() - 1;) {
		os << '~';

		/* il est possible que l'on reçoive un caractère unicode invalide, donc
		 * on incrémente au minimum de 1 pour ne pas être bloqué dans une
		 * boucle infinie. À FAIRE : trouver mieux */
		auto n = std::max(1, nombre_octets(&chaine[i]));
		i += static_cast<size_t>(n);
	}
}

void lance_erreur(
		const std::string &quoi,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau,
		type_erreur type)
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
	ss << ", obtenu : " << chaine << " (" << chaine_identifiant(identifiant) << ')';

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

	throw frappe(ss.str().c_str(), type_erreur::NOMBRE_ARGUMENT);
}

[[noreturn]] void lance_erreur_type_arguments(
		const DonneesType &type_arg,
		const DonneesType &type_enf,
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
	ss << "Requiers : " << type_arg << '\n';
	ss << "Obtenu   : " << type_enf << '\n';
	throw frappe(ss.str().c_str(), type_erreur::TYPE_ARGUMENT);
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

	throw frappe(ss.str().c_str(), type_erreur::ARGUMENT_INCONNU);
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

	throw frappe(ss.str().c_str(), type_erreur::ARGUMENT_REDEFINI);
}

[[noreturn]] void lance_erreur_assignation_type_differents(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
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

	ss << "Ne peut pas assigner des types différents !\n";
	ss << "Type à gauche : " << type_gauche << '\n';
	ss << "Type à droite : " << type_droite << '\n';

	throw frappe(ss.str().c_str(), type_erreur::ASSIGNATION_MAUVAIS_TYPE);
}

void lance_erreur_type_operation(
		const DonneesType &type_gauche,
		const DonneesType &type_droite,
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

	ss << "Les types de l'opération sont différents !\n";
	ss << "Type à gauche : " << type_gauche << '\n';
	ss << "Type à droite : " << type_droite << '\n';

	throw frappe(ss.str().c_str(), type_erreur::TYPE_DIFFERENTS);
}

}

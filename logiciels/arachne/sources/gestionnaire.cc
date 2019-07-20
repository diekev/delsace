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

#include "gestionnaire.h"

#include "biblinternes/systeme_fichier/utilitaires.h"

namespace arachne {

gestionnaire::gestionnaire()
	: m_chemin_racine(dls::systeme_fichier::chemin_repertoire_maison() / "arachne")
	, m_chemin_courant("")
	, m_base_courante("")
{
	if (!std::experimental::filesystem::exists(m_chemin_racine)) {
		std::fprintf(stderr, "Création du dossier '%s'\n", m_chemin_racine.c_str());
		std::experimental::filesystem::create_directory(m_chemin_racine);
	}
}

void gestionnaire::cree_base_donnees(const dls::chaine &nom)
{
	const auto chemin = m_chemin_racine / nom.c_str();

	if (!std::experimental::filesystem::exists(chemin)) {
		std::fprintf(stderr, "Création du dossier '%s'\n", chemin.c_str());
		std::experimental::filesystem::create_directory(chemin);
	}

	m_chemin_courant = chemin;
	m_base_courante = nom;
}

void gestionnaire::supprime_base_donnees(const dls::chaine &nom)
{
	auto chemin = m_chemin_racine / nom.c_str();

	if (!std::experimental::filesystem::exists(chemin)) {
		return;
	}

	std::experimental::filesystem::remove(chemin);
	m_base_courante = "";
	m_chemin_courant = "";
}

bool gestionnaire::change_base_donnees(const dls::chaine &nom)
{
	auto chemin = m_chemin_racine / nom.c_str();

	if (!std::experimental::filesystem::exists(chemin)) {
		return false;
	}

	m_chemin_courant = chemin;
	m_base_courante = nom;

	return true;
}

std::experimental::filesystem::path gestionnaire::chemin_racine() const
{
	return m_chemin_racine;
}

std::experimental::filesystem::path gestionnaire::chemin_courant() const
{
	return m_chemin_courant;
}

dls::chaine gestionnaire::base_courante() const
{
	return m_base_courante;
}

}  /* namespace arachne */

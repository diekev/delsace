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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "test_unitaire.hh"

#include <execinfo.h>

namespace dls {
namespace test_unitaire {

static constexpr auto TAILLE_TRACAGE = 10;

Controleuse::Controleuse(std::ostream &os)
	: m_flux(os)
	, m_total(0)
	, m_fonctions()
	, m_echecs()
{}

void Controleuse::verifie(
		const bool condition,
		const char *cond_str,
		const char *fichier,
		const int ligne)
{
	if (!condition) {
		std::stringstream ss;

		ss << fichier << ":" << ligne
		   << "\n\tLa condition '" << cond_str
		   << "' n'est pas remplie.";

		pousse_erreur(ss.str());
	}

	m_flux << ".";
	++m_total;
}

void Controleuse::debute_proposition(const char *raison)
{
	m_proposition = raison;
}

void Controleuse::debute_proposition(const std::string &raison)
{
	m_proposition = raison;
}

void Controleuse::termine_proposition()
{
	m_proposition = "";
}

void Controleuse::imprime_resultat()
{
	const auto &nombre_echecs = m_echecs.size();

	m_flux << "\nSUCCÈS (" << m_total - nombre_echecs << ")\n";

	if (nombre_echecs != 0) {
		m_flux << "ÉCHECS (" << nombre_echecs << ")\n";

		for (const auto &fail : m_echecs) {
			for (size_t i = 0; i < m_taille_max_erreur; ++i) {
				m_flux << '-';
			}
			m_flux << '\n';
			m_flux << fail;
			for (size_t i = 0; i < m_taille_max_erreur; ++i) {
				m_flux << '-';
			}
			m_flux << '\n';
		}
	}
}

void Controleuse::performe_controles()
{
	for (const auto &fonction : m_fonctions) {
		fonction(*this);
	}
}

void Controleuse::pousse_erreur(const std::string &erreur)
{
	std::stringstream ss;
	ss << "Proposition : " << m_proposition << "\n\n";
	ss << erreur << '\n';

	void *tampon[TAILLE_TRACAGE];
	auto taille_tracage = backtrace(tampon, TAILLE_TRACAGE);

	auto symboles = backtrace_symbols(tampon, taille_tracage);

	ss << '\n';
	for (int i = 0; i < taille_tracage; ++i) {
		ss << symboles[i] << '\n';
	}
	ss << '\n';

	free(symboles);

	auto chaine = ss.str();

	if (erreur.size() > m_taille_max_erreur) {
		m_taille_max_erreur = erreur.size();
	}

	m_echecs.push_back(chaine);
}

}  /* namespace test_unitaire */
}  /* namespace dls */

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

#include "donnees_type.hh"

#include "biblinternes/structures/flux_chaine.hh"

#include "contexte_generation_code.hh"

DonneesType::DonneesType(id_morceau i0)
{
	m_donnees.pousse(i0);
}

DonneesType::DonneesType(id_morceau i0, id_morceau i1)
{
	m_donnees.pousse(i0);
	m_donnees.pousse(i1);
}

void DonneesType::pousse(id_morceau identifiant)
{
	m_donnees.pousse(identifiant);
}

void DonneesType::pousse(const DonneesType &autre)
{
	auto const taille = m_donnees.taille();
	m_donnees.redimensionne(taille + autre.m_donnees.taille());
	std::copy(autre.m_donnees.debut(), autre.m_donnees.fin(), m_donnees.debut() + static_cast<long>(taille));
}

id_morceau DonneesType::type_base() const
{
	return m_donnees.front();
}

bool DonneesType::est_invalide() const
{
	if (m_donnees.est_vide()) {
		return true;
	}

	switch (m_donnees.back()) {
		case id_morceau::POINTEUR:
		case id_morceau::REFERENCE:
		case id_morceau::TABLEAU:
			return true;
		default:
			return false;
	}
}

DonneesType::iterateur_const DonneesType::begin() const
{
	return m_donnees.debut_inverse();
}

DonneesType::iterateur_const DonneesType::end() const
{
	return m_donnees.fin_inverse();
}

DonneesType DonneesType::derefence() const
{
	auto donnees = DonneesType{};

	for (long i = 1; i < m_donnees.taille(); ++i) {
		donnees.pousse(m_donnees[i]);
	}

	return donnees;
}

dls::chaine chaine_type(DonneesType const &donnees_type, ContexteGenerationCode const &contexte)
{
	dls::flux_chaine os;

	if (donnees_type.est_invalide()) {
		os << "type invalide";
	}
	else {
		auto debut = donnees_type.end() - 1;
		auto fin = donnees_type.begin() - 1;

		for (;debut != fin; --debut) {
			auto donnee = *debut;
			switch (donnee & 0xff) {
				case id_morceau::TROIS_POINTS:
				{
					os << "...";
					break;
				}
				case id_morceau::POINTEUR:
					os << '*';
					break;
				case id_morceau::TABLEAU:
					os << '[';

					if (static_cast<size_t>(donnee >> 8) != 0) {
						os << static_cast<size_t>(donnee >> 8);
					}

					os << ']';
					break;
				case id_morceau::BOOL:
					os << "bool";
					break;
				case id_morceau::PARENTHESE_OUVRANTE:
					os << '(';
					break;
				case id_morceau::PARENTHESE_FERMANTE:
					os << ')';
					break;
				case id_morceau::VIRGULE:
					os << ',';
					break;
				case id_morceau::CHAINE_CARACTERE:
				{
					auto id = static_cast<long>(donnee >> 8);
					os << contexte.nom_struct(id);
					break;
				}
				default:
				{
					os << "INVALIDE";
					break;
				}
			}
		}
	}

	return os.chn();
}

/* ************************************************************************** */

long MagasinDonneesType::ajoute_type(const DonneesType &donnees)
{
	auto iter = donnees_type_index.trouve(donnees);

	if (iter != donnees_type_index.fin()) {
		return iter->second;
	}

	auto index = donnees_types.taille();
	donnees_types.pousse(donnees);

	donnees_type_index.insere({donnees, index});

	return index;
}

/* ************************************************************************** */

/**
 * Retourne un vecteur contenant les DonneesType de chaque paramètre et du type
 * de retour d'un DonneesType d'un pointeur fonction. Si le DonneesType passé en
 * paramètre n'est pas un pointeur fonction, retourne un vecteur vide.
 */
[[nodiscard]] auto donnees_types_parametres(
		const DonneesType &donnees_type) noexcept(false) -> dls::tableau<DonneesType>
{
//	if (donnees_type.type_base() != id_morceau::FONC) {
//		return {};
//	}

	auto dt = DonneesType{};
	dls::tableau<DonneesType> donnees_types;

	auto debut = donnees_type.end() - 1;
	auto fin   = donnees_type.begin() - 1;

	--debut; /* fonction */
	--debut; /* ( */

	/* type paramètres */
	while (*debut != id_morceau::PARENTHESE_FERMANTE) {
		while (*debut != id_morceau::PARENTHESE_FERMANTE) {
			dt.pousse(*debut--);

			if (*debut == id_morceau::VIRGULE) {
				--debut;
				break;
			}
		}

		donnees_types.pousse(dt);

		dt = DonneesType{};
	}

	--debut; /* ) */

	/* type retour */
	while (debut != fin) {
		dt.pousse(*debut--);
	}

	donnees_types.pousse(dt);

	return donnees_types;
}

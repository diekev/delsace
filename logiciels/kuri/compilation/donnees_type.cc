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

#include "donnees_type.h"

#include "arbre_syntactic.h"
#include "broyage.hh"
#include "contexte_generation_code.h"
#include "outils_morceaux.hh"

/* ************************************************************************** */

id_morceau DonneesTypeDeclare::type_base() const
{
	if (donnees.est_vide()) {
		return id_morceau::INCONNU;
	}

	return donnees[0];
}

long DonneesTypeDeclare::taille() const
{
	return donnees.taille();
}

id_morceau DonneesTypeDeclare::operator[](long idx) const
{
	return donnees[idx];
}

void DonneesTypeDeclare::pousse(id_morceau id)
{
	donnees.pousse(id);
}

void DonneesTypeDeclare::pousse(const DonneesTypeDeclare &dtd)
{
	for (auto i = 0; i < dtd.donnees.taille(); ++i) {
		donnees.pousse(dtd.donnees[i]);
	}

	for (auto i = 0; i < dtd.expressions.taille(); ++i) {
		expressions.pousse(dtd.expressions[i]);
	}
}

DonneesTypeDeclare::type_plage DonneesTypeDeclare::plage() const
{
	return type_plage(donnees.donnees(), donnees.donnees() + donnees.taille());
}

DonneesTypeDeclare::type_plage DonneesTypeDeclare::dereference() const
{
	auto p = plage();
	p.effronte();
	return p;
}

/* ************************************************************************** */

DonneesTypeFinal::DonneesTypeFinal(id_morceau i0)
{
	m_donnees.pousse(i0);
}

DonneesTypeFinal::DonneesTypeFinal(id_morceau i0, id_morceau i1)
{
	m_donnees.pousse(i0);
	m_donnees.pousse(i1);
}

DonneesTypeFinal::DonneesTypeFinal(type_plage_donnees_type autre)
{
	pousse(autre);
}

void DonneesTypeFinal::pousse(id_morceau identifiant)
{
	m_donnees.pousse(identifiant);
}

void DonneesTypeFinal::pousse(const DonneesTypeFinal &autre)
{
	auto const taille = m_donnees.taille();
	m_donnees.redimensionne(taille + autre.m_donnees.taille());
	std::copy(autre.m_donnees.debut(), autre.m_donnees.fin(), m_donnees.debut() + taille);
}

void DonneesTypeFinal::pousse(type_plage_donnees_type autre)
{
	auto const taille = m_donnees.taille();
	m_donnees.reserve(taille + autre.taille());

	while (!autre.est_finie()) {
		m_donnees.pousse(autre.front());
		autre.effronte();
	}
}

id_morceau DonneesTypeFinal::type_base() const
{
	return m_donnees.front();
}

bool DonneesTypeFinal::est_invalide() const
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

DonneesTypeFinal::iterateur_const DonneesTypeFinal::begin() const
{
	return m_donnees.debut_inverse();
}

DonneesTypeFinal::iterateur_const DonneesTypeFinal::end() const
{
	return m_donnees.fin_inverse();
}

type_plage_donnees_type DonneesTypeFinal::plage() const
{
	auto d = m_donnees.donnees();
	auto f = d + this->taille();

	return type_plage(d, f);
}

type_plage_donnees_type DonneesTypeFinal::dereference() const
{
	auto p = plage();
	p.effronte();
	return p;
}

long DonneesTypeFinal::taille() const
{
	return m_donnees.taille();
}

dls::chaine chaine_type(DonneesTypeFinal const &donnees_type, ContexteGenerationCode const &contexte)
{
	dls::flux_chaine os;

	if (donnees_type.est_invalide()) {
		os << "type invalide";
	}
	else {
		auto plage = donnees_type.plage();

		while (!plage.est_finie()) {
			auto donnee = plage.front();
			plage.effronte();

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
				case id_morceau::N8:
					os << "n8";
					break;
				case id_morceau::N16:
					os << "n16";
					break;
				case id_morceau::N32:
					os << "n32";
					break;
				case id_morceau::N64:
					os << "n64";
					break;
				case id_morceau::N128:
					os << "n128";
					break;
				case id_morceau::R16:
					os << "r16";
					break;
				case id_morceau::R32:
					os << "r32";
					break;
				case id_morceau::R64:
					os << "r64";
					break;
				case id_morceau::R128:
					os << "r128";
					break;
				case id_morceau::Z8:
					os << "z8";
					break;
				case id_morceau::Z16:
					os << "z16";
					break;
				case id_morceau::Z32:
					os << "z32";
					break;
				case id_morceau::Z64:
					os << "z64";
					break;
				case id_morceau::Z128:
					os << "z128";
					break;
				case id_morceau::BOOL:
					os << "bool";
					break;
				case id_morceau::CHAINE:
					os << "chaine";
					break;
				case id_morceau::FONC:
					os << "fonc";
					break;
				case id_morceau::COROUT:
					os << "corout";
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
				case id_morceau::EINI:
					os << "eini";
					break;
				case id_morceau::RIEN:
					os << "rien";
					break;
				case id_morceau::OCTET:
					os << "octet";
					break;
				case id_morceau::NUL:
					os << "nul";
					break;
				case id_morceau::REFERENCE:
				{
					os << "&";
					break;
				}
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

unsigned alignement(
		ContexteGenerationCode &contexte,
		const DonneesTypeFinal &donnees_type)
{
	id_morceau identifiant = donnees_type.type_base();

	switch (identifiant & 0xff) {
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
			return 1;
		case id_morceau::R16:
		case id_morceau::N16:
		case id_morceau::Z16:
			return 2;
		case id_morceau::R32:
		case id_morceau::N32:
		case id_morceau::Z32:
			return 4;
		case id_morceau::TABLEAU:
		{
			if (size_t(identifiant >> 8) == 0) {
				return 8;
			}

			return alignement(contexte, donnees_type.dereference());
		}
		case id_morceau::FONC:
		case id_morceau::POINTEUR:
		case id_morceau::REFERENCE:
		case id_morceau::EINI:
		case id_morceau::R64:
		case id_morceau::N64:
		case id_morceau::Z64:
		case id_morceau::N128:
		case id_morceau::Z128:
		case id_morceau::R128:
		case id_morceau::CHAINE:
			return 8;
		case id_morceau::CHAINE_CARACTERE:
		{
			auto const &id_structure = (static_cast<long>(identifiant) & 0xffffff00) >> 8;
			auto &ds = contexte.donnees_structure(id_structure);

			if (ds.est_enum) {
				auto dt_enum = contexte.typeuse[ds.noeud_decl->index_type];
				return alignement(contexte, dt_enum);
			}

			auto a = 0u;

			for (auto const &donnees : ds.index_types) {
				auto const &dt = contexte.typeuse[donnees];
				a = std::max(a, alignement(contexte, dt));
			}

			return a;
		}
		default:
			assert(false);
	}

	return 0;
}

/* ************************************************************************** */

unsigned int taille_octet_type(
		ContexteGenerationCode const &contexte,
		const DonneesTypeFinal &donnees_type)
{
	auto type_base = donnees_type.type_base();

	switch (type_base & 0xff) {
		default:
		{
			assert(false);
			break;
		}
		case id_morceau::OCTET:
		case id_morceau::BOOL:
		case id_morceau::N8:
		case id_morceau::Z8:
		{
			return 1;
		}
		case id_morceau::N16:
		case id_morceau::Z16:
		{
			return 2;
		}
		case id_morceau::R16:
		{
			return 2;
		}
		case id_morceau::N32:
		case id_morceau::Z32:
		case id_morceau::R32:
		{
			return 4;
		}
		case id_morceau::N64:
		case id_morceau::Z64:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case id_morceau::R64:
		{
			return 8;
		}
		case id_morceau::N128:
		case id_morceau::Z128:
		case id_morceau::R128:
		{
			return 16;
		}
		case id_morceau::CHAINE_CARACTERE:
		{
			auto index_struct = static_cast<long>(type_base >> 8);
			auto &ds = contexte.donnees_structure(index_struct);

			if (ds.est_enum) {
				auto dt_enum = contexte.typeuse[ds.noeud_decl->index_type];
				return taille_octet_type(contexte, dt_enum);
			}

			return ds.taille_octet;
		}
		case id_morceau::POINTEUR:
		case id_morceau::FONC:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case id_morceau::TABLEAU:
		case id_morceau::EINI:
		case id_morceau::CHAINE:
		{
			if (contexte.bit32) {
				return 8;
			}

			return 16;
		}
		case id_morceau::RIEN:
		{
			return 0;
		}
	}

	return 0;
}

void ajoute_contexte_programme(ContexteGenerationCode &contexte, DonneesTypeDeclare &dt)
{
	auto ds = DonneesStructure();

	if (contexte.structure_existe("ContexteProgramme")) {
		ds = contexte.donnees_structure("ContexteProgramme");
	}
	else {
		contexte.ajoute_donnees_structure("ContexteProgramme", ds);
	}

	dt.pousse(id_morceau::POINTEUR);
	dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(ds.id << 8));
}

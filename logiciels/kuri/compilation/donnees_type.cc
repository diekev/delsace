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
#include "outils_lexemes.hh"

/* ************************************************************************** */

GenreLexeme DonneesTypeDeclare::type_base() const
{
	if (donnees.est_vide()) {
		return GenreLexeme::INCONNU;
	}

	return donnees[0];
}

long DonneesTypeDeclare::taille() const
{
	return donnees.taille();
}

GenreLexeme DonneesTypeDeclare::operator[](long idx) const
{
	return donnees[idx];
}

void DonneesTypeDeclare::pousse(GenreLexeme id)
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

DonneesTypeFinal::DonneesTypeFinal(GenreLexeme i0)
{
	m_donnees.pousse(i0);
}

DonneesTypeFinal::DonneesTypeFinal(GenreLexeme i0, GenreLexeme i1)
{
	m_donnees.pousse(i0);
	m_donnees.pousse(i1);
}

DonneesTypeFinal::DonneesTypeFinal(type_plage_donnees_type autre)
{
	pousse(autre);
}

void DonneesTypeFinal::pousse(GenreLexeme identifiant)
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

GenreLexeme DonneesTypeFinal::type_base() const
{
	return m_donnees.front();
}

bool DonneesTypeFinal::est_invalide() const
{
	if (m_donnees.est_vide()) {
		return true;
	}

	switch (m_donnees.back()) {
		case GenreLexeme::POINTEUR:
		case GenreLexeme::REFERENCE:
		case GenreLexeme::TABLEAU:
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
				case GenreLexeme::TROIS_POINTS:
				{
					os << "...";
					break;
				}
				case GenreLexeme::POINTEUR:
					os << '*';
					break;
				case GenreLexeme::TABLEAU:
					os << '[';

					if (static_cast<size_t>(donnee >> 8) != 0) {
						os << static_cast<size_t>(donnee >> 8);
					}

					os << ']';
					break;
				case GenreLexeme::N8:
					os << "n8";
					break;
				case GenreLexeme::N16:
					os << "n16";
					break;
				case GenreLexeme::N32:
					os << "n32";
					break;
				case GenreLexeme::N64:
					os << "n64";
					break;
				case GenreLexeme::R16:
					os << "r16";
					break;
				case GenreLexeme::R32:
					os << "r32";
					break;
				case GenreLexeme::R64:
					os << "r64";
					break;
				case GenreLexeme::Z8:
					os << "z8";
					break;
				case GenreLexeme::Z16:
					os << "z16";
					break;
				case GenreLexeme::Z32:
					os << "z32";
					break;
				case GenreLexeme::Z64:
					os << "z64";
					break;
				case GenreLexeme::BOOL:
					os << "bool";
					break;
				case GenreLexeme::CHAINE:
					os << "chaine";
					break;
				case GenreLexeme::FONC:
					os << "fonc";
					break;
				case GenreLexeme::COROUT:
					os << "corout";
					break;
				case GenreLexeme::PARENTHESE_OUVRANTE:
					os << '(';
					break;
				case GenreLexeme::PARENTHESE_FERMANTE:
					os << ')';
					break;
				case GenreLexeme::VIRGULE:
					os << ',';
					break;
				case GenreLexeme::EINI:
					os << "eini";
					break;
				case GenreLexeme::RIEN:
					os << "rien";
					break;
				case GenreLexeme::OCTET:
					os << "octet";
					break;
				case GenreLexeme::NUL:
					os << "nul";
					break;
				case GenreLexeme::REFERENCE:
				{
					os << "&";
					break;
				}
				case GenreLexeme::CHAINE_CARACTERE:
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
	GenreLexeme identifiant = donnees_type.type_base();

	switch (identifiant & 0xff) {
		case GenreLexeme::OCTET:
		case GenreLexeme::BOOL:
		case GenreLexeme::N8:
		case GenreLexeme::Z8:
			return 1;
		case GenreLexeme::R16:
		case GenreLexeme::N16:
		case GenreLexeme::Z16:
			return 2;
		case GenreLexeme::R32:
		case GenreLexeme::N32:
		case GenreLexeme::Z32:
			return 4;
		case GenreLexeme::TABLEAU:
		{
			if (size_t(identifiant >> 8) == 0) {
				return 8;
			}

			return alignement(contexte, donnees_type.dereference());
		}
		case GenreLexeme::FONC:
		case GenreLexeme::POINTEUR:
		case GenreLexeme::REFERENCE:
		case GenreLexeme::EINI:
		case GenreLexeme::R64:
		case GenreLexeme::N64:
		case GenreLexeme::Z64:
		case GenreLexeme::CHAINE:
			return 8;
		case GenreLexeme::CHAINE_CARACTERE:
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
		case GenreLexeme::OCTET:
		case GenreLexeme::BOOL:
		case GenreLexeme::N8:
		case GenreLexeme::Z8:
		{
			return 1;
		}
		case GenreLexeme::N16:
		case GenreLexeme::Z16:
		{
			return 2;
		}
		case GenreLexeme::R16:
		{
			return 2;
		}
		case GenreLexeme::N32:
		case GenreLexeme::Z32:
		case GenreLexeme::R32:
		{
			return 4;
		}
		case GenreLexeme::N64:
		case GenreLexeme::Z64:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case GenreLexeme::R64:
		{
			return 8;
		}
		case GenreLexeme::CHAINE_CARACTERE:
		{
			auto index_struct = static_cast<long>(type_base >> 8);
			auto &ds = contexte.donnees_structure(index_struct);

			if (ds.est_enum) {
				auto dt_enum = contexte.typeuse[ds.noeud_decl->index_type];
				return taille_octet_type(contexte, dt_enum);
			}

			return ds.taille_octet;
		}
		case GenreLexeme::POINTEUR:
		case GenreLexeme::FONC:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case GenreLexeme::TABLEAU:
		case GenreLexeme::EINI:
		case GenreLexeme::CHAINE:
		{
			if (contexte.bit32) {
				return 8;
			}

			return 16;
		}
		case GenreLexeme::RIEN:
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

	dt.pousse(GenreLexeme::POINTEUR);
	dt.pousse(GenreLexeme::CHAINE_CARACTERE | static_cast<int>(ds.id << 8));
}

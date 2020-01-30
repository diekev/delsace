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

TypeLexeme DonneesTypeDeclare::type_base() const
{
	if (donnees.est_vide()) {
		return TypeLexeme::INCONNU;
	}

	return donnees[0];
}

long DonneesTypeDeclare::taille() const
{
	return donnees.taille();
}

TypeLexeme DonneesTypeDeclare::operator[](long idx) const
{
	return donnees[idx];
}

void DonneesTypeDeclare::pousse(TypeLexeme id)
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

DonneesTypeFinal::DonneesTypeFinal(TypeLexeme i0)
{
	m_donnees.pousse(i0);
}

DonneesTypeFinal::DonneesTypeFinal(TypeLexeme i0, TypeLexeme i1)
{
	m_donnees.pousse(i0);
	m_donnees.pousse(i1);
}

DonneesTypeFinal::DonneesTypeFinal(type_plage_donnees_type autre)
{
	pousse(autre);
}

void DonneesTypeFinal::pousse(TypeLexeme identifiant)
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

TypeLexeme DonneesTypeFinal::type_base() const
{
	return m_donnees.front();
}

bool DonneesTypeFinal::est_invalide() const
{
	if (m_donnees.est_vide()) {
		return true;
	}

	switch (m_donnees.back()) {
		case TypeLexeme::POINTEUR:
		case TypeLexeme::REFERENCE:
		case TypeLexeme::TABLEAU:
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
				case TypeLexeme::TROIS_POINTS:
				{
					os << "...";
					break;
				}
				case TypeLexeme::POINTEUR:
					os << '*';
					break;
				case TypeLexeme::TABLEAU:
					os << '[';

					if (static_cast<size_t>(donnee >> 8) != 0) {
						os << static_cast<size_t>(donnee >> 8);
					}

					os << ']';
					break;
				case TypeLexeme::N8:
					os << "n8";
					break;
				case TypeLexeme::N16:
					os << "n16";
					break;
				case TypeLexeme::N32:
					os << "n32";
					break;
				case TypeLexeme::N64:
					os << "n64";
					break;
				case TypeLexeme::N128:
					os << "n128";
					break;
				case TypeLexeme::R16:
					os << "r16";
					break;
				case TypeLexeme::R32:
					os << "r32";
					break;
				case TypeLexeme::R64:
					os << "r64";
					break;
				case TypeLexeme::R128:
					os << "r128";
					break;
				case TypeLexeme::Z8:
					os << "z8";
					break;
				case TypeLexeme::Z16:
					os << "z16";
					break;
				case TypeLexeme::Z32:
					os << "z32";
					break;
				case TypeLexeme::Z64:
					os << "z64";
					break;
				case TypeLexeme::Z128:
					os << "z128";
					break;
				case TypeLexeme::BOOL:
					os << "bool";
					break;
				case TypeLexeme::CHAINE:
					os << "chaine";
					break;
				case TypeLexeme::FONC:
					os << "fonc";
					break;
				case TypeLexeme::COROUT:
					os << "corout";
					break;
				case TypeLexeme::PARENTHESE_OUVRANTE:
					os << '(';
					break;
				case TypeLexeme::PARENTHESE_FERMANTE:
					os << ')';
					break;
				case TypeLexeme::VIRGULE:
					os << ',';
					break;
				case TypeLexeme::EINI:
					os << "eini";
					break;
				case TypeLexeme::RIEN:
					os << "rien";
					break;
				case TypeLexeme::OCTET:
					os << "octet";
					break;
				case TypeLexeme::NUL:
					os << "nul";
					break;
				case TypeLexeme::REFERENCE:
				{
					os << "&";
					break;
				}
				case TypeLexeme::CHAINE_CARACTERE:
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
	TypeLexeme identifiant = donnees_type.type_base();

	switch (identifiant & 0xff) {
		case TypeLexeme::BOOL:
		case TypeLexeme::N8:
		case TypeLexeme::Z8:
			return 1;
		case TypeLexeme::R16:
		case TypeLexeme::N16:
		case TypeLexeme::Z16:
			return 2;
		case TypeLexeme::R32:
		case TypeLexeme::N32:
		case TypeLexeme::Z32:
			return 4;
		case TypeLexeme::TABLEAU:
		{
			if (size_t(identifiant >> 8) == 0) {
				return 8;
			}

			return alignement(contexte, donnees_type.dereference());
		}
		case TypeLexeme::FONC:
		case TypeLexeme::POINTEUR:
		case TypeLexeme::REFERENCE:
		case TypeLexeme::EINI:
		case TypeLexeme::R64:
		case TypeLexeme::N64:
		case TypeLexeme::Z64:
		case TypeLexeme::N128:
		case TypeLexeme::Z128:
		case TypeLexeme::R128:
		case TypeLexeme::CHAINE:
			return 8;
		case TypeLexeme::CHAINE_CARACTERE:
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
		case TypeLexeme::OCTET:
		case TypeLexeme::BOOL:
		case TypeLexeme::N8:
		case TypeLexeme::Z8:
		{
			return 1;
		}
		case TypeLexeme::N16:
		case TypeLexeme::Z16:
		{
			return 2;
		}
		case TypeLexeme::R16:
		{
			return 2;
		}
		case TypeLexeme::N32:
		case TypeLexeme::Z32:
		case TypeLexeme::R32:
		{
			return 4;
		}
		case TypeLexeme::N64:
		case TypeLexeme::Z64:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case TypeLexeme::R64:
		{
			return 8;
		}
		case TypeLexeme::N128:
		case TypeLexeme::Z128:
		case TypeLexeme::R128:
		{
			return 16;
		}
		case TypeLexeme::CHAINE_CARACTERE:
		{
			auto index_struct = static_cast<long>(type_base >> 8);
			auto &ds = contexte.donnees_structure(index_struct);

			if (ds.est_enum) {
				auto dt_enum = contexte.typeuse[ds.noeud_decl->index_type];
				return taille_octet_type(contexte, dt_enum);
			}

			return ds.taille_octet;
		}
		case TypeLexeme::POINTEUR:
		case TypeLexeme::FONC:
		{
			if (contexte.bit32) {
				return 4;
			}

			return 8;
		}
		case TypeLexeme::TABLEAU:
		case TypeLexeme::EINI:
		case TypeLexeme::CHAINE:
		{
			if (contexte.bit32) {
				return 8;
			}

			return 16;
		}
		case TypeLexeme::RIEN:
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

	dt.pousse(TypeLexeme::POINTEUR);
	dt.pousse(TypeLexeme::CHAINE_CARACTERE | static_cast<int>(ds.id << 8));
}

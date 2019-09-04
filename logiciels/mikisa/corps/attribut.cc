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

#include "attribut.h"

#include <cstring>

#include "corps.h"

/* ************************************************************************** */

static void supprime_liste(dls::tableau<char> *liste)
{
	memoire::deloge("attribut", liste);
}

long taille_octet_type_attribut(type_attribut type)
{
	switch (type) {
		case type_attribut::N8:
		case type_attribut::Z8:
			return static_cast<long>(sizeof(char));
		case type_attribut::N16:
		case type_attribut::Z16:
			return static_cast<long>(sizeof(short));
		case type_attribut::N32:
		case type_attribut::Z32:
			return static_cast<long>(sizeof(int));
		case type_attribut::N64:
		case type_attribut::Z64:
			return static_cast<long>(sizeof(long));
		case type_attribut::R16:
			return static_cast<long>(sizeof(r16));
		case type_attribut::R32:
			return static_cast<long>(sizeof(float));
		case type_attribut::R64:
			return static_cast<long>(sizeof(double));
		case type_attribut::CHAINE:
			return static_cast<long>(sizeof(dls::chaine));
		case type_attribut::INVALIDE:
			return 0l;
	}

	return 0l;
}

/* ************************************************************************** */

Attribut::Attribut(dls::chaine const &name, type_attribut type, int dims, portee_attr portee_, long taille)
	: m_nom(name)
	, m_type(type)
	, portee(portee_)
	, dimensions(dims)
{
	assert(taille >= 0);
	detache();
	m_tampon->redimensionne(taille * taille_octet_type_attribut(m_type) * dimensions);
}

Attribut::Attribut(Attribut const &rhs)
	: Attribut(rhs.nom(), rhs.type(), rhs.dimensions, rhs.portee, rhs.taille())
{
	std::copy(rhs.m_tampon->debut(), rhs.m_tampon->fin(), m_tampon->debut());
}

type_attribut Attribut::type() const
{
	return m_type;
}

dls::chaine Attribut::nom() const
{
	return m_nom;
}

void Attribut::nom(dls::chaine const &n)
{
	m_nom = n;
}

void Attribut::reserve(long n)
{
	assert(n >= 0);
	detache();
	m_tampon->reserve(n * taille_octet_type_attribut(m_type) * dimensions);
}

void Attribut::redimensionne(long n)
{
	assert(n >= 0);
	detache();
	m_tampon->redimensionne(n * taille_octet_type_attribut(m_type) * dimensions);
}

long Attribut::taille() const
{
	return m_tampon->taille() / (taille_octet_type_attribut(m_type) * dimensions);
}

void Attribut::detache()
{
	if (m_tampon == nullptr) {
		m_tampon = ptr_liste(memoire::loge<type_liste>("attribut"), supprime_liste);
		return;
	}

	if (!m_tampon.unique()) {
		m_tampon = ptr_liste(memoire::loge<type_liste>("attribut", *(m_tampon.get())), supprime_liste);
	}
}

void Attribut::reinitialise()
{
	m_tampon->efface();
}

const void *Attribut::donnees() const
{
	return m_tampon->donnees();
}

void *Attribut::donnees()
{
	return m_tampon->donnees();
}

long Attribut::taille_octets() const
{
	return m_tampon->taille();
}

/* ************************************************************************** */

void copie_attribut(Attribut const *attr_orig, long idx_orig, Attribut *attr_dest, long idx_dest)
{
	switch (attr_orig->type()) {
		default:
		{
			auto donnees_orig = static_cast<char const *>(attr_orig->donnees());
			auto donnees_dest = static_cast<char *>(attr_dest->donnees());

			auto octets_type = taille_octet_type_attribut(attr_orig->type()) * attr_orig->dimensions;

			auto ptr_orig = donnees_orig + (idx_orig * octets_type);
			auto ptr_dest = donnees_dest + (idx_dest * octets_type);

			std::memcpy(ptr_dest, ptr_orig, static_cast<size_t>(octets_type));
			break;
		}
		case type_attribut::CHAINE:
		{
			assigne(attr_dest->chaine(idx_dest), *attr_orig->chaine(idx_orig));
			break;
		}
		case type_attribut::INVALIDE:
		{
			break;
		}
	}
}

/* ************************************************************************** */

TransferanteAttribut::TransferanteAttribut(const Corps &corps_orig, Corps &corps_dest, int drapeaux)
{
	for (auto const &attr : corps_orig.attributs()) {
		switch (attr.portee) {
			case portee_attr::POINT:
			{
				if ((drapeaux & TRANSFERE_ATTR_POINTS) != 0) {
					auto nattr = corps_dest.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee);
					m_attr_points.pousse({ &attr, nattr });
				}

				break;
			}
			case portee_attr::PRIMITIVE:
			{
				if ((drapeaux & TRANSFERE_ATTR_PRIMS) != 0) {
					auto nattr = corps_dest.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee);
					m_attr_prims.pousse({ &attr, nattr });
				}

				break;
			}
			case portee_attr::VERTEX:
			{
				if ((drapeaux & TRANSFERE_ATTR_SOMMETS) != 0) {
					auto nattr = corps_dest.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee);
					m_attr_sommets.pousse({ &attr, nattr });
				}

				break;
			}
			case portee_attr::CORPS:
			{
				if ((drapeaux & TRANSFERE_ATTR_CORPS) != 0) {
					auto nattr = corps_dest.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee);
					m_attr_corps.pousse({ &attr, nattr });
				}

				break;
			}
			case portee_attr::GROUPE:
			{
				if ((drapeaux & TRANSFERE_ATTR_GROUPES) != 0) {
					auto nattr = corps_dest.ajoute_attribut(attr.nom(), attr.type(), attr.dimensions, attr.portee);
					m_attr_groupes.pousse({ &attr, nattr });
				}

				break;
			}
		}
	}
}

void TransferanteAttribut::transfere_attributs_points(long idx_orig, long idx_dest)
{
	for (auto &paire : m_attr_points) {
		copie_attribut(paire.first, idx_orig, paire.second, idx_dest);
	}
}

void TransferanteAttribut::transfere_attributs_prims(long idx_orig, long idx_dest)
{
	for (auto &paire : m_attr_prims) {
		copie_attribut(paire.first, idx_orig, paire.second, idx_dest);
	}
}

void TransferanteAttribut::transfere_attributs_sommets(long idx_orig, long idx_dest)
{
	for (auto &paire : m_attr_sommets) {
		copie_attribut(paire.first, idx_orig, paire.second, idx_dest);
	}
}

void TransferanteAttribut::transfere_attributs_corps(long idx_orig, long idx_dest)
{
	for (auto &paire : m_attr_corps) {
		copie_attribut(paire.first, idx_orig, paire.second, idx_dest);
	}
}

void TransferanteAttribut::transfere_attributs_groupes(long idx_orig, long idx_dest)
{
	for (auto &paire : m_attr_groupes) {
		copie_attribut(paire.first, idx_orig, paire.second, idx_dest);
	}
}

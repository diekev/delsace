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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "constructrice_code_c.hh"

Enchaineuse::Enchaineuse()
	: tampon_courant(&m_tampon_base)
{}

Enchaineuse::~Enchaineuse()
{
	auto tampon = m_tampon_base.suivant;

	while (tampon != nullptr) {
		auto tmp = tampon;
		tampon = tampon->suivant;

		memoire::deloge("Tampon", tmp);
	}
}

void Enchaineuse::pousse(const dls::vue_chaine &chn)
{
	pousse(&chn[0], chn.taille());
}

void Enchaineuse::pousse(const char *c_str, long N)
{
	auto tampon = tampon_courant;

	if (tampon->occupe + N < TAILLE_TAMPON) {
		memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(N));
		tampon->occupe += static_cast<int>(N);
	}
	else {
		auto taille_a_ecrire = N;
		auto taille_max = TAILLE_TAMPON - tampon->occupe;

		if (taille_max != 0) {
			memcpy(&tampon->donnees[tampon->occupe], c_str, static_cast<size_t>(taille_max));
			tampon->occupe += taille_max;
			taille_a_ecrire -= taille_max;
		}

		ajoute_tampon();
		tampon = tampon_courant;

		memcpy(&tampon->donnees[0], c_str + taille_max, static_cast<size_t>(taille_a_ecrire));
		tampon->occupe += static_cast<int>(taille_a_ecrire);
	}
}

void Enchaineuse::pousse_caractere(char c)
{
	auto tampon = tampon_courant;

	if (tampon->occupe == TAILLE_TAMPON) {
		ajoute_tampon();
		tampon = tampon_courant;
	}

	tampon->donnees[tampon->occupe++] = c;
}

void Enchaineuse::ajoute_tampon()
{
	auto tampon = memoire::loge<Tampon>("Tampon");
	tampon_courant->suivant = tampon;
	tampon_courant = tampon;
}

ConstructriceCodeC &operator <<(ConstructriceCodeC &constructrice, const dls::vue_chaine_compacte &chn)
{
	constructrice.m_enchaineuse.pousse(chn.pointeur(), chn.taille());
	return constructrice;
}

ConstructriceCodeC &operator <<(ConstructriceCodeC &constructrice, const dls::chaine &chn)
{
	constructrice.m_enchaineuse.pousse(chn.c_str(), chn.taille());
	return constructrice;
}

ConstructriceCodeC &operator << (ConstructriceCodeC &constructrice, const char *chn)
{
	auto ptr = chn;

	while (*chn != '\0') {
		++chn;
	}

	constructrice.m_enchaineuse.pousse(ptr, chn - ptr);

	return constructrice;
}

void ConstructriceCodeC::imprime_dans_flux(std::ostream &flux)
{
	auto tampon = &m_enchaineuse.m_tampon_base;

	while (tampon != nullptr) {
		flux.write(&tampon->donnees[0], tampon->occupe);
		tampon = tampon->suivant;
	}
}

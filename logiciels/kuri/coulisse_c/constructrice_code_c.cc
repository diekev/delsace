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

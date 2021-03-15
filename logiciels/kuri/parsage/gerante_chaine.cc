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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "gerante_chaine.hh"

long GeranteChaine::ajoute_chaine(const kuri::chaine &chaine)
{
	return ajoute_chaine(kuri::chaine_statique(chaine));
}

long GeranteChaine::ajoute_chaine(kuri::chaine_statique chaine)
{
	if ((enchaineuse.tampon_courant->occupe + chaine.taille()) >= Enchaineuse::TAILLE_TAMPON) {
		enchaineuse.ajoute_tampon();
	}

	// calcul l'adresse de la chaine
	auto adresse = (enchaineuse.nombre_tampons() - 1) * Enchaineuse::TAILLE_TAMPON + enchaineuse.tampon_courant->occupe;

	enchaineuse.ajoute(chaine);

	return adresse | (chaine.taille() << 32);
}

kuri::chaine_statique GeranteChaine::chaine_pour_adresse(long adresse) const
{
	assert(adresse >= 0);

	auto taille = (adresse >> 32) & 0xffffffff;
	adresse = (adresse & 0xfffffff);

	auto tampon_courant = &enchaineuse.m_tampon_base;

	while (adresse >= Enchaineuse::TAILLE_TAMPON) {
		adresse -= Enchaineuse::TAILLE_TAMPON;
		tampon_courant = tampon_courant->suivant;
	}

	assert(tampon_courant);
	return { &tampon_courant->donnees[adresse], taille };
}

long GeranteChaine::memoire_utilisee() const
{
	return enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;
}

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

#include "magasin_chaine.h"

#include "autrice_fichier.h"
#include "lectrice_fichier.h"

namespace arachne {

unsigned int magasin_chaine::ajoute_chaine(const std::string &chaine)
{
	auto index = index_chaine(chaine);

	if (index == 0) {
		m_tableau.insere({chaine, ++m_nombre_chaines});
		m_taille_chaines += chaine.size();
		return m_nombre_chaines;
	}

	return index;
}

unsigned int magasin_chaine::index_chaine(const std::string &chaine) const
{
	auto iter = m_tableau.trouve(chaine);

	if (iter == m_tableau.fin()) {
		return 0;
	}

	return static_cast<unsigned>((*iter).second);
}

bool magasin_chaine::est_valide() const
{
	return m_nombre_chaines == m_tableau.taille();
}

size_t magasin_chaine::taille() const
{
	return m_nombre_chaines;
}

size_t magasin_chaine::taille_chaines() const
{
	return m_taille_chaines;
}

magasin_chaine::iterateur magasin_chaine::debut()
{
	return m_tableau.debut();
}

magasin_chaine::iterateur magasin_chaine::fin()
{
	return m_tableau.fin();
}

magasin_chaine::iterateur magasin_chaine::begin()
{
	return m_tableau.debut();
}

magasin_chaine::iterateur magasin_chaine::end()
{
	return m_tableau.fin();
}

magasin_chaine::iterateur_const magasin_chaine::begin() const
{
	return m_tableau.debut();
}

magasin_chaine::iterateur_const magasin_chaine::end() const
{
	return m_tableau.fin();
}

void ecris_magasin_chaine(const magasin_chaine &magasin, const std::experimental::filesystem::path &chemin)
{
	autrice_fichier autrice;
	autrice.ouvre(chemin);

	if (!autrice.est_ouverte()) {
		std::fprintf(stderr, "Le fichier de magasin n'est pas ouvert !");
		return;
	}

	std::fprintf(stderr, "Écriture de %lu d'entrées du magasin dans le fichier.\n", magasin.taille());

	std::vector<char> tampon;
	tampon.resize(magasin.taille() * 12 + magasin.taille_chaines());

	auto decalage = 0ul;

	for (const auto &pair : magasin) {
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 0]) = static_cast<unsigned>(pair.second);
		*reinterpret_cast<size_t *>(&tampon[decalage + 4]) = pair.first.size();

		auto pointeur = &tampon[decalage + 12];

		for (size_t i = 0; i < pair.first.size(); ++i) {
			*pointeur++ = pair.first[i];
		}

		decalage += (12 + pair.first.size());
	}

	autrice.ecrit_tampon(tampon.data(), tampon.size());

	autrice.ferme();
}

void lis_magasin_chaine(const std::experimental::filesystem::path &chemin)
{
	lectrice_fichier lectrice;
	lectrice.ouvre(chemin);

	if (!lectrice.est_ouverte()) {
		return;
	}

	std::fprintf(stderr, "------------------------------------\n");
	std::fprintf(stderr, "Lecture fichier '%s'\n", chemin.c_str());

	auto taille_fichier = lectrice.taille();
	std::string tampon_chaine;

	std::fprintf(stderr, "Taille fichier : %lu\n", taille_fichier);

	while (taille_fichier != 0) {
		std::fprintf(stderr, "------------------------------------\n");
		/* Lis l'index et la taille de la chaine */
		char tampon[12];

		lectrice.lis_tampon(tampon, 12);

		auto index = *reinterpret_cast<unsigned int *>(&tampon[0]);
		auto taille = *reinterpret_cast<size_t *>(&tampon[4]);

		std::fprintf(stderr, "Index : %u, taille %lu\n", index, taille);

		tampon_chaine.resize(taille);

		lectrice.lis_tampon(&tampon_chaine[0], taille);

		std::fprintf(stderr, "%s\n", tampon_chaine.c_str());

		taille_fichier -= (12 + taille);
	}

	lectrice.ferme();
}

}  /* namespace arachne */

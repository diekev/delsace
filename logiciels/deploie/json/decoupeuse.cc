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

#include "decoupeuse.hh"

#include <iostream>
#include <cstring>

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "erreur.hh"

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(lng::tampon_source const &tampon)
	: m_tampon(tampon)
	, m_debut_mot(m_tampon.debut())
	, m_debut(m_tampon.debut())
	, m_fin(m_tampon.fin())
{
	construit_tables_caractere_speciaux();
}

void decoupeuse_texte::genere_morceaux()
{
	auto id_special = id_morceau{};

	while (!this->fini()) {
		if (est_caractere_special(this->caractere_courant(), id_special)) {
			this->enregistre_position_mot();
			this->pousse_caractere();
			this->pousse_mot(id_special);
			this->avance();
		}
		else if (lng::est_espace_blanc(this->caractere_courant())) {
			this->avance();
		}
		else if (this->caractere_courant() == '"') {
			this->avance();

			this->enregistre_position_mot();

			auto dernier_caractere = this->caractere_courant();

			while (true) {
				if (this->caractere_courant() == '"' && dernier_caractere != '\\') {
					break;
				}

				dernier_caractere = this->caractere_courant();

				this->pousse_caractere();
				this->avance();
			}

			this->pousse_mot(id_morceau::CHAINE_CARACTERE);
			this->avance();
		}
		else if (lng::est_nombre_decimal(this->caractere_courant())) {
			this->enregistre_position_mot();

			using denombreuse = lng::decoupeuse_nombre<id_morceau>;
			id_morceau id_nombre;

			/* NOTE : on utilise une variable temporaire pour stocker le compte au
			 * lieu d'utiliser m_taille_mot_courant directement, car
			 * m_taille_mot_courant est remis à 0 dans pousse_mot() donc on ne peut
			 * l'utiliser en paramètre de avance() (ce qui causerait une boucle
			 * infinie. */
			auto const compte = denombreuse::extrait_nombre(m_debut, m_fin, id_nombre);
			m_taille_mot_courant = static_cast<long>(compte);

			this->pousse_mot(id_nombre);
			this->avance(static_cast<int>(compte));
		}
		else {
			this->avance();
		}
	}
}

size_t decoupeuse_texte::memoire_morceaux() const
{
	return static_cast<size_t>(m_morceaux.taille()) * sizeof(DonneesMorceau);
}

dls::tableau<DonneesMorceau> &decoupeuse_texte::morceaux()
{
	return m_morceaux;
}

decoupeuse_texte::iterateur decoupeuse_texte::begin()
{
	return m_morceaux.debut();
}

decoupeuse_texte::iterateur decoupeuse_texte::end()
{
	return m_morceaux.fin();
}

bool decoupeuse_texte::fini() const
{
	return m_debut >= m_fin;
}

void decoupeuse_texte::avance(int n)
{
	for (int i = 0; i < n; ++i) {
		if (this->caractere_courant() == '\n') {
			++m_compte_ligne;
			m_position_ligne = 0;
		}
		else {
			++m_position_ligne;
		}

		++m_debut;
	}
}

char decoupeuse_texte::caractere_courant() const
{
	return *m_debut;
}

char decoupeuse_texte::caractere_voisin(int n) const
{
	return *(m_debut + n);
}

dls::vue_chaine decoupeuse_texte::mot_courant() const
{
	return dls::vue_chaine(m_debut_mot, m_taille_mot_courant);
}

void decoupeuse_texte::lance_erreur(const dls::chaine &quoi) const
{
	auto ligne_courante = m_tampon[m_compte_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0l; i < m_position_ligne; ++i) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}
	}

	ss << "^~~~\n";
	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), erreur::DECOUPAGE);
}

void decoupeuse_texte::pousse_caractere()
{
	m_taille_mot_courant += 1;
}

void decoupeuse_texte::pousse_mot(id_morceau identifiant)
{
	m_morceaux.pousse({ mot_courant(), static_cast<size_t>((m_compte_ligne << 32) | m_pos_mot), identifiant });
	m_taille_mot_courant = 0;
	m_debut_mot = nullptr;
}

void decoupeuse_texte::enregistre_position_mot()
{
	m_pos_mot = m_position_ligne;
	m_debut_mot = m_debut;
}

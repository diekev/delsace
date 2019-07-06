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
#include <sstream>

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/conditions.h"

#include "erreur.hh"

/* ************************************************************************** */

static bool est_caractere_blanc(char c)
{
	return dls::outils::est_element(c, '\t', '\r', '\n', ' ');
}

/* ************************************************************************** */

enum {
	ETAT_SCRIPT,
	ETAT_HTML,
};

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
	auto etat = ETAT_HTML;

	while (!this->fini()) {
		if (this->caractere_courant() == '{') {
			if (this->caractere_voisin() == '{') {
				if (m_taille_mot_courant != 0) {
					pousse_mot(id_chaine(mot_courant()));
				}

				etat = ETAT_SCRIPT;

				this->enregistre_position_mot();
				this->pousse_caractere();
				this->pousse_caractere();
				pousse_mot(ID_DEBUT_VARIABLE);
				avance(2);
			}
			else if (this->caractere_voisin() == '%') {
				if (m_taille_mot_courant != 0) {
					pousse_mot(id_chaine(mot_courant()));
				}

				etat = ETAT_SCRIPT;

				this->enregistre_position_mot();
				this->pousse_caractere();
				this->pousse_caractere();
				pousse_mot(ID_DEBUT_EXPRESSION);
				avance(2);
			}
		}
		else if (this->caractere_courant() == '}' && this->caractere_voisin() == '}') {
			if (m_taille_mot_courant != 0) {
				pousse_mot(id_chaine(mot_courant()));
			}

			etat = ETAT_HTML;

			this->enregistre_position_mot();
			this->pousse_caractere();
			this->pousse_caractere();
			pousse_mot(ID_FIN_VARIABLE);
			avance(2);
		}
		else if (this->caractere_courant() == '%' && this->caractere_voisin() == '}') {
			if (m_taille_mot_courant != 0) {
				pousse_mot(id_chaine(mot_courant()));
			}

			etat = ETAT_HTML;

			this->enregistre_position_mot();
			this->pousse_caractere();
			this->pousse_caractere();
			pousse_mot(ID_FIN_EXPRESSION);
			avance(2);
		}
		else {
			if (etat == ETAT_HTML) {
				if (m_taille_mot_courant == 0) {
					enregistre_position_mot();
				}

				pousse_caractere();
			}
			else {
				analyse_caractere_simple();
			}

			avance();
		}
	}

	if (m_taille_mot_courant != 0) {
		pousse_mot(ID_CHAINE_CARACTERE);
	}
}

size_t decoupeuse_texte::memoire_morceaux() const
{
	return static_cast<size_t>(m_morceaux.taille()) * sizeof(DonneesMorceaux);
}

dls::tableau<DonneesMorceaux> &decoupeuse_texte::morceaux()
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

std::string_view decoupeuse_texte::mot_courant() const
{
	return std::string_view(m_debut_mot, m_taille_mot_courant);
}

void decoupeuse_texte::lance_erreur(const std::string &quoi) const
{
	auto ligne_courante = m_tampon[m_compte_ligne];

	std::stringstream ss;
	ss << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (size_t i = 0; i < m_position_ligne; ++i) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}
	}

	ss << "^~~~\n";
	ss << quoi;

	throw erreur::frappe(ss.str().c_str(), erreur::DECOUPAGE);
}

void decoupeuse_texte::analyse_caractere_simple()
{
	if (est_caractere_blanc(this->caractere_courant())) {
		if (m_taille_mot_courant != 0) {
			pousse_mot(id_chaine(mot_courant()));
		}
	}
	else {
		if (m_taille_mot_courant == 0) {
			enregistre_position_mot();
		}

		pousse_caractere();
	}
}

void decoupeuse_texte::pousse_caractere()
{
	m_taille_mot_courant += 1;
}

void decoupeuse_texte::pousse_mot(int identifiant)
{
	m_morceaux.pousse({ mot_courant(), ((m_compte_ligne << 32) | m_pos_mot), static_cast<size_t>(identifiant) });
	m_taille_mot_courant = 0;
	m_debut_mot = nullptr;
}

void decoupeuse_texte::enregistre_position_mot()
{
	m_pos_mot = m_position_ligne;
	m_debut_mot = m_debut;
}

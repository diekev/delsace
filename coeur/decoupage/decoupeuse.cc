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

#include "decoupeuse.h"

#include <iostream>
#include <cstring>
#include <sstream>

#include "erreur.h"
#include "nombres.h"
#include "unicode.h"

/* ************************************************************************** */

bool est_espace_blanc(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(const TamponSource &tampon)
	: m_tampon(tampon)
	, m_debut_mot(m_tampon.debut())
	, m_debut(m_tampon.debut())
	, m_fin(m_tampon.fin())
{}

void decoupeuse_texte::genere_morceaux()
{
	m_taille_mot_courant = 0;

	while (!this->fini()) {
		const auto nombre_octet = nombre_octets(m_debut);

		if (nombre_octet == 1) {
			analyse_caractere_simple();
		}
		else if (nombre_octet >= 2 && nombre_octet <= 4) {
			/* Les caractères spéciaux ne peuvent être des caractères unicode
			 * pour le moment, donc on les copie directement dans le tampon du
			 * mot_courant. */
			if (m_taille_mot_courant == 0) {
				this->enregistre_pos_mot();
			}

			m_taille_mot_courant += static_cast<size_t>(nombre_octet);
			this->avance(nombre_octet);
		}
		else {
			/* Le caractère (octet) courant est invalide dans le codec unicode. */
			lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
		}
	}

	if (m_taille_mot_courant != 0) {
		lance_erreur("Des caractères en trop se trouve à la fin du texte !");
	}
}

const std::vector<DonneesMorceaux> &decoupeuse_texte::morceaux() const
{
	return m_morceaux;
}

decoupeuse_texte::iterateur decoupeuse_texte::begin()
{
	return m_morceaux.begin();
}

decoupeuse_texte::iterateur decoupeuse_texte::end()
{
	return m_morceaux.end();
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
	for (size_t i = 0; i < m_position_ligne; i += static_cast<size_t>(nombre_octets(&ligne_courante[i]))) {
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

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe trois point
//        sinon si caractere est '"':
//            decoupe chaine caractere littérale
//        sinon si caractere est '#':
//            decoupe commentaire
//        sinon si caractere est '\'':
//            decoupe caractere
//        sinon:
//        	ajoute mot caractere simple
// sinon si nombre et mot est vide:
//    decoupe nombre
// sinon:
//    ajoute caractere mot courant
void decoupeuse_texte::analyse_caractere_simple()
{
	int idc = ID_INCONNU;

	if (est_espace_blanc(this->caractere_courant())) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		this->avance();
	}
	else if (est_caractere_special(this->caractere_courant(), idc)) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		this->enregistre_pos_mot();

		auto id = id_caractere_double(std::string_view(m_debut, 2));

		if (id != ID_INCONNU) {
			this->pousse_caractere();
			this->pousse_caractere();
			this->pousse_mot(id);
			this->avance(2);
			return;
		}

		if (this->caractere_courant() == '.') {
			if (this->caractere_voisin() != '.') {
				lance_erreur("Point inattendu !\n");
			}

			if (this->caractere_voisin(2) != '.') {
				lance_erreur("Un point est manquant ou un point est en trop !\n");
			}

			this->pousse_caractere();
			this->pousse_caractere();
			this->pousse_caractere();

			this->pousse_mot(ID_TROIS_POINTS);
			this->avance(3);
		}
		else if (this->caractere_courant() == '"') {
			// Saute le premier guillemet.
			this->avance();
			this->enregistre_pos_mot();

			while (!this->fini()) {
				if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
					break;
				}

				this->pousse_caractere();
				this->avance();
			}

			// Saute le dernier guillemet.
			this->avance();

			this->pousse_mot(ID_CHAINE_LITTERALE);
		}
		else if (this->caractere_courant() == '\'') {
			// Saute la première apostrophe.
			this->avance();

			this->enregistre_pos_mot();

			if (this->caractere_courant() == '\\') {
				this->pousse_caractere();
				this->avance();
			}

			this->pousse_caractere();
			this->pousse_mot(ID_CARACTERE);

			this->avance();

			// Saute la dernière apostrophe.
			if (this->caractere_courant() != '\'') {
				lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
			}

			this->avance();
		}
		else if (this->caractere_courant() == '#') {
			// ignore commentaire
			while (this->caractere_courant() != '\n') {
				this->avance();
			}
		}
		else {
			this->pousse_caractere();
			this->pousse_mot(idc);
			this->avance();
		}
	}
	else if (est_nombre_decimal(this->caractere_courant()) && m_taille_mot_courant == 0) {
		this->enregistre_pos_mot();

		int id_nombre;
		std::string nombre;
		const auto compte = extrait_nombre(m_debut, m_fin, nombre, id_nombre);

		m_taille_mot_courant = static_cast<size_t>(compte);

		/* À FAIRE : reconsidération de la manière de découper les nombres. */
		if (id_nombre != ID_NOMBRE_ENTIER && id_nombre != ID_NOMBRE_REEL) {
			m_pos_mot += 2;
			m_debut_mot += 2;
			m_taille_mot_courant -= 2;
		}

		this->pousse_mot(id_nombre);
		this->avance(compte);
	}
	else {
		if (m_taille_mot_courant == 0) {
			this->enregistre_pos_mot();
		}

		this->pousse_caractere();
		this->avance();
	}
}

void decoupeuse_texte::pousse_caractere()
{
	m_taille_mot_courant += 1;
}

void decoupeuse_texte::pousse_mot(int identifiant)
{
	m_morceaux.push_back({ mot_courant(), m_compte_ligne, m_pos_mot, identifiant });
	m_taille_mot_courant = 0;
}

void decoupeuse_texte::enregistre_pos_mot()
{
	m_pos_mot = m_position_ligne;
	m_debut_mot = m_debut;
}

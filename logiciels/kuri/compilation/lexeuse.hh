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

#pragma once

#include "lexemes.hh"
#include "structures.hh"

struct Compilatrice;
struct Fichier;

enum {
	INCLUS_CARACTERES_BLANC = (1 << 0),
	INCLUS_COMMENTAIRES     = (1 << 1),
};

class Lexeuse {
	Compilatrice &m_compilatrice;
	Fichier *m_fichier;
	const char *m_debut_mot = nullptr;
	const char *m_debut = nullptr;
	const char *m_fin = nullptr;

	long m_position_ligne = 0;
	long m_compte_ligne = 0;
	long m_pos_mot = 0;
	long m_taille_mot_courant = 0;

	int m_drapeaux = 0;
	GenreLexeme m_dernier_id = GenreLexeme::INCONNU;

public:
	Lexeuse(Compilatrice &compilatrice, Fichier *fichier, int drapeaux = 0);

	void performe_lexage();

	/**
	 * Retourne la taille en octets de la mémoire utilisée par les lexemes.
	 */
	size_t memoire_lexemes() const;

	void imprime_lexemes(std::ostream &os);

private:
	bool fini() const;

	void avance(int n = 1);

	char caractere_courant() const;

	char caractere_voisin(int n = 1) const;

	dls::vue_chaine_compacte mot_courant() const;

	[[noreturn]] void lance_erreur(const dls::chaine &quoi) const;

	void analyse_caractere_simple();

	void pousse_caractere(int n = 1);

	void pousse_mot(GenreLexeme identifiant);
	void pousse_mot(GenreLexeme identifiant, unsigned valeur);
	void pousse_mot(GenreLexeme identifiant, kuri::chaine valeur);

	void enregistre_pos_mot();

	void lexe_commentaire();

	void lexe_commentaire_bloc();

	void lexe_nombre();
	void lexe_nombre_decimal();
	void lexe_nombre_hexadecimal();
	void lexe_nombre_binaire();
	void lexe_nombre_octal();

	unsigned lexe_caractere_litteral(kuri::chaine *chaine);

	void pousse_lexeme_entier(unsigned long long valeur);
	void pousse_lexeme_reel(double valeur);
};

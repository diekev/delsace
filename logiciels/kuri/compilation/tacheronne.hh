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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/file.hh"

#include "unite_compilation.hh"

#include "../representation_intermediaire/constructrice_ri.hh"

struct Compilatrice;

#define ENUMERE_GENRES_TACHE \
	ENUMERE_GENRE_TACHE_EX(DORS) \
	ENUMERE_GENRE_TACHE_EX(COMPILATION_TERMINEE) \
	ENUMERE_GENRE_TACHE_EX(LEXE) \
	ENUMERE_GENRE_TACHE_EX(PARSE) \
	ENUMERE_GENRE_TACHE_EX(TYPAGE_ENTETE_FONCTION) \
	ENUMERE_GENRE_TACHE_EX(TYPAGE) \
	ENUMERE_GENRE_TACHE_EX(GENERE_RI) \
	ENUMERE_GENRE_TACHE_EX(EXECUTE) \
	ENUMERE_GENRE_TACHE_EX(LIAISON_EXECUTABLE) \
	ENUMERE_GENRE_TACHE_EX(GENERE_FICHIER_OBJET)

enum class GenreTache {
#define ENUMERE_GENRE_TACHE_EX(etat) etat,
		ENUMERE_GENRES_TACHE
#undef ENUMERE_GENRE_TACHE_EX
};

const char *chaine_genre_tache(GenreTache genre);

std::ostream &operator<<(std::ostream &os, GenreTache genre);

struct Tache {
	GenreTache genre = GenreTache::DORS;
	UniteCompilation *unite = nullptr;

	static Tache dors();

	static Tache compilation_terminee();

	static Tache genere_fichier_objet(UniteCompilation *unite_);

	static Tache liaison_objet(UniteCompilation *unite_);
};

struct OrdonnanceuseTache {
private:
	Compilatrice *m_compilatrice = nullptr;

	dls::file<Tache> taches_lexage{};
	dls::file<Tache> taches_parsage{};
	dls::file<Tache> taches_typage{};
	dls::file<Tache> taches_generation_ri{};
	dls::file<Tache> taches_execution{};

	tableau_page<UniteCompilation> unites{};

	/* utilsé pour définir ce que fait chaque tâcheronne afin de savoir si tout le
	 * monde fait quelque : si tout le monde dors, il n'y a plus rien à faire donc
	 * la compilation est terminée */
	dls::tablet<GenreTache, 16> etats_tacheronnes{};

	int nombre_de_taches_en_proces = 0;
	bool compilation_terminee = false;

public:
	OrdonnanceuseTache() = default;
	OrdonnanceuseTache(Compilatrice *compilatrice);

	OrdonnanceuseTache(OrdonnanceuseTache const &) = delete;
	OrdonnanceuseTache &operator=(OrdonnanceuseTache const &) = delete;

	void cree_tache_pour_lexage(EspaceDeTravail *espace, Fichier *fichier);
	void cree_tache_pour_parsage(EspaceDeTravail *espace, Fichier *fichier);
	void cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud);
	void cree_tache_pour_typage_fonction(EspaceDeTravail *espace, NoeudDeclarationFonction *noeud);
	void cree_tache_pour_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud);
	void cree_tache_pour_execution(EspaceDeTravail *espace, NoeudDirectiveExecution *noeud);

	Tache tache_suivante(Tache const &tache_terminee, bool tache_completee, int id, bool premiere);
	Tache tache_metaprogramme_suivante(Tache const &tache_terminee, int id, bool premiere);

	long memoire_utilisee() const;

private:
	void cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud, GenreTache genre_tache);

	void renseigne_etat_tacheronne(int id, GenreTache genre_tache);

	bool toutes_les_tacheronnes_dorment() const;
};

struct Tacheronne {
	Compilatrice &compilatrice;

	ConstructriceRI constructrice_ri{compilatrice};

	double temps_validation = 0.0;
	double temps_lexage = 0.0;
	double temps_parsage = 0.0;	
	double temps_executable = 0.0;
	double temps_fichier_objet = 0.0;
	double temps_scene = 0.0;
	double temps_generation_code = 0.0;

	int id = 0;

	Tacheronne(Compilatrice &comp);

	void gere_tache();
	void gere_tache_metaprogramme();

private:
	bool gere_unite_pour_typage(UniteCompilation *unite);
	bool gere_unite_pour_ri(UniteCompilation *unite);
	void gere_unite_pour_execution(UniteCompilation *unite);
};

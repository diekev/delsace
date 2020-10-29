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

#include "biblinternes/outils/badge.hh"
#include "biblinternes/structures/file.hh"

#include "allocatrice_noeud.hh"
#include "statistiques.hh"
#include "unite_compilation.hh"

#include "../representation_intermediaire/constructrice_ri.hh"
#include "../representation_intermediaire/machine_virtuelle.hh"

struct Compilatrice;
struct MetaProgramme;
struct Tacheronne;

#define ENUMERE_GENRES_TACHE \
	ENUMERE_GENRE_TACHE_EX(DORS) \
	ENUMERE_GENRE_TACHE_EX(COMPILATION_TERMINEE) \
	ENUMERE_GENRE_TACHE_EX(LEXE) \
	ENUMERE_GENRE_TACHE_EX(PARSE) \
	ENUMERE_GENRE_TACHE_EX(TYPAGE) \
	ENUMERE_GENRE_TACHE_EX(GENERE_RI) \
	ENUMERE_GENRE_TACHE_EX(EXECUTE) \
	ENUMERE_GENRE_TACHE_EX(LIAISON_EXECUTABLE) \
	ENUMERE_GENRE_TACHE_EX(GENERE_FICHIER_OBJET) \
	ENUMERE_GENRE_TACHE_EX(ENVOIE_MESSAGE)

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
	EspaceDeTravail *espace = nullptr;

	static Tache dors(EspaceDeTravail *espace_);

	static Tache compilation_terminee();

	static Tache genere_fichier_objet(EspaceDeTravail *espace_);

	static Tache liaison_objet(EspaceDeTravail *espace_);

	static Tache attend_message(UniteCompilation *unite_);
};

enum class DrapeauxTacheronne : uint32_t {
	PEUT_LEXER           = (1 << 0),
	PEUT_PARSER          = (1 << 1),
	PEUT_TYPER           = (1 << 2),
	PEUT_EXECUTER        = (1 << 3),
	PEUT_GENERER_CODE    = (1 << 4),
	PEUT_ENVOYER_MESSAGE = (1 << 5),
	PEUT_GENERER_RI      = (1 << 6),

	PEUT_TOUT_FAIRE      = 0xfffffff,
};

DEFINIE_OPERATEURS_DRAPEAU(DrapeauxTacheronne, unsigned int)

struct OrdonnanceuseTache {
private:
	Compilatrice *m_compilatrice = nullptr;

	dls::file<Tache> taches_lexage{};
	dls::file<Tache> taches_parsage{};
	dls::file<Tache> taches_typage{};
	dls::file<Tache> taches_generation_ri{};
	dls::file<Tache> taches_execution{};
	dls::file<Tache> taches_message{};

	tableau_page<UniteCompilation> unites{};

	/* utilsé pour définir ce que fait chaque tâcheronne afin de savoir si tout le
	 * monde fait quelque : si tout le monde dors, il n'y a plus rien à faire donc
	 * la compilation est terminée */
	dls::tablet<GenreTache, 16> etats_tacheronnes{};

	int nombre_de_tacheronnes = 0;
	bool compilation_terminee = false;

public:
	OrdonnanceuseTache() = default;
	OrdonnanceuseTache(Compilatrice *compilatrice);

	OrdonnanceuseTache(OrdonnanceuseTache const &) = delete;
	OrdonnanceuseTache &operator=(OrdonnanceuseTache const &) = delete;

	void cree_tache_pour_lexage(EspaceDeTravail *espace, Fichier *fichier);
	void cree_tache_pour_parsage(EspaceDeTravail *espace, Fichier *fichier);
	void cree_tache_pour_typage(EspaceDeTravail *espace, NoeudExpression *noeud);
	void cree_tache_pour_generation_ri(EspaceDeTravail *espace, NoeudExpression *noeud);
	void cree_tache_pour_execution(EspaceDeTravail *espace, MetaProgramme *metaprogramme);

	Tache tache_suivante(Tache &tache_terminee, bool tache_completee, int id, DrapeauxTacheronne drapeaux);

	long memoire_utilisee() const;

	int enregistre_tacheronne(Badge<Tacheronne> badge);

private:
	void renseigne_etat_tacheronne(int id, GenreTache genre_tache);

	bool toutes_les_tacheronnes_dorment() const;

	long nombre_de_taches_en_attente() const;

	Tache tache_suivante(EspaceDeTravail *espace, int id, DrapeauxTacheronne drapeaux);
};

struct Tacheronne {
	Compilatrice &compilatrice;

	ConstructriceRI constructrice_ri{compilatrice};
	MachineVirtuelle mv{compilatrice};

	AllocatriceNoeud allocatrice_noeud{};
	AssembleuseArbre *assembleuse = nullptr;

	StatistiquesTypage stats_typage{};

	double temps_validation = 0.0;
	double temps_lexage = 0.0;
	double temps_parsage = 0.0;	
	double temps_executable = 0.0;
	double temps_fichier_objet = 0.0;
	double temps_scene = 0.0;
	double temps_generation_code = 0.0;

	DrapeauxTacheronne drapeaux = DrapeauxTacheronne::PEUT_TOUT_FAIRE;

	int id = 0;
	double temps_passe_a_dormir = 0.0;

	Tacheronne(Compilatrice &comp);

	~Tacheronne();

	COPIE_CONSTRUCT(Tacheronne);

	void gere_tache();

private:
	bool gere_unite_pour_typage(UniteCompilation *unite);
	bool gere_unite_pour_ri(UniteCompilation *unite);
	bool gere_unite_pour_execution(UniteCompilation *unite);
};

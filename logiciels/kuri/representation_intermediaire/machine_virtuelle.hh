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

#include "code_binaire.hh"

#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

struct AtomeFonction;
struct Compilatrice;
struct MetaProgramme;
struct Statistiques;
struct TypeFonction;

struct GestionnaireBibliotheques {
	struct BibliothequePartagee {
		dls::systeme_fichier::shared_library bib{};
		dls::chaine chemin{};
	};

	using type_fonction = void(*)();

private:
	dls::tableau<BibliothequePartagee> bibliotheques{};
	dls::dico<IdentifiantCode *, type_fonction> symboles_et_fonctions{};

public:
	void ajoute_bibliotheque(dls::chaine const &chemin);
	void ajoute_fonction_pour_symbole(IdentifiantCode *symbole, type_fonction fonction);
	type_fonction fonction_pour_symbole(IdentifiantCode *symbole);

	long memoire_utilisee() const;
};

struct FrameAppel {
	AtomeFonction *fonction = nullptr;
	octet_t *pointeur = nullptr;
	octet_t *pointeur_pile = nullptr;
};

enum {
	DONNEES_CONSTANTES,
	DONNEES_GLOBALES,
};

enum {
	ADRESSE_CONSTANTE,
	ADRESSE_GLOBALE,
};

// Ces patchs sont utilisés pour écrire au bon endroit les adresses des constantes ou des globales dans les constantes ou les globales.
// Par exemple, les pointeurs des infos types des membres des structures sont écris dans un tableau constant, et le pointeur du tableau
// constant doit être écris dans la zone mémoire ou se trouve le tableaeu de membre de l'InfoTypeStructure.
struct PatchDonneesConstantes {
	int ou;
	int quoi;
	int decalage_ou;
	int decalage_quoi;
};

std::ostream &operator<<(std::ostream &os, PatchDonneesConstantes const &patch);

static constexpr auto TAILLE_FRAMES_APPEL = 64;

struct DonneesExecution {
	dls::tableau<unsigned char> donnees_globales{};
	dls::tableau<unsigned char> donnees_constantes{};

	octet_t *pile = nullptr;
	octet_t *pointeur_pile = nullptr;

	FrameAppel frames[TAILLE_FRAMES_APPEL];
	int profondeur_appel = 0;

	int ajoute_globale(Type *type, IdentifiantCode *ident);
};

struct MachineVirtuelle {
private:
    enum class ResultatInterpretation : int {
        OK,
        ERREUR,
        COMPILATION_ARRETEE,
        TERMINE,
        PASSE_AU_SUIVANT,
    };

	Compilatrice &compilatrice;

	tableau_page<DonneesExecution> donnees_execution{};

	dls::tableau<MetaProgramme *> m_metaprogrammes{};
	dls::tableau<MetaProgramme *> m_metaprogrammes_termines{};

	bool m_metaprogrammes_termines_lu = false;

	/* données pour l'exécution de chaque métaprogramme */
	octet_t *pile = nullptr;
	octet_t *pointeur_pile = nullptr;

	unsigned char *ptr_donnees_constantes = nullptr;
	unsigned char *ptr_donnees_globales = nullptr;

	FrameAppel *frames = nullptr;
	int profondeur_appel = 0;

	int nombre_de_metaprogrammes_executes = 0;
	double temps_execution_metaprogammes = 0;

	MetaProgramme *m_metaprogramme = nullptr;

public:
	GestionnaireBibliotheques gestionnaire_bibliotheques{};

	dls::tableau<Globale> globales{};
	dls::tableau<unsigned char> donnees_globales{};
	dls::tableau<unsigned char> donnees_constantes{};
	dls::tableau<PatchDonneesConstantes> patchs_donnees_constantes{};

	bool stop = false;

	MachineVirtuelle(Compilatrice &compilatrice_);
	~MachineVirtuelle();

	COPIE_CONSTRUCT(MachineVirtuelle);

	typedef void (*fonction_symbole)();

	fonction_symbole trouve_symbole(IdentifiantCode *symbole);

	int ajoute_globale(Type *type, IdentifiantCode *ident);

	void ajoute_metaprogramme(MetaProgramme *metaprogramme);

	void execute_metaprogrammes_courants();

	dls::tableau<MetaProgramme *> const &metaprogrammes_termines();

	DonneesExecution *loge_donnees_execution();
	void deloge_donnees_execution(DonneesExecution *&donnees);

	bool terminee() const;

	void rassemble_statistiques(Statistiques &stats);

private:
	template <typename T>
	void empile(T valeur);

	template <typename T>
	T depile();

	void depile(long n);

	bool appel(AtomeFonction *fonction);

	bool appel_fonction_interne(AtomeFonction *ptr_fonction, int taille_argument, FrameAppel *&frame);
	void appel_fonction_externe(AtomeFonction *ptr_fonction, int taille_argument, InstructionAppel *inst_appel);

	void empile_constante(FrameAppel *frame);

	void installe_metaprogramme(MetaProgramme *metaprogramme);

	void desinstalle_metaprogramme(MetaProgramme *metaprogramme);

	ResultatInterpretation execute_instruction();
};

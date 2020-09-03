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

#include "biblinternes/systeme_fichier/shared_library.h"

struct AtomeFonction;
struct Compilatrice;
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

struct MachineVirtuelle {
	Compilatrice &compilatrice;
	dls::tableau<Globale> globales{};
	dls::tableau<unsigned char> donnees_globales{};

	octet_t *pile = nullptr;
	octet_t *pointeur_pile = nullptr;

	dls::tableau<unsigned char> donnees_constantes{};

	dls::tableau<PatchDonneesConstantes> patchs_donnees_constantes{};

	static constexpr auto TAILLE_FRAMES_APPEL = 64;
	FrameAppel frames[TAILLE_FRAMES_APPEL];
	int profondeur_appel = 0;

	GestionnaireBibliotheques gestionnaire_bibliotheques{};

	int nombre_de_metaprogrammes_executes = 0;
	double temps_execution_metaprogammes = 0;

	bool stop = false;

	enum ResultatInterpretation {
		OK,
		ERREUR,
	};

	MachineVirtuelle(Compilatrice &compilatrice_);
	~MachineVirtuelle();

	COPIE_CONSTRUCT(MachineVirtuelle);

	void reinitialise_pile();

	template <typename T>
	void empile(T valeur);

	template <typename T>
	T depile();

	long depile();

	void depile(long n);

	bool appel(AtomeFonction *fonction, int taille_argument);

	ResultatInterpretation interprete(AtomeFonction *fonction);

	ResultatInterpretation lance();

	void empile_constante(FrameAppel *frame);

	typedef void (*fonction_symbole)();

	fonction_symbole trouve_symbole(IdentifiantCode *symbole);

	int ajoute_globale(Type *type, IdentifiantCode *ident);
};

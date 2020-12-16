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

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/resultat.hh"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/structures/tablet.hh"
#include "biblinternes/structures/tuples.hh"

#include <mutex>

#include "lexemes.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct IdentifiantCode;
struct MetaProgramme;
struct Module;
struct NoeudBloc;
struct NoeudDeclaration;
struct NoeudDeclarationCorpsFonction;
struct Statistiques;

struct DonneesConstantesFichier {
	double temps_chargement = 0.0;
	double temps_decoupage = 0.0;
	double temps_tampon = 0.0;

	lng::tampon_source tampon{""};

	dls::tableau<Lexeme> lexemes{};

	dls::chaine nom{""};
	dls::chaine chemin{""};

	long id = 0;

	std::mutex mutex{};
	bool fut_lexe = false;
	bool fut_charge = false;
	bool en_chargement = false;
	bool en_lexage = false;

	void charge_tampon(lng::tampon_source &&t)
	{
		tampon = std::move(t);
		fut_charge = true;
	}
};

struct Fichier {
	double temps_analyse = 0.0;

	DonneesConstantesFichier *donnees_constantes = nullptr;

	dls::ensemblon<IdentifiantCode *, 16> modules_importes{};

	Module *module = nullptr;
	MetaProgramme *metaprogramme_corps_texte = nullptr;

	Fichier() = default;

	Fichier(DonneesConstantesFichier *dc)
		: donnees_constantes(dc)
	{}

	COPIE_CONSTRUCT(Fichier);

	/**
	 * Retourne vrai si le fichier importe un module du nom spécifié.
	 */
	bool importe_module(IdentifiantCode *nom_module) const;

	dls::chaine const &chemin() const
	{
		return donnees_constantes->chemin;
	}

	dls::chaine const &nom() const
	{
		return donnees_constantes->nom;
	}

	long id() const
	{
		return donnees_constantes->id;
	}

	lng::tampon_source const &tampon() const
	{
		return donnees_constantes->tampon;
	}
};

template <int i>
struct EnveloppeFichier {
	static const int tag;

	Fichier *fichier = nullptr;

	EnveloppeFichier(Fichier &f)
		: fichier(&f)
	{}
};

using FichierExistant = EnveloppeFichier<0>;
using FichierNeuf = EnveloppeFichier<1>;

using ResultatFichier = Resultat<FichierExistant, FichierNeuf>;

struct DonneesConstantesModule {
	/* le nom du module, qui est le nom du dossier où se trouve les fichiers */
	IdentifiantCode *nom = nullptr;
	dls::chaine chemin{""};
};

struct Module {
	DonneesConstantesModule *donnees_constantes = nullptr;

	std::mutex mutex{};
	NoeudBloc *bloc = nullptr;

	dls::tablet<Fichier *, 16> fichiers{};
	bool importe = false;

	Module(DonneesConstantesModule *dc)
		: donnees_constantes(dc)
	{}

	COPIE_CONSTRUCT(Module);

	dls::chaine const &chemin() const
	{
		return donnees_constantes->chemin;
	}

	IdentifiantCode *const &nom() const
	{
		return donnees_constantes->nom;
	}
};

struct SystemeModule {
	tableau_page<DonneesConstantesModule> donnees_modules{};
	tableau_page<DonneesConstantesFichier> donnees_fichiers{};

	DonneesConstantesModule *trouve_ou_cree_module(IdentifiantCode *nom, dls::vue_chaine chemin);

	DonneesConstantesModule *cree_module(IdentifiantCode *nom, dls::vue_chaine chemin);

	DonneesConstantesFichier *trouve_ou_cree_fichier(dls::vue_chaine nom, dls::vue_chaine chemin);

	DonneesConstantesFichier *cree_fichier(dls::vue_chaine nom, dls::vue_chaine chemin);

	void rassemble_stats(Statistiques &stats) const;

	long memoire_utilisee() const;
};

/* ************************************************************************** */

struct PositionLexeme {
	long index_ligne = 0;
	long numero_ligne = 0;
	long pos = 0;
};

PositionLexeme position_lexeme(Lexeme const &lexeme);

void imprime_fichier_ligne(const EspaceDeTravail &espace, Lexeme const &lexeme);

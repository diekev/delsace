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

#include <list>
#include <set>
#include <string>
#include <unordered_map>

#include <delsace/langage/tampon_source.hh>

#include "donnees_type.h"
#include "morceaux.h"

namespace llvm {
class Type;
}

namespace noeud {
struct base;
}

struct ContexteGenerationCode;

struct DonneesArgument {
	size_t index = 0;
	size_t donnees_type{-1ul};
	bool est_variadic = false;
	bool est_dynamic = false;
	char pad[6];
};

struct DonneesCoroutine {
	using paire_donnees = std::pair<size_t, char>;
	using paire_variable = std::pair<std::string, paire_donnees>;
	std::vector<paire_variable> variables{};
	int nombre_retenues = 0;
};

struct DonneesFonction {
	std::unordered_map<std::string_view, DonneesArgument> args{};
	size_t index_type_retour{-1ul};
	size_t index_type{-1ul};
	std::vector<std::string_view> nom_args{};
	std::string nom_broye{};
	noeud::base *noeud_decl = nullptr;
	bool est_externe = false;
	bool est_variadique = false;
	bool est_coroutine = false;
	char pad[5];
	DonneesCoroutine donnees_coroutine{};
};

struct DonneesModule {
	lng::tampon_source tampon{""};
	std::vector<DonneesMorceaux> morceaux{};
	std::set<std::string_view> modules_importes{};
	std::set<std::string_view> fonctions_exportees{};
	std::unordered_map<std::string_view, std::vector<DonneesFonction>> fonctions{};
	size_t id = 0ul;
	std::string nom{""};
	std::string chemin{""};
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;

	DonneesModule() = default;

	/**
	 * Retourne vrai si le module importe un module du nom spécifié.
	 */
	bool importe_module(std::string_view const &nom_module) const;

	/**
	 * Retourne vrai si le module possède une fonction du nom spécifié.
	 */
	bool possede_fonction(std::string_view const &nom_fonction) const;

	/**
	 * Ajoute les données de la fonction dont le nom est spécifié en paramètres
	 * à la table de fonctions de ce contexte.
	 */
	void ajoute_donnees_fonctions(std::string_view const &nom_fonction, DonneesFonction const &donnees);

	/**
	 * Retourne les données de la fonction dont le nom est spécifié en
	 * paramètre. Si aucune fonction ne portant ce nom n'existe, des données
	 * vides sont retournées.
	 */
	[[nodiscard]] std::vector<DonneesFonction> &donnees_fonction(std::string_view const &nom_fonction) noexcept;

	/**
	 * Retourne vrai si le nom spécifié en paramètre est celui d'une fonction
	 * ayant déjà été ajouté à la liste de fonctions de ce module.
	 */
	[[nodiscard]] bool fonction_existe(std::string_view const &nom_fonction) const noexcept;

	/**
	 * Retourne la mémoire utilisée en octet par les données de ce module. La
	 * quantité de mémoire utilisée n'est pas mise en cache est est recalculée
	 * à chaque appel à cette méthode.
	 */
	[[nodiscard]] size_t memoire_utilisee() const noexcept;

private:
	/* Utilisées comme retour dans donnees_fonction(nom). */
	DonneesFonction m_donnees_invalides{};
};

/**
 * Charge le module dont le nom est spécifié.
 *
 * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
 * dans le dossier du module racine.
 *
 * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
 * du module, sauf pour le module racine.
 *
 * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
 * pour tenir compte de la progression de la compilation. Si un nom de module ne
 * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
 * exception est lancée.
 *
 * Les DonneesMorceaux doivent être celles du nom du module et sont utilisées
 * pour les erreurs lancées.
 *
 * Le paramètre est_racine ne doit être vrai que pour le module racine.
 */
void charge_module(
		std::ostream &os,
		std::string const &racine_kuri,
		std::string const &nom,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau,
		bool est_racine = false);

/* ************************************************************************** */

enum {
	FONCTION_TROUVEE,
	FONCTION_INTROUVEE,
};

enum {
	AUCUNE_RAISON,
	METYPAGE_ARG,
	MENOMMAGE_ARG,
	RENOMMAGE_ARG,
	MECOMPTAGE_ARGS,
	MANQUE_NOM_APRES_VARIADIC,
};

struct DonneesCandidate {
	DonneesFonction *df = nullptr;
	int etat = FONCTION_INTROUVEE;
	int raison = AUCUNE_RAISON;
	double poids_args = 0.0;
	std::string_view nom_arg{};
	/* les expressions remises dans l'ordre selon les noms, si la fonction est trouvée. */
	std::vector<noeud::base *> exprs{};
	DonneesType type1{};
	DonneesType type2{};
	noeud::base *noeud_decl = nullptr;
	bool arg_pointeur = false;
	std::vector<niveau_compat> drapeaux{};
};

struct ResultatRecherche {
	std::vector<DonneesCandidate> candidates{};
};

ResultatRecherche cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		std::string_view const &nom,
		std::list<std::string_view> &noms_arguments,
		std::list<noeud::base *> const &exprs,
		size_t index_module,
		size_t index_module_appel);

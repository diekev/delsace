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

#include "contexte_generation_code.h"

#ifdef AVEC_LLVM
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <llvm/IR/LegacyPassManager.h>
#pragma GCC diagnostic pop
#endif

#include "assembleuse_arbre.h"
#include "broyage.hh"
#include "modules.hh"

ContexteGenerationCode::ContexteGenerationCode()
	: assembleuse(memoire::loge<assembleuse_arbre>("assembleuse_arbre", *this))
	, typeuse(graphe_dependance, this->operateurs)
{
	enregistre_operateurs_basiques(*this, this->operateurs);
}

ContexteGenerationCode::~ContexteGenerationCode()
{
	for (auto module : modules) {
		memoire::deloge("DonneesModule", module);
	}

	for (auto fichier : fichiers) {
		memoire::deloge("Fichier", fichier);
	}

	memoire::deloge("assembleuse_arbre", assembleuse);
}

/* ************************************************************************** */

DonneesModule *ContexteGenerationCode::cree_module(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	auto chemin_corrige = chemin;

	if (chemin_corrige[chemin_corrige.taille() - 1] != '/') {
		chemin_corrige.append('/');
	}

	for (auto module : modules) {
		if (module->chemin == chemin_corrige) {
			return module;
		}
	}

	auto module = memoire::loge<DonneesModule>("DonneesModule", *this);
	module->id = static_cast<size_t>(modules.taille());
	module->nom = nom;
	module->chemin = chemin_corrige;

	modules.pousse(module);

	return module;
}

DonneesModule *ContexteGenerationCode::module(size_t index) const
{
	return modules[static_cast<long>(index)];
}

DonneesModule *ContexteGenerationCode::module(const dls::vue_chaine_compacte &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return module;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::module_existe(const dls::vue_chaine_compacte &nom) const
{
	for (auto module : modules) {
		if (module->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

Fichier *ContexteGenerationCode::cree_fichier(
		dls::chaine const &nom,
		dls::chaine const &chemin)
{
	for (auto fichier : fichiers) {
		if (fichier->chemin == chemin) {
			return nullptr;
		}
	}

	auto fichier = memoire::loge<Fichier>("Fichier");
	fichier->id = static_cast<size_t>(fichiers.taille());
	fichier->nom = nom;
	fichier->chemin = chemin;

	if (importe_kuri) {
		fichier->modules_importes.insere("Kuri");
	}

	fichiers.pousse(fichier);

	return fichier;
}

Fichier *ContexteGenerationCode::fichier(size_t index) const
{
	return fichiers[static_cast<long>(index)];
}

Fichier *ContexteGenerationCode::fichier(const dls::vue_chaine_compacte &nom) const
{
	for (auto fichier : fichiers) {
		if (fichier->nom == nom) {
			return fichier;
		}
	}

	return nullptr;
}

bool ContexteGenerationCode::fichier_existe(const dls::vue_chaine_compacte &nom) const
{
	for (auto fichier : fichiers) {
		if (fichier->nom == nom) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

void ContexteGenerationCode::empile_goto_continue(dls::vue_chaine_compacte chaine, dls::chaine const &bloc)
{
	m_pile_goto_continue.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_continue()
{
	m_pile_goto_continue.pop_back();
}

dls::chaine ContexteGenerationCode::goto_continue(dls::vue_chaine_compacte chaine)
{
	if (m_pile_goto_continue.est_vide()) {
		return "";
	}

	if (chaine.est_vide()) {
		return m_pile_goto_continue.back().second;
	}

	for (auto const &paire : m_pile_goto_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::empile_goto_arrete(dls::vue_chaine_compacte chaine, dls::chaine const &bloc)
{
	m_pile_goto_arrete.pousse({chaine, bloc});
}

void ContexteGenerationCode::depile_goto_arrete()
{
	m_pile_goto_arrete.pop_back();
}

dls::chaine ContexteGenerationCode::goto_arrete(dls::vue_chaine_compacte chaine)
{
	if (m_pile_goto_arrete.est_vide()) {
		return "";
	}

	if (chaine.est_vide()) {
		return m_pile_goto_arrete.back().second;
	}

	for (auto const &paire : m_pile_goto_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::commence_fonction(NoeudDeclarationFonction *df)
{
	this->donnees_fonction = df;
}

void ContexteGenerationCode::termine_fonction()
{
	this->donnees_fonction = nullptr;
}

/* ************************************************************************** */

size_t ContexteGenerationCode::memoire_utilisee() const
{
	auto memoire = sizeof(ContexteGenerationCode);

	/* À FAIRE : réusinage arbre */
//	for (auto module : modules) {
//		memoire += module->memoire_utilisee();
//	}

	return memoire;
}

Metriques ContexteGenerationCode::rassemble_metriques() const
{
	auto metriques = Metriques{};
	metriques.nombre_modules  = static_cast<size_t>(modules.taille());
	metriques.temps_validation = this->temps_validation;
	metriques.temps_generation = this->temps_generation;
	metriques.memoire_types = this->typeuse.memoire_utilisee();
	metriques.memoire_operateurs = this->operateurs.memoire_utilisee();

	metriques.memoire_arbre += this->allocatrice_noeud.memoire_utilisee();
	metriques.nombre_noeuds += this->allocatrice_noeud.nombre_noeuds();

	metriques.nombre_types = typeuse.nombre_de_types();

	POUR (operateurs.donnees_operateurs) {
		metriques.nombre_operateurs += it.second.taille();
	}

	for (auto fichier : fichiers) {
		metriques.nombre_lignes += fichier->tampon.nombre_lignes();
		metriques.memoire_tampons += fichier->tampon.taille_donnees();
		metriques.memoire_lexemes += static_cast<size_t>(fichier->lexemes.taille()) * sizeof(Lexeme);
		metriques.nombre_lexemes += static_cast<size_t>(fichier->lexemes.taille());
		metriques.temps_analyse += fichier->temps_analyse;
		metriques.temps_chargement += fichier->temps_chargement;
		metriques.temps_tampon += fichier->temps_tampon;
		metriques.temps_decoupage += fichier->temps_decoupage;
	}

	return metriques;
}

/* ************************************************************************** */

dls::vue_chaine_compacte ContexteGenerationCode::trouve_membre_actif(const dls::vue_chaine_compacte &nom_union)
{
	for (auto const &paire : membres_actifs) {
		if (paire.first == nom_union) {
			return paire.second;
		}
	}

	return "";
}

void ContexteGenerationCode::renseigne_membre_actif(const dls::vue_chaine_compacte &nom_union, const dls::vue_chaine_compacte &nom_membre)
{
	for (auto &paire : membres_actifs) {
		if (paire.first == nom_union) {
			paire.second = nom_membre;
			return;
		}
	}

	membres_actifs.pousse({ nom_union, nom_membre });
}

/* ************************************************************************** */

static int hex_depuis_char(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}

	if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}

	return 256;
}

GeranteChaine::~GeranteChaine()
{
	POUR (m_table) {
		kuri::detruit_chaine(it.second);
	}
}

char GeranteChaine::valide_caractere(const char *ptr, int &i, bool &ok, const char *&message_erreur, int &position)
{
	auto c = ptr[i++];

	if (c == '\\') {
		c = ptr[i++];

		if (c == 'n') {
			c = '\n';
		}
		else if (c == 't') {
			c = '\t';
		}
		else if (c == 'e') {
			c = 0x1b;
		}
		else if (c == '\\') {
			// RÀF
		}
		else if (c == '0') {
			c = 0;
		}
		else if (c == '"') {
			c = '"';
		}
		else if (c == '\'') {
			c = '\'';
		}
		else if (c == 'r') {
			c = '\r';
		}
		else if (c == 'f') {
			c = '\f';
		}
		else if (c == 'v') {
			c = '\v';
		}
		else if (c == 'x') {
			auto v = 0;

			for (auto j = 0; j < 2; ++j) {
				auto n = ptr[i++];

				auto c0 = hex_depuis_char(n);

				if (c0 == 256) {
					ok = false;
					message_erreur = "Caractère invalide la séquence hexadécimale";
					position = i - 1;
					break;
				}

				v <<= 4;
				v |= c0;
			}

			if (!ok) {
				return c;
			}

			c = static_cast<char>(v);
		}
		else if (c == 'd') {
			auto v = 0;

			for (auto j = 0; j < 3; ++j) {
				auto n = ptr[i++];

				if (n < '0' || n > '9') {
					ok = false;
					message_erreur = "Caractère invalide la séquence décimale";
					position = i - 1;
					break;
				}

				v *= 10;
				v += (n - '0');
			}

			if (!ok) {
				return c;
			}

			if (v > 255) {
				ok = false;
				message_erreur = "Valeur décimale trop grande, le maximum est 255";
				position = i - 3;
				return c;
			}

			c = static_cast<char>(v);
		}
		else if (c == 'u') {
			// À FAIRE : génére octets depuis UTF-32
			auto v = 0;

			for (auto j = 0; j < 4; ++j) {
				auto n = ptr[i++];

				auto c0 = hex_depuis_char(n);

				if (c0 == 256) {
					ok = false;
					message_erreur = "Caractère invalide la séquence hexadécimale";
					position = i - 1;
					break;
				}

				v <<= 4;
				v |= c0;
			}

			if (!ok) {
				return c;
			}

			c = static_cast<char>(v);
		}
		else if (c == 'U') {
			// À FAIRE : génére octets depuis UTF-32
			auto v = 0;

			for (auto j = 0; j < 8; ++j) {
				auto n = ptr[i++];

				auto c0 = hex_depuis_char(n);

				if (c0 == 256) {
					ok = false;
					message_erreur = "Caractère invalide la séquence hexadécimale";
					position = i - 1;
					break;
				}

				v <<= 4;
				v |= c0;
			}

			if (!ok) {
				return c;
			}

			c = static_cast<char>(v);
		}
		else {
			ok = false;
			message_erreur = "Séquence d'échappement invalide";
			position = i - 1;
			return c;
		}
	}

	return c;
}

GeranteChaine::Resultat GeranteChaine::ajoute_chaine(const dls::vue_chaine_compacte &chaine)
{
	auto iter = m_table.trouve(chaine);

	if (iter != m_table.fin()) {
		return { iter->second, true, "", 0 };
	}

	/* Séquences d'échappement du langage :
	 * \e : insère un caractère d'échappement (par exemple pour les couleurs dans les terminaux)
	 * \f : insère un saut de page
	 * \n : insère une nouvelle ligne
	 * \r : insère un retour chariot
	 * \t : insère une tabulation horizontale
	 * \v : insère une tabulation verticale
	 * \0 : insère un octet dont la valeur est de zéro (0)
	 * \' : insère une apostrophe
	 * \" : insère un guillemet
	 * \\ : insère un slash arrière
	 * \dnnn      : insère une valeur décimale, où n est nombre décimal
	 * \unnnn     : insère un caractère Unicode dont seuls les 16-bits du bas sont spécifiés, où n est un nombre hexadécimal
	 * \Unnnnnnnn : insère un caractère Unicode sur 32-bits, où n est un nombre hexadécimal
	 * \xnn       : insère une valeur hexadécimale, où n est un nombre hexadécimal
	 */

	auto ptr = chaine.pointeur();

	auto chaine_resultat = kuri::chaine();
	auto ok = true;
	auto message_erreur = "";
	auto position = 0;

	for (auto i = 0; i < chaine.taille();) {
		auto c = valide_caractere(ptr, i, ok, message_erreur, position);

		if (!ok) {
			break;
		}

		chaine_resultat.pousse(c);
	}

	if (ok) {
		m_table.insere({ chaine, chaine_resultat });
	}
	else {
		kuri::detruit_chaine(chaine_resultat);
	}

	return { chaine_resultat, ok, message_erreur, position };
}

kuri::chaine GeranteChaine::trouve_chaine(const dls::vue_chaine_compacte &chaine)
{
	auto iter = m_table.trouve(chaine);

	if (iter != m_table.fin()) {
		return iter->second;
	}

	return ajoute_chaine(chaine).c;
}

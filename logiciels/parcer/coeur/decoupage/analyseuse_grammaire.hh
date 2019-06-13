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

#include "biblinternes/langage/analyseuse.hh"

#include "assembleuse_arbre.hh"
#include "erreur.hh"

class DonneesType;

struct ContexteGenerationCode;
struct DonneesFonction;
struct DonneesModule;

class analyseuse_grammaire : public lng::analyseuse<DonneesMorceaux> {
	ContexteGenerationCode &m_contexte;
	assembleuse_arbre *m_assembleuse = nullptr;

	/* Ces vecteurs sont utilisés pour stocker les données des expressions
	 * compilées au travers de 'analyse_expression_droite()'. Nous les stockons
	 * pour pouvoir réutiliser la mémoire qu'ils allouent après leurs
	 * utilisations. Ainsi nous n'avons pas à récréer des vecteurs à chaque
	 * appel vers 'analyse_expression_droite()', mais cela rend la classe peu
	 * sûre niveau multi-threading.
	 */
	using paire_vecteurs = std::pair<std::vector<noeud::base *>, std::vector<noeud::base *>>;
	std::vector<paire_vecteurs> m_paires_vecteurs;
	size_t m_profondeur = 0;

	std::string m_racine_kuri{};

	DonneesModule *m_module;

	double m_debut_analyse = 0.0;

public:
	analyseuse_grammaire(
			ContexteGenerationCode &contexte,
			DonneesModule *module,
			std::string const &racine_kuri);

	/* Désactive la copie, car il ne peut y avoir qu'une seule analyseuse par
	 * module. */
	analyseuse_grammaire(analyseuse_grammaire const &) = delete;
	analyseuse_grammaire &operator=(analyseuse_grammaire const &) = delete;

	void lance_analyse(std::ostream &os) override;

private:
	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaine passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(
			const std::string &quoi,
			erreur::type_erreur type = erreur::type_erreur::NORMAL);

	void analyse_corps(std::ostream &os);
	void analyse_expression_droite(id_morceau identifiant_final, id_morceau racine_expr, bool const calcul_expression = false);

	bool requiers_identifiant_type();
	bool requiers_nombre_entier();
};

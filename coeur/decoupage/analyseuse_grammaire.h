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

#include "analyseuse.h"

#include "assembleuse_arbre.h"

class DonneesType;
class NoeudAppelFonction;
class NoeudDeclarationFonction;

struct ContexteGenerationCode;
struct DonneesFonction;

class analyseuse_grammaire : public Analyseuse {
	assembleuse_arbre *m_assembleuse = nullptr;
	ContexteGenerationCode &m_contexte;

	/* Ces vecteurs sont utilisés pour stocker les données des expressions
	 * compilées au travers de 'analyse_expression_droite()'. Nous les stockons
	 * pour pouvoir réutiliser la mémoire qu'ils allouent après leurs
	 * utilisations. Ainsi nous n'avons pas à récréer des vecteurs à chaque
	 * appel vers 'analyse_expression_droite()', mais cela rend la classe peu
	 * sûre niveau multi-threading.
	 */
	using paire_vecteurs = std::pair<std::vector<Noeud *>, std::vector<Noeud *>>;
	std::vector<paire_vecteurs> m_paires_vecteurs;
	size_t m_profondeur = 0;

public:
	analyseuse_grammaire(ContexteGenerationCode &contexte, const std::vector<DonneesMorceaux> &identifiants, const TamponSource &tampon, assembleuse_arbre *assembleuse);

	void lance_analyse() override;

private:
	void analyse_corps();
	void analyse_declaration_fonction();
	void analyse_parametres_fonction(NoeudDeclarationFonction *noeud, DonneesFonction &donnees);
	void analyse_corps_fonction();
	void analyse_expression_droite(id_morceau identifiant_final, const bool calcul_expression = false, const bool assignation = false);
	void analyse_appel_fonction(NoeudAppelFonction *noeud);
	void analyse_declaration_structure();
	void analyse_declaration_constante();
	void analyse_declaration_enum();
	void analyse_declaration_type(DonneesType &donnees_type, bool double_point = true);
	void analyse_controle_si();

	bool requiers_identifiant_type();
	bool requiers_nombre_entier();
};

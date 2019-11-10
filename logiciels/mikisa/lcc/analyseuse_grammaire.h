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

#include "arbre_syntactic.h"
#include "morceaux.hh"
#include "erreur.h"

struct assembleuse_arbre;
struct ContexteGenerationCode;
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
	using paire_vecteurs = std::pair<dls::tableau<lcc::noeud::base *>, dls::tableau<lcc::noeud::base *>>;
	dls::tableau<paire_vecteurs> m_paires_vecteurs;
	long m_profondeur = 0;

	DonneesModule *m_module;

public:
	analyseuse_grammaire(
			ContexteGenerationCode &contexte,
			DonneesModule *module);

	/* Désactive la copie, car il ne peut y avoir qu'une seule analyseuse par
	 * module. */
	analyseuse_grammaire(analyseuse_grammaire const &) = delete;
	analyseuse_grammaire &operator=(analyseuse_grammaire const &) = delete;

	void lance_analyse(std::ostream &os) override;

private:
	void analyse_expression(id_morceau identifiant_final, bool const calcul_expression = false);
	void analyse_appel_fonction();
	void analyse_bloc();
	void analyse_controle_si();
	void analyse_controle_pour();
	void analyse_directive();
	void analyse_parametre(lcc::type_var type_param);

	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(
			const dls::chaine &quoi,
			erreur::type_erreur type = erreur::type_erreur::NORMAL);
};

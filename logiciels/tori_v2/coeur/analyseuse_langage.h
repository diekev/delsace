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
#include "biblinternes/structures/dico_desordonne.hh"

#include "morceaux.h"
#include "postfix.h"

namespace langage {

struct DonneesVariables {
	dls::chaine valeur{};

	DonneesVariables() = default;
};

using Expression = dls::tableau<Variable>;

struct DonneesFonction {
	dls::dico_desordonne<dls::chaine, DonneesVariables> variables_locales{};
	dls::tableau<Expression> expressions{};
	Expression expression_retour{};
};

struct DonneesScript {
	dls::dico_desordonne<dls::chaine, DonneesVariables> variables{};
	dls::dico_desordonne<dls::chaine, DonneesFonction> fonctions{};
};

class AnalyseuseLangage : public lng::analyseuse<DonneesMorceaux> {
	DonneesScript m_donnees_script{};
	dls::dico_desordonne<dls::chaine, double> m_chronometres{};

public:
	AnalyseuseLangage(dls::tableau<DonneesMorceaux> &morceaux);

	void lance_analyse(std::ostream &os) override;

private:
	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	void lance_erreur(const dls::chaine &quoi);

	void analyse_declaration();
	void analyse_imprime();
	void analyse_variable();
	void analyse_chronometre();
	void analyse_fonction();
	void analyse_expression(DonneesFonction &donnees_fonction);
};

}  /* namespace langage */

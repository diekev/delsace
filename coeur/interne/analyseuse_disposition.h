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

#include "base_analyseuse.h"

namespace danjo {

class AssembleurDisposition;

/**
 * La classe AnalyseuseDisposition s'occupe de vérifié que la grammaire du
 * langage de définition des dispositions respectée. Dès que possible,
 * l'analyseuse notifie son assembleur pour le faire créer les éléments
 * rencontrés.
 *
 * Si une erreur de syntaxe est repérée, une exception de type ErreurSyntactique
 * est lancée.
 */
class AnalyseuseDisposition : public Analyseuse {
	AssembleurDisposition *m_assembleur = nullptr;

public:
	AnalyseuseDisposition() = default;

	/**
	 * Installe l'assembleur à utiliser pour générer l'interface.
	 */
	void installe_assembleur(AssembleurDisposition *assembleur);

	/**
	 * Lance l'analyse syntactique du vecteur d'identifiants spécifié.
	 *
	 * Si aucun assembleur n'est installé lors de l'appel de cette méthode,
	 * une exception est lancée.
	 */
	void lance_analyse(const std::vector<DonneesMorceaux> &identifiants) override;

private:
	/**
	 * Débute l'analyse du script, en vérifiant que les identifiants
	 * correspondent à la grammaire suivante :
	 *
	 * disposition "nom_disposition" { disposition... }
	 */
	void analyse_script_disposition();

	/**
	 * Analyse le contenu d'une disposition. La disposition doit contenir soit
	 * un élément de disposition (ligne ou colonne), soit des contrôles ou
	 * boutons.
	 */
	void analyse_disposition();

	/**
	 * Analyse la déclaration d'une ligne en vérifiant que les identifiants
	 * correspondent à la grammaire suivante :
	 *
	 * ligne { disposition... }
	 */
	void analyse_ligne();

	/**
	 * Analyse la déclaration d'une colonne en vérifiant que les identifiants
	 * correspondent à la grammaire suivante :
	 *
	 * colonne { disposition... }
	 */
	void analyse_colonne();

	/**
	 * Analyse la déclaration d'un contrôle en vérifiant que les identifiants
	 * correspondent à la des grammaire suivante :
	 *
	 * identifiant_contrôle ( propriété... )
	 *
	 * avec identifiant_contrôle pouvant prendre l'une des valeurs suivantes :
	 * case, couleur, décimal, entier, étiquette, fichier_entrée,
	 * fichier_sortie, liste, vecteur.
	 */
	void analyse_controle();

	/**
	 * Analyse la déclaration d'un bouton en vérifiant que les identifiants
	 * correspondent à l'une des grammaires suivantes :
	 *
	 * bouton ( propriété... )
	 */
	void analyse_bouton();

	/**
	 * Analyse la déclaration d'une propriété d'un contrôle ou d'un bouton en
	 * vérifiant que les identifiants correspondent à la grammaire suivante :
	 *
	 * identifiant_propriété = "chaîne de caractères";
	 *
	 * avec identifiant_propriété pouvant prendre l'une des valeurs suivantes :
	 * valeur, min, max, items, infobulle, précision, pas, attache.
	 */
	void analyse_propriete(int type_controle);

	/**
	 * Analyse la déclaration d'une liste d'items pour un contrôle de type
	 * liste déroulante en vérifiant que les identifiants correspondent à la
	 * grammaire suivante :
	 *
	 * [ item... ]
	 */
	void analyse_liste_item();

	/**
	 * Analyse la déclaration d'une item d'une liste d'items en vérifiant que
	 * les identifiants correspondent à la grammaire suivante :
	 *
	 * { nom = "chaîne de caractères", valeur = "chaîne de caractères" },
	 */
	void analyse_item();

	void analyse_action();
	void analyse_menu();
	void analyse_script_menu();

	/**
	 * Analyse la déclaration d'un dossier en vérifiant que les identifiants
	 * correspondent à la grammaire suivante :
	 *
	 * dossier { onglet... }
	 */
	void analyse_dossier();

	/**
	 * Analyse la déclaration d'un onglet en vérifiant que les identifiants
	 * correspondent à la grammaire suivante :
	 *
	 * onglet "chaîne de caractères" { disposition... }
	 */
	void analyse_onglet();
	void analyse_script_barre_outils();
	void analyse_barre_outils();
};

}  /* namespace danjo */

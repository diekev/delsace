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

#include "allocatrice_noeud.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/definitions.h"

#include "statistiques.hh"

long AllocatriceNoeud::nombre_noeuds() const
{
	auto noeuds = 0l;

	noeuds += m_noeuds_bloc.taille();
	noeuds += m_noeuds_comme.taille();
	noeuds += m_noeuds_declaration_variable.taille();
	noeuds += m_noeuds_declaration_corps_fonction.taille();
	noeuds += m_noeuds_declaration_entete_fonction.taille();
	noeuds += m_noeuds_enum.taille();
	noeuds += m_noeuds_struct.taille();
	noeuds += m_noeuds_expression_binaire.taille();
	noeuds += m_noeuds_expression_membre.taille();
	noeuds += m_noeuds_expression_reference.taille();
	noeuds += m_noeuds_appel.taille();
	noeuds += m_noeuds_expression_unaire.taille();
	noeuds += m_noeuds_expression.taille();
	noeuds += m_noeuds_boucle.taille();
	noeuds += m_noeuds_pour.taille();
	noeuds += m_noeuds_discr.taille();
	noeuds += m_noeuds_si.taille();
	noeuds += m_noeuds_pousse_contexte.taille();
	noeuds += m_noeuds_tableau_args_variadiques.taille();
	noeuds += m_noeuds_directive_execution.taille();
	noeuds += m_noeuds_expression_virgule.taille();
	noeuds += m_noeuds_litterales.taille();
	noeuds += m_noeuds_module.taille();

	return noeuds;
}

#undef COMPTE_RECHERCHES

#ifdef COMPTE_RECHERCHES
static auto nombre_recherches = 0;
#endif

void AllocatriceNoeud::rassemble_statistiques(Statistiques &stats) const
{
	auto &stats_arbre = stats.stats_arbre;
	auto taille_max_donnees_assignations = 0;
	auto taille_max_donnees_assignations_vars = 0;
	auto taille_max_donnees_assignations_tfms = 0;
	auto taille_max_arbre_aplatis = 0;
	auto taille_max_params_entree = 0;
	auto taille_max_params_sorties = 0;
	auto taille_max_annotations = 0;
	auto taille_max_monomorphisations = 0;
	auto taille_max_items_monomorphisations = 0;
	auto taille_max_params_appels = 0;
	auto taille_max_noeuds_differes = 0;
	auto taille_max_paires_discr = 0;
	auto taille_max_expr_variadiques = 0;

#define DONNEES_ENTREE(Nom, Tableau) \
	Nom, Tableau.taille(), Tableau.memoire_utilisee()

	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudEnum", m_noeuds_enum) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionBinaire", m_noeuds_expression_binaire) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionMembre", m_noeuds_expression_membre) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionReference", m_noeuds_expression_reference) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionUnaire", m_noeuds_expression_unaire) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpression", m_noeuds_expression) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudBoucle", m_noeuds_boucle) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudPour", m_noeuds_pour) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudSi", m_noeuds_si) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudSiStatique", m_noeuds_si_statique) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudPousseContexte", m_noeuds_pousse_contexte) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudTente", m_noeuds_tente) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudDirectiveExecution", m_noeuds_directive_execution) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudComme", m_noeuds_comme) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionLitterale", m_noeuds_litterales) });
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudModule", m_noeuds_module) });

	auto memoire_retour = 0l;
	pour_chaque_element(m_noeuds_retour, [&](NoeudRetour const &noeud)
	{
		memoire_retour += noeud.donnees_exprs.taille() * taille_de(DonneesAssignations);
		taille_max_donnees_assignations = std::max(taille_max_donnees_assignations, noeud.donnees_exprs.taille());
		POUR (noeud.donnees_exprs.plage()) {
			taille_max_donnees_assignations_vars = std::max(taille_max_donnees_assignations_vars, it.variables.taille());
			taille_max_donnees_assignations_tfms = std::max(taille_max_donnees_assignations_tfms, it.transformations.taille());
		}
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudRetour", m_noeuds_retour) + memoire_retour });

	auto memoire_assignation = 0l;
	pour_chaque_element(m_noeuds_assignation, [&](NoeudAssignation const &noeud)
	{
		memoire_assignation += noeud.donnees_exprs.taille() * taille_de(DonneesAssignations);
		taille_max_donnees_assignations = std::max(taille_max_donnees_assignations, noeud.donnees_exprs.taille());
		POUR (noeud.donnees_exprs.plage()) {
			taille_max_donnees_assignations_vars = std::max(taille_max_donnees_assignations_vars, it.variables.taille());
			taille_max_donnees_assignations_tfms = std::max(taille_max_donnees_assignations_tfms, it.transformations.taille());
		}
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudAssignation", m_noeuds_assignation) + memoire_assignation });

	auto memoire_decl = 0l;
	pour_chaque_element(m_noeuds_declaration_variable, [&](NoeudDeclarationVariable const &noeud)
	{
		memoire_decl += noeud.donnees_decl.taille() * taille_de(DonneesAssignations);
		taille_max_donnees_assignations = std::max(taille_max_donnees_assignations, noeud.donnees_decl.taille());
		POUR (noeud.donnees_decl.plage()) {
			taille_max_donnees_assignations_vars = std::max(taille_max_donnees_assignations_vars, it.variables.taille());
			taille_max_donnees_assignations_tfms = std::max(taille_max_donnees_assignations_tfms, it.transformations.taille());
		}
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudDeclarationVariable", m_noeuds_declaration_variable) + memoire_decl });

	auto memoire_struct = 0l;
	pour_chaque_element(m_noeuds_struct, [&](NoeudStruct const &noeud)
	{
		memoire_struct += noeud.arbre_aplatis.taille_memoire();
		taille_max_arbre_aplatis = std::max(taille_max_arbre_aplatis, noeud.arbre_aplatis.taille());

		if (noeud.monomorphisations) {
			memoire_struct += noeud.monomorphisations->memoire_utilisee();
			taille_max_items_monomorphisations = std::max(taille_max_items_monomorphisations, noeud.monomorphisations->nombre_items_max());
			taille_max_monomorphisations = std::max(taille_max_monomorphisations, noeud.monomorphisations->taille());
		}
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudStruct", m_noeuds_struct) + memoire_struct });

	auto memoire_bloc = 0l;
	pour_chaque_element(m_noeuds_bloc, [&](NoeudBloc const &noeud)
	{
		memoire_bloc += noeud.membres->taille_memoire();
		memoire_bloc += noeud.expressions->taille_memoire();
		memoire_bloc += noeud.noeuds_differes.taille_memoire();
		taille_max_noeuds_differes = std::max(taille_max_noeuds_differes, noeud.noeuds_differes.taille());

#ifdef COMPTE_RECHERCHES
		if (noeud.nombre_recherches) {
			std::cerr << "Taille bloc : " << noeud.membres->taille << ", recherches : " << noeud.nombre_recherches << '\n';
		}

		nombre_recherches += noeud.nombre_recherches * noeud.membres->taille;
#endif
	});

#ifdef COMPTE_RECHERCHES
	std::cerr << "nombre_recherches : " << nombre_recherches << '\n';
#endif

	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudBloc", m_noeuds_bloc) + memoire_bloc });

	auto memoire_corps_fonction = 0l;
	pour_chaque_element(m_noeuds_declaration_corps_fonction, [&](NoeudDeclarationCorpsFonction const &noeud)
	{
		memoire_corps_fonction += noeud.arbre_aplatis.taille_memoire();
		taille_max_arbre_aplatis = std::max(taille_max_arbre_aplatis, noeud.arbre_aplatis.taille());
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudDeclarationCorpsFonction", m_noeuds_declaration_corps_fonction) + memoire_corps_fonction });

	auto memoire_entete_fonction = 0l;
	pour_chaque_element(m_noeuds_declaration_entete_fonction, [&](NoeudDeclarationEnteteFonction const &noeud)
	{
		memoire_entete_fonction += noeud.params.taille_memoire();
		memoire_entete_fonction += noeud.params_sorties.taille_memoire();
		memoire_entete_fonction += noeud.arbre_aplatis.taille_memoire();
		memoire_entete_fonction += noeud.annotations.taille_memoire();
		taille_max_arbre_aplatis = std::max(taille_max_arbre_aplatis, noeud.arbre_aplatis.taille());
		taille_max_params_entree = std::max(taille_max_params_entree, noeud.params.taille());
		taille_max_params_sorties = std::max(taille_max_params_sorties, noeud.params_sorties.taille());
		taille_max_annotations = std::max(taille_max_annotations, noeud.annotations.taille());

		if (noeud.monomorphisations) {
			memoire_entete_fonction += noeud.monomorphisations->memoire_utilisee();
			taille_max_items_monomorphisations = std::max(taille_max_items_monomorphisations, noeud.monomorphisations->nombre_items_max());
			taille_max_monomorphisations = std::max(taille_max_monomorphisations, noeud.monomorphisations->taille());
		}

		memoire_entete_fonction += noeud.nom_broye_.taille();
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudDeclarationEnteteFonction", m_noeuds_declaration_entete_fonction) + memoire_entete_fonction });

	auto memoire_entete_appel = 0l;
	pour_chaque_element(m_noeuds_appel, [&](NoeudExpressionAppel const &noeud)
	{
		memoire_entete_appel += noeud.params.taille_memoire();
		memoire_entete_appel += noeud.exprs.taille_memoire();
		taille_max_params_appels = std::max(taille_max_params_appels, noeud.params.taille());
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionAppel", m_noeuds_appel) + memoire_entete_appel });

	auto memoire_discr = 0l;
	pour_chaque_element(m_noeuds_discr, [&](NoeudDiscr const &noeud)
	{
		memoire_discr += noeud.paires_discr.taille_memoire();
		taille_max_paires_discr = std::max(taille_max_paires_discr, noeud.paires_discr.taille());
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudDiscr", m_noeuds_discr) + memoire_discr });

	auto memoire_tableau_args_variadiques = 0l;
	pour_chaque_element(m_noeuds_tableau_args_variadiques, [&](NoeudTableauArgsVariadiques const &noeud)
	{
		memoire_tableau_args_variadiques += noeud.exprs.taille_memoire();
		taille_max_expr_variadiques = std::max(taille_max_expr_variadiques, noeud.exprs.taille());
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudTableauArgsVariadiques", m_noeuds_tableau_args_variadiques) + memoire_tableau_args_variadiques });

	auto memoire_expression_virgule = 0l;
	pour_chaque_element(m_noeuds_expression_virgule, [&](NoeudExpressionVirgule const &noeud)
	{
		memoire_expression_virgule += noeud.expressions.taille_memoire();
	});
	stats_arbre.fusionne_entree({ DONNEES_ENTREE("NoeudExpressionVirgule", m_noeuds_expression_virgule) + memoire_expression_virgule });


#undef DONNEES_ENTREE

	auto &stats_tableaux = stats.stats_tableaux;
#define DONNEES_TAILLE(x) stats_tableaux.fusionne_entree({ #x, x })
	DONNEES_TAILLE(taille_max_donnees_assignations);
	DONNEES_TAILLE(taille_max_donnees_assignations_vars);
	DONNEES_TAILLE(taille_max_donnees_assignations_tfms);
	DONNEES_TAILLE(taille_max_arbre_aplatis);
	DONNEES_TAILLE(taille_max_params_entree);
	DONNEES_TAILLE(taille_max_params_sorties);
	DONNEES_TAILLE(taille_max_annotations);
	DONNEES_TAILLE(taille_max_monomorphisations);
	DONNEES_TAILLE(taille_max_items_monomorphisations);
	DONNEES_TAILLE(taille_max_params_appels);
	DONNEES_TAILLE(taille_max_noeuds_differes);
	DONNEES_TAILLE(taille_max_paires_discr);
	DONNEES_TAILLE(taille_max_expr_variadiques);
	DONNEES_TAILLE(taille_max_donnees_assignations);
#undef DONNEES_TAILLE
}

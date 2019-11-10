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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "plan.hh"

struct ContexteEvaluation;
struct Mikisa;

/* ************************************************************************** */

/* Design Presto :
 * l'exécutrice se souvient des noeuds qui ont été, ou n'ont pas été exécuté
 *
 * les exécutrices utilisent des gestionnaires de données pour stocker les
 * données calculés ; chaque exécutrice posséde son propre gestionnaire pour
 * stocker les données pour tout le réseau -> pas de verrou nécessaire
 *
 * les gestionnaires stockent les données calculées ainsi que des masques de
 * validité, qui tiennent trace des états sales des sorties, et d'autres données
 * par noeud ou par sortie qui sont requises par l'exécutrice
 *
 * hierarchie d'exécutrices où les enfants peuvent lire depuis et parfois écrire
 * à ses parents pour éviter des calculs redondants
 *
 * peuvent être spécialisées pour utiliser différents algorithmes pour traiter
 * le réseau (par exemple pour différentes techniques de multithreading, ou
 * différent matériel)
 *
 * l'exécutrice incarne un schéma de multithreading
 *
 * multi-threading :
 * - par noeud (chaque noeud fait son propre multithreading)
 * - par branche (planifirice spécialisé)
 * - par objet (si aucune dépendance entre objet)
 * - par image (si possible quand aucune simulation n'est en cours, ou non tamponé)
 */

/* ************************************************************************** */

struct Executrice {
	void execute_plan(Mikisa &mikisa, Planifieuse::PtrPlan const &plan, ContexteEvaluation const &contexte);
};

/* ************************************************************************** */

class Composite;

void execute_graphe_composite(Mikisa &mikisa, Composite *composite, const char *message);

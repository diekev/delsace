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

#pragma once

struct Metriques {
    long nombre_modules = 0ul;
    long nombre_identifiants = 0ul;
    long nombre_lignes = 0ul;
    long nombre_lexemes = 0ul;
    long nombre_noeuds = 0ul;
    long nombre_noeuds_deps = 0ul;
    long nombre_metaprogrammes_executes = 0ul;
    long memoire_tampons = 0ul;
    long memoire_lexemes = 0ul;
    long memoire_arbre = 0ul;
    long memoire_compilatrice = 0ul;
    long memoire_types = 0ul;
    long memoire_operateurs = 0ul;
    long memoire_ri = 0ul;
    long memoire_graphe = 0ul;
    long memoire_mv = 0ul;
    long nombre_types = 0;
    long nombre_operateurs = 0;
    double temps_chargement = 0.0;
    double temps_analyse = 0.0;
    double temps_tampon = 0.0;
    double temps_decoupage = 0.0;
    double temps_validation = 0.0;
    double temps_generation_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_executable = 0.0;
    double temps_nettoyage = 0.0;
    double temps_ri = 0.0;
    double temps_metaprogrammes = 0.0;
    double temps_scene = 0.0;
};

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

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"

struct IdentifiantCode {
    kuri::chaine_statique nom{};
    kuri::chaine nom_broye{};
};

struct TableIdentifiant {
  private:
    kuri::table_hachage<dls::vue_chaine_compacte, IdentifiantCode *> table{};
    tableau_page<IdentifiantCode, 1024> identifiants{};

    Enchaineuse enchaineuse{};

  public:
    TableIdentifiant();

    IdentifiantCode *identifiant_pour_chaine(dls::vue_chaine_compacte const &nom);

    IdentifiantCode *identifiant_pour_nouvelle_chaine(kuri::chaine const &nom);

    long taille() const;

    long memoire_utilisee() const;

  private:
    IdentifiantCode *ajoute_identifiant(dls::vue_chaine_compacte const &nom);
};

#define ENUMERE_IDENTIFIANTS_COMMUNS                                                              \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(chaine_vide, "")                                            \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(Kuri, "Kuri")                                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(contexte, "contexte")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(ContexteProgramme, "ContexteProgramme")                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoType, "InfoType")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeEnum, "InfoTypeÉnum")                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeStructure, "InfoTypeStructure")                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeUnion, "InfoTypeUnion")                             \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeMembreStructure, "InfoTypeMembreStructure")         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeEntier, "InfoTypeEntier")                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeTableau, "InfoTypeTableau")                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypePointeur, "InfoTypePointeur")                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeFonction, "InfoTypeFonction")                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoTypeOpaque, "InfoTypeOpaque")                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(PositionCodeSource, "PositionCodeSource")                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoFonctionTraceAppel, "InfoFonctionTraceAppel")           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(TraceAppel, "TraceAppel")                                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(BaseAllocatrice, "BaseAllocatrice")                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(InfoAppelTraceAppel, "InfoAppelTraceAppel")                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(StockageTemporaire, "StockageTemporaire")                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique, "panique")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique_hors_memoire, "panique_hors_mémoire")               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique_depassement_limites_tableau,                        \
                                      "panique_dépassement_limites_tableau")                      \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique_depassement_limites_chaine,                         \
                                      "panique_dépassement_limites_chaine")                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique_membre_union, "panique_membre_union")               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(panique_erreur_non_geree, "panique_erreur_non_gérée")       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(__rappel_panique_defaut, "__rappel_panique_défaut")         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(DLS_vers_r32, "DLS_vers_r32")                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(DLS_vers_r64, "DLS_vers_r64")                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(DLS_depuis_r32, "DLS_depuis_r32")                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(DLS_depuis_r64, "DLS_depuis_r64")                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(it, "it")                                                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(index_it, "index_it")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(principale, "principale")                                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(enligne, "enligne")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(horsligne, "horsligne")                                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(nulctx, "nulctx")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(externe, "externe")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(sanstrace, "sanstrace")                                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(interface, "interface")                                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(bibliotheque_dynamique, "bibliothèque_dynamique")           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(bibliotheque_statique, "bibliothèque_statique")             \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(def, "def")                                                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(execute, "exécute")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(chemin, "chemin")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(creation_contexte, "création_contexte")                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(compilatrice, "compilatrice")                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(fonction_test_variadique_externe,                           \
                                      "fonction_test_variadique_externe")                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(test, "test")                                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(assert_, "assert")                                          \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(sansbroyage, "sansbroyage")                                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(racine, "racine")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(malloc_, "malloc")                                          \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(realloc_, "realloc")                                        \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(free_, "free")                                              \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(si, "si")                                                   \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(pointeur, "pointeur")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(corps_texte, "corps_texte")                                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(cuisine, "cuisine")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(opaque, "opaque")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(__point_d_entree_systeme, "__point_d_entree_systeme")       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(taille, "taille")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(capacite, "capacité")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(anonyme, "anonyme")                                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(valeur, "valeur")                                           \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(membre_actif, "membre_actif")                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(info, "info")                                               \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(_0, "0")                                                    \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(_1, "1")                                                    \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(nombre_elements, "nombre_éléments")                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(min, "min")                                                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(max, "max")                                                 \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(valeurs_legales, "valeurs_légales")                         \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(valeurs_illegales, "valeurs_illégales")                     \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(compacte, "compacte")                                       \
    ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(aligne, "aligne")

namespace ID {
#define ENUMERE_IDENTIFIANT_COMMUN_SIMPLE(x, y) extern IdentifiantCode *x;
ENUMERE_IDENTIFIANTS_COMMUNS
#undef ENUMERE_IDENTIFIANT_COMMUN_SIMPLE
}  // namespace ID

void initialise_identifiants(TableIdentifiant &table);

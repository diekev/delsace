/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "statistiques.hh"

#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"

static inline int ratio(double a, double b)
{
    if (b <= 0.0001) {
        return 0;
    }

    return static_cast<int>(a / b);
}

void imprime_stats(Statistiques const &stats, dls::chrono::compte_seconde debut_compilation)
{
    auto const temps_total = debut_compilation.temps();

    auto const temps_tampons = stats.temps_tampons;
    auto const temps_chargement = stats.temps_chargement;
    auto const temps_typage = stats.temps_typage;
    auto const temps_parsage = stats.temps_parsage;
    auto const temps_lexage = stats.temps_lexage;

    auto const temps_scene = temps_tampons + temps_lexage + temps_parsage + temps_chargement +
                             temps_typage + stats.temps_metaprogrammes + stats.temps_ri;

    auto const temps_coulisse = stats.temps_generation_code + stats.temps_fichier_objet +
                                stats.temps_executable;

    auto const temps_aggrege = temps_scene + temps_coulisse;

    auto calc_pourcentage = [&](const double &x, const double &total) {
        if (total == 0.0) {
            return 0.0;
        }
        return (x * 100.0 / total);
    };

    auto const mem_totale = stats.stats_fichiers.totaux.memoire_tampons +
                            stats.stats_fichiers.totaux.memoire_lexemes +
                            stats.stats_arbre.totaux.memoire + stats.memoire_compilatrice +
                            stats.stats_graphe_dependance.totaux.memoire +
                            stats.stats_types.totaux.memoire +
                            stats.stats_operateurs.totaux.memoire + stats.memoire_ri;

    auto memoire_consommee = memoire::consommee();

    auto const nombre_lignes = stats.stats_fichiers.totaux.nombre_lignes;
    auto const lignes_double = static_cast<double>(nombre_lignes);
    auto const debit_lignes = ratio(lignes_double, temps_aggrege);
    auto const debit_lignes_scene = ratio(lignes_double,
                                          (temps_scene - stats.temps_metaprogrammes));
    auto const debit_lignes_coulisse = ratio(lignes_double, temps_coulisse);
    auto const debit_seconde = ratio(static_cast<double>(memoire_consommee), temps_aggrege);

    auto tableau = Tableau({"Nom", "Valeur", "Unité", "Pourcentage"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(3, Alignement::DROITE);

    tableau.ajoute_ligne({"Temps total", formatte_nombre(temps_total * 1000.0), "ms"});
    tableau.ajoute_ligne({"Temps aggrégé", formatte_nombre(temps_aggrege * 1000.0), "ms"});
    tableau.ajoute_ligne({"Nombre de modules", formatte_nombre(stats.nombre_modules), ""});
    tableau.ajoute_ligne({"Nombre de lignes", formatte_nombre(nombre_lignes), ""});
    tableau.ajoute_ligne({"Lignes / seconde", formatte_nombre(debit_lignes), ""});
    tableau.ajoute_ligne({"Lignes / seconde (scène)", formatte_nombre(debit_lignes_scene), ""});
    tableau.ajoute_ligne(
        {"Lignes / seconde (coulisse)", formatte_nombre(debit_lignes_coulisse), ""});
    tableau.ajoute_ligne({"Débit par seconde", formatte_nombre(debit_seconde), "o/s"});

    tableau.ajoute_ligne({"Arbre Syntaxique", "", ""});
    tableau.ajoute_ligne(
        {"- Nombre Identifiants", formatte_nombre(stats.nombre_identifiants), ""});
    tableau.ajoute_ligne(
        {"- Nombre Lexèmes", formatte_nombre(stats.stats_fichiers.totaux.nombre_lexemes), ""});
    tableau.ajoute_ligne(
        {"- Nombre Noeuds", formatte_nombre(stats.stats_arbre.totaux.compte), ""});
    tableau.ajoute_ligne({"- Nombre Noeuds Déps",
                          formatte_nombre(stats.stats_graphe_dependance.totaux.compte),
                          ""});
    tableau.ajoute_ligne(
        {"- Nombre Opérateurs", formatte_nombre(stats.stats_operateurs.totaux.compte), ""});
    tableau.ajoute_ligne({"- Nombre Types", formatte_nombre(stats.stats_types.totaux.compte), ""});

    tableau.ajoute_ligne({"Mémoire", "", ""});
    tableau.ajoute_ligne({"- Suivie", formatte_nombre(mem_totale), "o"});
    tableau.ajoute_ligne({"- Effective", formatte_nombre(memoire_consommee), "o"});
    tableau.ajoute_ligne({"- Arbre", formatte_nombre(stats.stats_arbre.totaux.memoire), "o"});
    tableau.ajoute_ligne({"- Compilatrice", formatte_nombre(stats.memoire_compilatrice), "o"});
    tableau.ajoute_ligne(
        {"- Graphe", formatte_nombre(stats.stats_graphe_dependance.totaux.memoire), "o"});
    tableau.ajoute_ligne(
        {"- Lexèmes", formatte_nombre(stats.stats_fichiers.totaux.memoire_lexemes), "o"});
    tableau.ajoute_ligne({"- MV", formatte_nombre(stats.memoire_mv), "o"});
    tableau.ajoute_ligne({"- Bibliothèques", formatte_nombre(stats.memoire_bibliotheques), "o"});
    tableau.ajoute_ligne(
        {"- Opérateurs", formatte_nombre(stats.stats_operateurs.totaux.memoire), "o"});
    tableau.ajoute_ligne({"- RI", formatte_nombre(stats.memoire_ri), "o"});
    tableau.ajoute_ligne(
        {"- Tampon", formatte_nombre(stats.stats_fichiers.totaux.memoire_tampons), "o"});
    tableau.ajoute_ligne({"- Types", formatte_nombre(stats.stats_types.totaux.memoire), "o"});
    tableau.ajoute_ligne(
        {"Nombre allocations", formatte_nombre(memoire::nombre_allocations()), ""});
    tableau.ajoute_ligne(
        {"Nombre métaprogrammes", formatte_nombre(stats.nombre_metaprogrammes_executes), ""});
    tableau.ajoute_ligne(
        {"Instructions exécutées", formatte_nombre(stats.instructions_executees), ""});

    tableau.ajoute_ligne({"Temps Scène",
                          formatte_nombre(temps_scene * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_scene, temps_total))});
    tableau.ajoute_ligne({"- Chargement",
                          formatte_nombre(temps_chargement * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_chargement, temps_scene))});
    tableau.ajoute_ligne({"- Tampon",
                          formatte_nombre(temps_tampons * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_tampons, temps_scene))});
    tableau.ajoute_ligne({"- Lexage",
                          formatte_nombre(temps_lexage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_lexage, temps_scene))});
    tableau.ajoute_ligne(
        {"- Métaprogrammes",
         formatte_nombre(stats.temps_metaprogrammes * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_metaprogrammes, temps_scene))});
    tableau.ajoute_ligne({"- Syntaxage",
                          formatte_nombre(temps_parsage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_parsage, temps_scene))});
    tableau.ajoute_ligne({"- Typage",
                          formatte_nombre(temps_typage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_typage, temps_scene))});
    tableau.ajoute_ligne({"- RI",
                          formatte_nombre(stats.temps_ri * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(stats.temps_ri, temps_scene))});

    tableau.ajoute_ligne({"Temps Coulisse",
                          formatte_nombre(temps_coulisse * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_coulisse, temps_total))});
    tableau.ajoute_ligne(
        {"- Génération Code",
         formatte_nombre(stats.temps_generation_code * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_generation_code, temps_coulisse))});
    tableau.ajoute_ligne(
        {"- Fichier Objet",
         formatte_nombre(stats.temps_fichier_objet * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_fichier_objet, temps_coulisse))});
    tableau.ajoute_ligne(
        {"- Exécutable",
         formatte_nombre(stats.temps_executable * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_executable, temps_coulisse))});

    imprime_tableau(tableau);

    return;
}

static void imprime_stats_tableau(EntreesStats<EntreeNombreMemoire> const &stats)
{
    std::sort(stats.entrees.begin(),
              stats.entrees.end(),
              [](const EntreeNombreMemoire &a, const EntreeNombreMemoire &b) {
                  return a.memoire > b.memoire;
              });

    auto tableau = Tableau({"Nom", "Compte", "Mémoire"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);

    POUR (stats.entrees) {
        tableau.ajoute_ligne(
            {dls::chaine(it.nom), formatte_nombre(it.compte), formatte_nombre(it.memoire)});
    }

    tableau.ajoute_ligne(
        {"", formatte_nombre(stats.totaux.compte), formatte_nombre(stats.totaux.memoire)});

    imprime_tableau(tableau);
}

static void imprime_stats_fichier(EntreesStats<EntreeFichier> const &stats)
{
    std::sort(stats.entrees.begin(),
              stats.entrees.end(),
              [](const EntreeFichier &a, const EntreeFichier &b) {
                  return a.nombre_lignes > b.nombre_lignes;
              });

    auto tableau = Tableau({"Nom", "Lignes", "Mémoire", "Lexèmes", "Mémoire Lexèmes"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);
    tableau.alignement(3, Alignement::DROITE);
    tableau.alignement(4, Alignement::DROITE);

    POUR (stats.entrees) {
        tableau.ajoute_ligne({dls::chaine(it.nom),
                              formatte_nombre(it.nombre_lignes),
                              formatte_nombre(it.memoire_tampons),
                              formatte_nombre(it.nombre_lexemes),
                              formatte_nombre(it.memoire_lexemes)});
    }

    tableau.ajoute_ligne({"",
                          formatte_nombre(stats.totaux.nombre_lignes),
                          formatte_nombre(stats.totaux.memoire_tampons),
                          formatte_nombre(stats.totaux.nombre_lexemes),
                          formatte_nombre(stats.totaux.memoire_lexemes)});

    imprime_tableau(tableau);
}

static void imprime_stats_tableaux(EntreesStats<EntreeTailleTableau> const &stats)
{
    std::sort(stats.entrees.begin(),
              stats.entrees.end(),
              [](const EntreeTailleTableau &a, const EntreeTailleTableau &b) {
                  return a.taille_max > b.taille_max;
              });

    auto tableau = Tableau({"Nom", "Taille Minimum", "Mode", "Taille Maximum"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);

    POUR (stats.entrees) {
        std::sort(it.valeurs.begin(), it.valeurs.end());

        auto mode = int64_t(0);
        if (!it.valeurs.est_vide()) {
            auto valeur_courante = it.valeurs[0];
            auto nombre_valeurs_courante = 1;
            auto nombre_valeurs_mode = 0;
            for (int64_t i = 1; i < it.valeurs.taille(); i++) {
                auto v = it.valeurs[i];

                if (v != valeur_courante) {
                    if (nombre_valeurs_courante > nombre_valeurs_mode) {
                        nombre_valeurs_mode = nombre_valeurs_courante;
                        mode = valeur_courante;
                    }

                    valeur_courante = v;
                    nombre_valeurs_courante = 0;
                }

                nombre_valeurs_courante += 1;
            }
        }

        tableau.ajoute_ligne({dls::chaine(it.nom),
                              formatte_nombre(it.taille_min),
                              formatte_nombre(mode),
                              formatte_nombre(it.taille_max)});
    }

    imprime_tableau(tableau);
}

void imprime_stats_detaillee(Statistiques const &stats)
{
    std::cout << "Arbre Syntaxique :\n";
    imprime_stats_tableau(stats.stats_arbre);
    std::cout << "Graphe Dépendance :\n";
    imprime_stats_tableau(stats.stats_graphe_dependance);
    std::cout << "RI :\n";
    imprime_stats_tableau(stats.stats_ri);
    std::cout << "Operateurs :\n";
    imprime_stats_tableau(stats.stats_operateurs);
    std::cout << "Types :\n";
    imprime_stats_tableau(stats.stats_types);
    std::cout << "Fichiers :\n";
    imprime_stats_fichier(stats.stats_fichiers);
    std::cout << "Tableaux :\n";
    imprime_stats_tableaux(stats.stats_tableaux);
}

static void imprime_stats_temps(EntreesStats<EntreeTemps> const &stats)
{
    std::sort(stats.entrees.begin(),
              stats.entrees.end(),
              [](const EntreeTemps &a, const EntreeTemps &b) { return a.temps > b.temps; });

    auto tableau = Tableau({"Nom", "Temps"});
    tableau.alignement(1, Alignement::DROITE);

    POUR (stats.entrees) {
        tableau.ajoute_ligne({dls::chaine(it.nom), formatte_nombre(it.temps)});
    }

    std::cout << stats.nom << " :\n";
    imprime_tableau(tableau);
}

void StatistiquesTypage::imprime_stats()
{
    imprime_stats_temps(validation_decl);
    imprime_stats_temps(validation_appel);
    imprime_stats_temps(ref_decl);
    imprime_stats_temps(operateurs_unaire);
    imprime_stats_temps(operateurs_binaire);
    imprime_stats_temps(entetes_fonctions);
    imprime_stats_temps(corps_fonctions);
    imprime_stats_temps(enumerations);
    imprime_stats_temps(structures);
    imprime_stats_temps(assignations);
    imprime_stats_temps(finalisation);
}

void StatistiquesGestion::imprime_stats()
{
    imprime_stats_temps(stats);
}

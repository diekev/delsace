/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "statistiques.hh"

#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"

#include "structures/enchaineuse.hh"

static inline int ratio(double a, double b)
{
    if (b <= 0.0001) {
        return 0;
    }

    return static_cast<int>(a / b);
}

void imprime_stats(Statistiques const &stats, dls::chrono::compte_seconde début_compilation)
{
    auto const temps_total = début_compilation.temps();

    auto const temps_tampons = stats.temps_tampons;
    auto const temps_chargement = stats.temps_chargement;
    auto const temps_typage = stats.temps_typage;
    auto const temps_parsage = stats.temps_parsage;
    auto const temps_lexage = stats.temps_lexage;

    auto const temps_scène = temps_tampons + temps_lexage + temps_parsage + temps_chargement +
                             temps_typage + stats.temps_métaprogrammes + stats.temps_ri;

    auto const temps_coulisse = stats.temps_génération_code + stats.temps_fichier_objet +
                                stats.temps_exécutable;

    auto const temps_aggrégé = temps_scène + temps_coulisse;

    auto calc_pourcentage = [&](const double &x, const double &total) {
        if (total == 0.0) {
            return 0.0;
        }
        return (x * 100.0 / total);
    };

    auto const &infos_mémoire_utilisée = stats.donne_mémoire_utilisée_pour_impression();

    auto mémoire_suivie = 0l;
    POUR (infos_mémoire_utilisée) {
        mémoire_suivie += it.quantité;
    }

    auto mémoire_consommee = memoire::consommee();

    auto const nombre_lignes = stats.stats_fichiers.totaux.nombre_lignes;
    auto const lignes_double = static_cast<double>(nombre_lignes);
    auto const débit_lignes = ratio(lignes_double, temps_aggrégé);
    auto const débit_lignes_scène = ratio(lignes_double,
                                          (temps_scène - stats.temps_métaprogrammes));
    auto const débit_lignes_coulisse = ratio(lignes_double, temps_coulisse);
    auto const débit_seconde = ratio(static_cast<double>(mémoire_consommee), temps_aggrégé);

    auto tableau = Tableau({"Nom", "Valeur", "Unité", "Pourcentage"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(3, Alignement::DROITE);

    tableau.ajoute_ligne({"Temps total", formatte_nombre(temps_total * 1000.0), "ms"});
    tableau.ajoute_ligne({"Temps aggrégé", formatte_nombre(temps_aggrégé * 1000.0), "ms"});
    tableau.ajoute_ligne({"Nombre de modules", formatte_nombre(stats.nombre_modules), ""});
    tableau.ajoute_ligne({"Nombre de lignes", formatte_nombre(nombre_lignes), ""});
    tableau.ajoute_ligne({"Lignes / seconde", formatte_nombre(débit_lignes), ""});
    tableau.ajoute_ligne({"Lignes / seconde (scène)", formatte_nombre(débit_lignes_scène), ""});
    tableau.ajoute_ligne(
        {"Lignes / seconde (coulisse)", formatte_nombre(débit_lignes_coulisse), ""});
    tableau.ajoute_ligne({"Débit par seconde", formatte_nombre(débit_seconde), "o/s"});

    tableau.ajoute_ligne({"Arbre Syntaxique", "", ""});
    tableau.ajoute_ligne(
        {"- Nombre Identifiants", formatte_nombre(stats.nombre_identifiants), ""});
    tableau.ajoute_ligne(
        {"- Nombre Lexèmes", formatte_nombre(stats.stats_fichiers.totaux.nombre_lexèmes), ""});
    tableau.ajoute_ligne(
        {"- Nombre Noeuds", formatte_nombre(stats.stats_arbre.totaux.compte), ""});
    tableau.ajoute_ligne({"- Nombre Noeuds Déps",
                          formatte_nombre(stats.stats_graphe_dependance.totaux.compte),
                          ""});
    tableau.ajoute_ligne(
        {"- Nombre Opérateurs", formatte_nombre(stats.stats_opérateurs.totaux.compte), ""});

    tableau.ajoute_ligne({"Mémoire", "", ""});
    tableau.ajoute_ligne({"- Suivie", formatte_nombre(mémoire_suivie), "o"});
    tableau.ajoute_ligne({"- Effective", formatte_nombre(mémoire_consommee), "o"});

    POUR (infos_mémoire_utilisée) {
        auto label = enchaine("- ", it.catégorie);
        tableau.ajoute_ligne(
            {dls::chaine(label.pointeur(), label.taille()),
             formatte_nombre(it.quantité),
             "o",
             formatte_nombre(calc_pourcentage(double(it.quantité), double(mémoire_consommee)))});
    }

    tableau.ajoute_ligne(
        {"Nombre allocations", formatte_nombre(memoire::nombre_allocations()), ""});
    tableau.ajoute_ligne(
        {"Nombre métaprogrammes", formatte_nombre(stats.nombre_métaprogrammes_exécutés), ""});
    tableau.ajoute_ligne(
        {"Instructions exécutées", formatte_nombre(stats.instructions_exécutées), ""});

    tableau.ajoute_ligne({"Temps Scène",
                          formatte_nombre(temps_scène * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_scène, temps_total))});
    tableau.ajoute_ligne({"- Chargement",
                          formatte_nombre(temps_chargement * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_chargement, temps_scène))});
    tableau.ajoute_ligne({"- Tampon",
                          formatte_nombre(temps_tampons * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_tampons, temps_scène))});
    tableau.ajoute_ligne({"- Lexage",
                          formatte_nombre(temps_lexage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_lexage, temps_scène))});
    tableau.ajoute_ligne(
        {"- Métaprogrammes",
         formatte_nombre(stats.temps_métaprogrammes * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_métaprogrammes, temps_scène))});
    tableau.ajoute_ligne({"- Syntaxage",
                          formatte_nombre(temps_parsage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_parsage, temps_scène))});
    tableau.ajoute_ligne({"- Typage",
                          formatte_nombre(temps_typage * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_typage, temps_scène))});
    tableau.ajoute_ligne({"- RI",
                          formatte_nombre(stats.temps_ri * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(stats.temps_ri, temps_scène))});

    tableau.ajoute_ligne({"Temps Coulisse",
                          formatte_nombre(temps_coulisse * 1000.0),
                          "ms",
                          formatte_nombre(calc_pourcentage(temps_coulisse, temps_total))});
    tableau.ajoute_ligne(
        {"- Génération Code",
         formatte_nombre(stats.temps_génération_code * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_génération_code, temps_coulisse))});
    tableau.ajoute_ligne(
        {"- Fichier Objet",
         formatte_nombre(stats.temps_fichier_objet * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_fichier_objet, temps_coulisse))});
    tableau.ajoute_ligne(
        {"- Exécutable",
         formatte_nombre(stats.temps_exécutable * 1000.0),
         "ms",
         formatte_nombre(calc_pourcentage(stats.temps_exécutable, temps_coulisse))});

    imprime_tableau(tableau);

    return;
}

static void imprime_stats_tableau(EntréesStats<EntréeNombreMémoire> const &stats)
{
    std::sort(stats.entrées.begin(),
              stats.entrées.end(),
              [](const EntréeNombreMémoire &a, const EntréeNombreMémoire &b) {
                  return a.mémoire > b.mémoire;
              });

    auto tableau = Tableau({"Nom", "Compte", "Mémoire"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);

    POUR (stats.entrées) {
        if (it.compte == 0) {
            continue;
        }

        tableau.ajoute_ligne({dls::chaine(it.nom.pointeur(), it.nom.taille()),
                              formatte_nombre(it.compte),
                              formatte_nombre(it.mémoire)});
    }

    tableau.ajoute_ligne(
        {"", formatte_nombre(stats.totaux.compte), formatte_nombre(stats.totaux.mémoire)});

    imprime_tableau(tableau);
}

static void imprime_stats_fichier(EntréesStats<EntréeFichier> const &stats)
{
    std::sort(stats.entrées.begin(),
              stats.entrées.end(),
              [](const EntréeFichier &a, const EntréeFichier &b) {
                  return a.nombre_lignes > b.nombre_lignes;
              });

    auto tableau = Tableau({"Nom", "Lignes", "Mémoire", "Lexèmes", "Mémoire Lexèmes"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);
    tableau.alignement(3, Alignement::DROITE);
    tableau.alignement(4, Alignement::DROITE);

    POUR (stats.entrées) {
        tableau.ajoute_ligne({dls::chaine(it.nom.pointeur(), it.nom.taille()),
                              formatte_nombre(it.nombre_lignes),
                              formatte_nombre(it.mémoire_tampons),
                              formatte_nombre(it.nombre_lexèmes),
                              formatte_nombre(it.mémoire_lexèmes)});
    }

    tableau.ajoute_ligne({"",
                          formatte_nombre(stats.totaux.nombre_lignes),
                          formatte_nombre(stats.totaux.mémoire_tampons),
                          formatte_nombre(stats.totaux.nombre_lexèmes),
                          formatte_nombre(stats.totaux.mémoire_lexèmes)});

    imprime_tableau(tableau);
}

static void imprime_stats_tableaux(EntréesStats<EntréeTailleTableau> const &stats)
{
    std::sort(stats.entrées.begin(),
              stats.entrées.end(),
              [](const EntréeTailleTableau &a, const EntréeTailleTableau &b) {
                  return a.taille_max > b.taille_max;
              });

    auto tableau = Tableau({"Nom", "Taille Minimum", "Mode", "Taille Maximum"});
    tableau.alignement(1, Alignement::DROITE);
    tableau.alignement(2, Alignement::DROITE);

    POUR (stats.entrées) {
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

        if (it.taille_max == 0) {
            continue;
        }

        tableau.ajoute_ligne({dls::chaine(it.nom.pointeur(), it.nom.taille()),
                              formatte_nombre(it.taille_min),
                              formatte_nombre(mode),
                              formatte_nombre(it.taille_max)});
    }

    imprime_tableau(tableau);
}

void imprime_stats_détaillées(Statistiques const &stats)
{
    std::cout << "Arbre Syntaxique :\n";
    imprime_stats_tableau(stats.stats_arbre);
    std::cout << "Graphe Dépendance :\n";
    imprime_stats_tableau(stats.stats_graphe_dependance);
    std::cout << "RI :\n";
    imprime_stats_tableau(stats.stats_ri);
    std::cout << "Operateurs :\n";
    imprime_stats_tableau(stats.stats_opérateurs);
    std::cout << "Fichiers :\n";
    imprime_stats_fichier(stats.stats_fichiers);
    std::cout << "Tableaux :\n";
    imprime_stats_tableaux(stats.stats_tableaux);
}

static void imprime_stats_temps(EntréesStats<EntréeTemps> const &stats)
{
    std::sort(stats.entrées.begin(),
              stats.entrées.end(),
              [](const EntréeTemps &a, const EntréeTemps &b) { return a.temps > b.temps; });

    auto tableau = Tableau({"Nom", "Temps"});
    tableau.alignement(1, Alignement::DROITE);

    POUR (stats.entrées) {
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
    imprime_stats_temps(opérateurs_unaire);
    imprime_stats_temps(opérateurs_binaire);
    imprime_stats_temps(entêtes_fonctions);
    imprime_stats_temps(corps_fonctions);
    imprime_stats_temps(énumérations);
    imprime_stats_temps(structures);
    imprime_stats_temps(assignations);
    imprime_stats_temps(finalisation);
}

void StatistiquesGestion::imprime_stats()
{
    imprime_stats_temps(stats);
}

const kuri::tableau<MémoireUtilisée> &Statistiques::donne_mémoire_utilisée_pour_impression() const
{
    ajoute_mémoire_utilisée("Graphe", stats_graphe_dependance.totaux.mémoire);
    ajoute_mémoire_utilisée("Opérateurs", stats_opérateurs.totaux.mémoire);
    ajoute_mémoire_utilisée("Arbre", stats_arbre.totaux.mémoire);
    ajoute_mémoire_utilisée("Lexèmes", stats_fichiers.totaux.mémoire_lexèmes);
    ajoute_mémoire_utilisée("Tampon", stats_fichiers.totaux.mémoire_tampons);

    std::sort(m_mémoires_utilisées.debut(), m_mémoires_utilisées.fin(), [](auto &a, auto &b) {
        return a.catégorie < b.catégorie;
    });

    return m_mémoires_utilisées;
}

void Statistiques::ajoute_mémoire_utilisée(kuri::chaine_statique catégorie, int64_t quantité) const
{
    POUR (m_mémoires_utilisées) {
        if (it.catégorie == catégorie) {
            it.quantité += quantité;
            return;
        }
    }

    m_mémoires_utilisées.ajoute({catégorie, quantité});
}

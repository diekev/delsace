/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "modules.hh"

#include <fstream>

#include "biblinternes/langage/erreur.hh"
#include "biblinternes/outils/numerique.hh"

#include "structures/enchaineuse.hh"

#include "statistiques/statistiques.hh"

#include "lexeuse.hh"

/* ************************************************************************** */

bool Fichier::importe_module(IdentifiantCode *nom_module) const
{
    bool importe = false;
    pour_chaque_element(modules_importés, [nom_module, &importe](Module *module_) {
        if (module_->nom() == nom_module) {
            importe = true;
            return kuri::DécisionItération::Arrête;
        }

        return kuri::DécisionItération::Continue;
    });
    return importe;
}

void Module::ajoute_fichier(Fichier *fichier)
{
    fichiers.ajoute(fichier);
    fichiers_sont_sales = true;
}

Module *SystèmeModule::trouve_ou_crée_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
    auto chemin_normalisé = kuri::chaine(chemin);

    if (chemin_normalisé.taille() > 0 && chemin_normalisé[chemin_normalisé.taille() - 1] != '/') {
        chemin_normalisé = enchaine(chemin_normalisé, '/');
    }

    POUR_TABLEAU_PAGE (modules) {
        if (it.nom() != nom) {
            continue;
        }

        if (it.chemin() == chemin_normalisé) {
            return &it;
        }
    }

    return crée_module(nom, chemin_normalisé);
}

Module *SystèmeModule::crée_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
    auto dm = modules.ajoute_element(chemin);
    dm->nom_ = nom;
    return dm;
}

Module *SystèmeModule::module(const IdentifiantCode *nom) const
{
    POUR_TABLEAU_PAGE (modules) {
        if (it.nom() == nom) {
            return const_cast<Module *>(&it);
        }
    }

    return nullptr;
}

RésultatFichier SystèmeModule::trouve_ou_crée_fichier(Module *module,
                                                      kuri::chaine_statique nom,
                                                      kuri::chaine_statique chemin)
{
    auto fichier = table_fichiers.valeur_ou(chemin, nullptr);

    if (fichier) {
        return FichierExistant(fichier);
    }

    return crée_fichier(module, nom, chemin);
}

FichierNeuf SystèmeModule::crée_fichier(Module *module,
                                        kuri::chaine_statique nom,
                                        kuri::chaine_statique chemin)
{
    auto df = fichiers.ajoute_element();
    df->nom_ = nom;
    df->chemin_ = chemin;
    df->id_ = fichiers.taille() - 1;
    df->module = module;

    module->ajoute_fichier(df);

    table_fichiers.insère(df->chemin(), df);

    return FichierNeuf(df);
}

void SystèmeModule::rassemble_stats(Statistiques &stats) const
{
    stats.nombre_modules = modules.taille();

    auto &stats_gapillage = stats.stats_gaspillage;
    auto gaspillage_lexèmes = 0;

    auto &stats_fichiers = stats.stats_fichiers;
    POUR_TABLEAU_PAGE (fichiers) {
        auto entrée = EntréeFichier();
        entrée.chemin = it.chemin();
        entrée.nom = it.nom();
        entrée.nombre_lignes = it.tampon().nombre_lignes();
        entrée.mémoire_tampons = it.tampon().taille_donnees();
        entrée.mémoire_lexèmes = it.lexèmes.taille_mémoire();
        entrée.nombre_lexèmes = it.lexèmes.taille();
        entrée.temps_chargement = it.temps_chargement;
        entrée.temps_tampon = it.temps_tampon;
        entrée.temps_lexage = it.temps_découpage;
        entrée.temps_parsage = it.temps_analyse;

        gaspillage_lexèmes += it.lexèmes.gaspillage_mémoire();

        stats_fichiers.fusionne_entrée(entrée);
    }

    stats_gapillage.fusionne_entrée({"Lexèmes", 1, gaspillage_lexèmes});
}

int64_t SystèmeModule::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += modules.memoire_utilisee();
    résultat += fichiers.memoire_utilisee();

    POUR_TABLEAU_PAGE (fichiers) {
        résultat += it.nom().taille();
        résultat += it.chemin().taille();
        résultat += it.tampon().chaine().taille();
        // les autres membres sont gérés dans rassemble_statistiques()
        if (!it.modules_importés.est_stocké_dans_classe()) {
            résultat += it.modules_importés.mémoire_utilisée();
        }
    }

    POUR_TABLEAU_PAGE (modules) {
        résultat += it.chemin().taille();
        résultat += it.fichiers.taille_mémoire();
    }

    return résultat;
}

Fichier *SystèmeModule::fichier(kuri::chaine_statique chemin) const
{
    POUR_TABLEAU_PAGE (fichiers) {
        if (it.chemin() == chemin) {
            return const_cast<Fichier *>(&it);
        }
    }

    return nullptr;
}

static dls::vue_chaine sous_chaine(dls::vue_chaine chaine, int debut, int fin)
{
    return {&chaine[debut], fin - debut};
}

void imprime_ligne_avec_message(Enchaineuse &enchaineuse,
                                SiteSource site,
                                kuri::chaine_statique message)
{
    auto const fichier = site.fichier;

    if (fichier == nullptr) {
        /* Il est possible que le fichier soit nul dans certains cas, par exemple quand une erreur
         * est rapportée sans site. */
        return;
    }

    auto const index_ligne = site.index_ligne;
    auto const numéro_ligne = site.index_ligne + fichier->décalage_fichier + 1;
    auto const index_colonne = site.index_colonne;
    auto const texte_ligne = fichier->tampon()[index_ligne];
    auto chemin = fichier->source == SourceFichier::CHAINE_AJOUTÉE ? ".chaine_ajoutées" :
                                                                     fichier->chemin();

    enchaineuse << chemin << ':' << numéro_ligne;

    if (index_colonne != -1) {
        enchaineuse << ':' << index_colonne;
    }

    enchaineuse << " : " << message << "\n";

    for (auto i = 0; i < 5 - dls::num::nombre_de_chiffres(numéro_ligne); ++i) {
        enchaineuse << ' ';
    }

    enchaineuse << numéro_ligne << " | " << texte_ligne;

    /* Le manque de nouvelle ligne peut arriver en fin de fichier. */
    if (texte_ligne.taille() != 0 && texte_ligne[texte_ligne.taille() - 1] != '\n') {
        enchaineuse << '\n';
    }

    if (index_colonne != -1) {
        enchaineuse << "      | ";

        if (site.index_colonne_min != -1 && site.index_colonne_min != site.index_colonne) {
            lng::erreur::imprime_caractere_vide(enchaineuse, site.index_colonne_min, texte_ligne);
            lng::erreur::imprime_tilde(
                enchaineuse, sous_chaine(texte_ligne, site.index_colonne_min, index_colonne + 1));
        }
        else {
            lng::erreur::imprime_caractere_vide(enchaineuse, index_colonne, texte_ligne);
        }

        enchaineuse << '^';
        lng::erreur::imprime_tilde(
            enchaineuse, sous_chaine(texte_ligne, index_colonne, site.index_colonne_max));
        enchaineuse << '\n';
    }
}

dls::chaine charge_contenu_fichier(const dls::chaine &chemin)
{
    std::ifstream fichier;
    fichier.open(chemin.c_str());

    if (!fichier.is_open()) {
        return "";
    }

    return dls::chaine(std::istreambuf_iterator<char>(fichier), std::istreambuf_iterator<char>());
}

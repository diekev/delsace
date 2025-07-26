/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "modules.hh"

#include <fstream>

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "statistiques/statistiques.hh"

#include "utilitaires/divers.hh"
#include "utilitaires/log.hh"

#include "lexeuse.hh"

#define NE_CHARGE_QUE_FICHIER_MODULE

/* ************************************************************************** */

bool Module::importe_module(IdentifiantCode *nom_module) const
{
    bool importe = false;
    pour_chaque_élément(modules_importés, [nom_module, &importe](ModuleImporté const &module_) {
        if (module_.module->nom() == nom_module) {
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

Module *SystèmeModule::initialise_module_kuri(kuri::chaine_statique chemin_racine_modules,
                                              bool doit_importer_kuri_implicitement)
{
    racine_modules_kuri = chemin_racine_modules;

    auto chemin_module = racine_modules_kuri / kuri::chemin_systeme("Kuri/");
    auto chemin_fichier_module_kuri = chemin_module / "module.kuri";

    if (!kuri::chemin_systeme::existe(chemin_fichier_module_kuri)) {
        dbg() << "Kuri/module.kuri n'existe pas";
        return nullptr;
    }

    auto module = crée_module(ID::Kuri, chemin_module);
    module->importé = true;

#ifndef NE_CHARGE_QUE_FICHIER_MODULE
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin_module);
    POUR (fichiers) {
        crée_fichier(module, it.nom_fichier_sans_extension(), it);
    }
#else
    crée_fichier(module, "module", chemin_fichier_module_kuri);
#endif

    auto chemin_fichier_constantes_kuri = chemin_module / "constantes.kuri";

    auto fichier_neuf = crée_fichier(module, "constantes", chemin_fichier_constantes_kuri);
    auto fichier = static_cast<Fichier *>(fichier_neuf);

    const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
    fichier->charge_tampon(TamponSource(source));

    fichier->fut_chargé = true;

    module_kuri = module;
    importe_kuri = doit_importer_kuri_implicitement;

    return module;
}

Module *SystèmeModule::crée_module_fichier_racine_compilation(kuri::chaine_statique dossier,
                                                              kuri::chaine_statique chemin_fichier)
{
    auto chemin_normalisé = kuri::chaine(dossier);
    if (chemin_normalisé.taille() > 0 && chemin_normalisé[chemin_normalisé.taille() - 1] != '/') {
        chemin_normalisé = enchaine(chemin_normalisé, '/');
    }
    auto module = crée_module(ID::chaine_vide, chemin_normalisé);
    module->importé = true;
    auto chemin_fichier_ = kuri::chemin_systeme(chemin_fichier);
    crée_fichier(module, chemin_fichier_.nom_fichier_sans_extension(), chemin_fichier_);
    return module;
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

// @duplication compilatrice.cc
std::optional<kuri::chemin_systeme> SystèmeModule::détermine_chemin_dossier_module(
    Fichier const *fichier, kuri::chaine_statique nom, InfoRequêteModule *info)
{
    auto const chemin_est_relatif_au_fichier = nom.taille() > 2 && nom.sous_chaine(0, 2) == "..";

    auto chemin = kuri::chemin_systeme(nom);

    if (!chemin_est_relatif_au_fichier) {
        /* Vérifions si le chemin est dans le dossier courant. */
        if (kuri::chemin_systeme::existe(chemin)) {
            return chemin;
        }

        /* Essayons dans la racine des modules. */
        auto chemin_dans_racine = racine_modules_kuri / chemin;
        if (kuri::chemin_systeme::existe(chemin_dans_racine)) {
            return chemin_dans_racine;
        }

        info->chemins_testés.ajoute(chemin);
        info->chemins_testés.ajoute(chemin_dans_racine);
    }

    /* Essayons dans le dossier du fichier. */
    auto chemin_du_module = fichier->module->chemin();
    auto chemin_possible = kuri::chemin_systeme(chemin_du_module) / chemin;
    if (kuri::chemin_systeme::existe(chemin_possible)) {
        return chemin_possible;
    }

    info->chemins_testés.ajoute(chemin_possible);
    info->état = InfoRequêteModule::État::CHEMIN_INEXISTANT;

    return {};
}

InfoRequêteModule SystèmeModule::trouve_ou_crée_module(
    kuri::Synchrone<TableIdentifiant> &table_identifiants,
    Fichier const *fichier,
    kuri::chaine_statique nom_module)
{
    auto résultat = InfoRequêteModule{};

    auto opt_chemin = détermine_chemin_dossier_module(fichier, nom_module, &résultat);
    if (!opt_chemin.has_value()) {
        return résultat;
    }

    auto chemin = opt_chemin.value();

    if (!kuri::chemin_systeme::est_dossier(chemin)) {
        résultat.état = InfoRequêteModule::État::PAS_UN_DOSSIER;
        return résultat;
    }

    /* Trouve le chemin absolu du module cannonique pour supprimer les "../../". */
    auto chemin_absolu = kuri::chemin_systeme::canonique_absolu(chemin);
    auto nom_dossier = chemin_absolu.nom_fichier();

    /* N'importe le module que s'il possède un fichier "module.kuri". */
    auto chemin_fichier_module = chemin_absolu / "module.kuri";
    if (!kuri::chemin_systeme::existe(chemin_fichier_module)) {
        résultat.état = InfoRequêteModule::État::PAS_DE_FICHIER_MODULE_KURI;
        résultat.chemin = chemin_absolu;
        return résultat;
    }

    auto module = trouve_ou_crée_module(
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier), chemin_absolu);
    résultat.module = module;
    résultat.état = InfoRequêteModule::État::TROUVÉ;

    if (module->fichiers.taille() != 0) {
        return résultat;
    }

#ifndef NE_CHARGE_QUE_FICHIER_MODULE
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin_absolu);
    POUR (fichiers) {
        crée_fichier(module, it.nom_fichier_sans_extension(), it);
    }
#else
    crée_fichier(module, "module", chemin_fichier_module);
#endif

    return résultat;
}

Module *SystèmeModule::crée_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
    auto dm = modules.ajoute_élément(chemin);
    dm->nom_ = nom;
    if (importe_kuri) {
        dm->modules_importés.insère({module_kuri, true});
    }
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
    auto df = fichiers.ajoute_élément();
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
    résultat += modules.mémoire_utilisée();
    résultat += fichiers.mémoire_utilisée();

    POUR_TABLEAU_PAGE (fichiers) {
        résultat += it.nom().taille();
        résultat += it.chemin().taille();
        résultat += it.tampon().chaine().taille();
    }

    POUR_TABLEAU_PAGE (modules) {
        résultat += it.chemin().taille();
        résultat += it.fichiers.taille_mémoire();
        // les autres membres sont gérés dans rassemble_statistiques()
        if (!it.modules_importés.est_stocké_dans_classe()) {
            résultat += it.modules_importés.mémoire_utilisée();
        }
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

void imprime_ligne_avec_message(Enchaineuse &enchaineuse,
                                kuri::chaine_statique message,
                                kuri::chaine_statique chemin_fichier,
                                kuri::chaine_statique texte_ligne,
                                int numéro_ligne,
                                int index_colonne,
                                int index_colonne_début,
                                int index_colonne_fin)
{

    enchaineuse << chemin_fichier << ':' << numéro_ligne;

    if (index_colonne != -1) {
        enchaineuse << ':' << index_colonne;
    }

    enchaineuse << " : " << message << "\n";

    for (auto i = 0; i < 5 - nombre_de_chiffres(numéro_ligne); ++i) {
        enchaineuse << ' ';
    }

    enchaineuse << numéro_ligne << " | " << texte_ligne;

    /* Le manque de nouvelle ligne peut arriver en fin de fichier. */
    if (texte_ligne.taille() != 0 && texte_ligne[texte_ligne.taille() - 1] != '\n') {
        enchaineuse << '\n';
    }

    if (index_colonne != -1) {
        enchaineuse << "      | ";

        if (index_colonne_début != -1 && index_colonne_début != index_colonne) {
            enchaineuse.imprime_caractère_vide(index_colonne_début, texte_ligne);
            enchaineuse.imprime_tilde(
                texte_ligne.sous_chaine(index_colonne_début, index_colonne + 1));
        }
        else {
            enchaineuse.imprime_caractère_vide(index_colonne, texte_ligne);
        }

        enchaineuse << '^';
        enchaineuse.imprime_tilde(texte_ligne.sous_chaine(index_colonne, index_colonne_fin));
        enchaineuse << '\n';
    }
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
    auto const numéro_ligne = site.index_ligne + int32_t(fichier->décalage_fichier) + 1;
    auto const texte_ligne = fichier->tampon()[index_ligne];
    auto chemin = fichier->source == SourceFichier::CHAINE_AJOUTÉE ? ".chaine_ajoutées" :
                                                                     fichier->chemin();

    auto texte_ligne_kuri = kuri::chaine_statique(texte_ligne.begin(), texte_ligne.taille());

    imprime_ligne_avec_message(enchaineuse,
                               message,
                               chemin,
                               texte_ligne_kuri,
                               numéro_ligne,
                               site.index_colonne,
                               site.index_colonne_min,
                               site.index_colonne_max);
}

kuri::chaine charge_contenu_fichier(kuri::chaine_statique chemin)
{
    auto chemin_std = vers_std_path(chemin);
    std::ifstream fichier;
    fichier.open(chemin_std.c_str(), std::ios::in | std::ios::binary);

    if (!fichier.is_open()) {
        return "";
    }

    fichier.seekg(0, std::ios_base::end);
    auto taille = fichier.tellg();
    fichier.seekg(0, std::ios_base::beg);

    auto résultat = kuri::chaine();
    résultat.redimensionne(taille);

    fichier.read(résultat.pointeur(), taille);

    return résultat;
}

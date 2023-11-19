/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "metaprogramme.hh"

#include <fstream>
#include <iostream>

#include "statistiques/statistiques.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "programme.hh"
#include "typage.hh"

int DonnéesConstantesExécutions::ajoute_globale(Type *type,
                                                IdentifiantCode *ident,
                                                const Type *pour_info_type)
{
    auto globale = Globale{};
    globale.type = type;
    globale.ident = ident;
    globale.adresse = données_globales.taille();

    if (pour_info_type) {
        globale.adresse_pour_exécution = pour_info_type->info_type;
    }
    else {
        globale.adresse_pour_exécution = nullptr;
    }

    données_globales.redimensionne(données_globales.taille() +
                                   static_cast<int>(type->taille_octet));

    auto ptr = globales.taille();

    globales.ajoute(globale);

    return ptr;
}

void DonnéesConstantesExécutions::rassemble_statistiques(Statistiques &stats) const
{
    auto memoire_mv = 0l;
    memoire_mv += globales.taille_memoire();
    memoire_mv += données_constantes.taille_memoire();
    memoire_mv += données_globales.taille_memoire();
    memoire_mv += patchs_données_constantes.taille_memoire();

    stats.memoire_mv += memoire_mv;
}

MetaProgramme::~MetaProgramme()
{
    POUR (logueuses) {
        if (it) {
            memoire::deloge("Enchaineuse", it);
        }
    }
    memoire::deloge("Programme", programme);
}

Enchaineuse &MetaProgramme::donne_logueuse(TypeLogMétaprogramme type_log)
{
    assert(type_log != TypeLogMétaprogramme::NOMBRE_DE_LOGS);
    auto index_logueuse = static_cast<int>(type_log);
    if (logueuses[index_logueuse] == nullptr) {
        logueuses[index_logueuse] = memoire::loge<Enchaineuse>("Enchaineuse");
    }

    return *logueuses[index_logueuse];
}

void MetaProgramme::vidange_logs_sur_disque()
{
    vidange_log_sur_disque(TypeLogMétaprogramme::APPEL);
    vidange_log_sur_disque(TypeLogMétaprogramme::INSTRUCTION);
    vidange_log_sur_disque(TypeLogMétaprogramme::STAT_INSTRUCTION);
}

static kuri::chaine_statique donne_suffixe_pour_type_log(TypeLogMétaprogramme type_log)
{
    switch (type_log) {
        case TypeLogMétaprogramme::APPEL:
        {
            return "appel";
        }
        case TypeLogMétaprogramme::INSTRUCTION:
        {
            return "flux_instructions";
        }
        case TypeLogMétaprogramme::STAT_INSTRUCTION:
        {
            return "stats_instructions";
        }
        default:
        {
            return "inconnu";
        }
    }
}

void MetaProgramme::vidange_log_sur_disque(TypeLogMétaprogramme type_log)
{
    assert(type_log != TypeLogMétaprogramme::NOMBRE_DE_LOGS);
    auto index_logueuse = static_cast<int>(type_log);
    if (!logueuses[index_logueuse]) {
        return;
    }
    auto &logueuse = logueuses[index_logueuse];
    auto texte = logueuse->chaine();

    auto nom_fichier = enchaine(
        "métaprogramme", this, "_", donne_suffixe_pour_type_log(type_log), ".txt");

    std::cout << "Écriture de log de métaprogramme..." << std::endl;

    auto chemin_fichier_entete = kuri::chemin_systeme::chemin_temporaire(nom_fichier);
    std::ofstream of(vers_std_path(chemin_fichier_entete));
    logueuse->imprime_dans_flux(of);
    of.close();
}

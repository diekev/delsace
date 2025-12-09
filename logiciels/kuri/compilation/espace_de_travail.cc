/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "espace_de_travail.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/lexeuse.hh"

#include "representation_intermediaire/constructrice_ri.hh"

#include "compilatrice.hh"
#include "coulisse.hh"
#include "programme.hh"

#include "statistiques/statistiques.hh"

#include "utilitaires/log.hh"

#define NOMBRE_DE_TACHES(x) nombre_de_tâches[size_t(GenreTâche::x)]

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(Compilatrice &compilatrice,
                                 OptionsDeCompilation opts,
                                 kuri::chaine nom_)
    : nom(nom_), options(opts), typeuse(graphe_dépendance), m_compilatrice(compilatrice)
{
    programme = Programme::crée_pour_espace(this);
    registre_ri = mémoire::loge<RegistreSymboliqueRI>("RegistreSymboliqueRI", typeuse);

    auto ops = opérateurs.verrou_écriture();
    enregistre_opérateurs_basiques(typeuse, *ops);

    m_bloc_racine = compilatrice.gestionnaire_code->crée_bloc_racine(typeuse);

    POUR (nombre_de_tâches) {
        it = 0;
    }

    for (auto i = 0; i < NOMBRE_DE_PhaseCompilation; i++) {
        m_phases[i].id = PhaseCompilation(i);
    }
}

EspaceDeTravail::~EspaceDeTravail()
{
    mémoire::déloge("Programme", programme);
    mémoire::déloge("RegistreSymboliqueRI", registre_ri);
}

Module *EspaceDeTravail::donne_module(const IdentifiantCode *nom_module) const
{
    return sys_module->module(nom_module);
}

const Fichier *EspaceDeTravail::fichier(int64_t indice) const
{
    return sys_module->fichier(indice);
}

Fichier *EspaceDeTravail::fichier(int64_t indice)
{
    return sys_module->fichier(indice);
}

Fichier *EspaceDeTravail::fichier(kuri::chaine_statique chemin) const
{
    return sys_module->fichier(chemin);
}

int64_t EspaceDeTravail::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += programme->mémoire_utilisée();
    résultat += sys_module->mémoire_utilisée();
    résultat += constructeurs_globaux->taille_mémoire();
    résultat += registre_chaines_ri->mémoire_utilisée();
    résultat += métaprogrammes_en_attente_de_crée_contexte.taille_mémoire();
    résultat += registre_annotations.mémoire_utilisée();
    résultat += trie_structs_employées.mémoire_utilisée();
    résultat += trie_types_entrée_sortie.mémoire_utilisée();
    return résultat;
}

void EspaceDeTravail::rassemble_statistiques(Statistiques &stats) const
{
    programme->rassemble_statistiques(stats);
    sys_module->rassemble_stats(stats);
    opérateurs->rassemble_statistiques(stats);
    graphe_dépendance->rassemble_statistiques(stats);
    typeuse.rassemble_statistiques(stats);
    registre_ri->rassemble_statistiques(stats);
    données_constantes_exécutions.rassemble_statistiques(stats);
}

void EspaceDeTravail::tâche_ajoutée(GenreTâche genre_tâche, kuri::Synchrone<Messagère> &messagère)
{
    auto phase = donne_phase();
    auto phase_pour_tâche = donne_phase(genre_tâche);
    phase_pour_tâche->nombre_de_tâches += 1;

    if (phase != phase_pour_tâche) {
        if (phase->id > phase_pour_tâche->id) {
            id_phase += 1;
            change_de_phase(messagère, phase_pour_tâche->id, __func__);
        }
    }
}

void EspaceDeTravail::tâche_terminée(GenreTâche genre_tâche, kuri::Synchrone<Messagère> &messagère)
{
    auto phase_pour_tâche = donne_phase(genre_tâche);
    phase_pour_tâche->nombre_de_tâches -= 1;
    assert_rappel(phase_pour_tâche->nombre_de_tâches >= 0, [&] {
        dbg() << "La phase est " << phase_pour_tâche->id;
        dbg() << "La phase courante est " << m_id_phase_courante;
        dbg() << "La tâche terminée est " << genre_tâche;
    });

    if (phase_pour_tâche->nombre_de_tâches == 0) {
        // À FAIRE(phase) : ici nous pouvons aller trop loin...
        for (auto i = int(m_id_phase_courante) + 1; i < NOMBRE_DE_PhaseCompilation; i++) {
            auto phase = &m_phases[i];

            change_de_phase(messagère, phase->id, __func__);

            if (phase->nombre_de_tâches != 0) {
                break;
            }
        }
    }
}

// nul-geste
#if 0
void EspaceDeTravail::progresse_phase_pour_tâche_terminée(GenreTâche genre_tâche,
                                                          kuri::Synchrone<Messagère> &messagère)
{
    PhaseCompilation nouvelle_phase = m_id_phase_courante;
    switch (genre_tâche) {
        case GenreTâche::CHARGEMENT:
        case GenreTâche::LEXAGE:
        {
            nouvelle_phase = PhaseCompilation::PARSAGE_EN_COURS;
            break;
        }
        case GenreTâche::PARSAGE:
        {
            if (parsage_terminé()) {
                nouvelle_phase = PhaseCompilation::PARSAGE_TERMINÉ;
            }
            break;
        }
        case GenreTâche::TYPAGE:
        {
            if (nombre_de_tâches[size_t(genre_tâche)] == 0 &&
                m_id_phase_courante == PhaseCompilation::PARSAGE_TERMINÉ) {
                nouvelle_phase = PhaseCompilation::TYPAGE_TERMINÉ;

                /* Il est possible que les dernières tâches de typages soient pour des choses qui
                 * n'ont pas de RI, donc avançons jusqu'à GÉNÉRATION_CODE_TERMINÉE. */
                if (nombre_de_tâches[size_t(GenreTâche::GENERATION_RI)] == 0) {
                    /* Notifie pour le changement de phase précédent. */
                    change_de_phase(messagère, nouvelle_phase, __func__);
                    nouvelle_phase = PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE;
                }
            }
            break;
        }
        case GenreTâche::GENERATION_RI:
        case GenreTâche::OPTIMISATION:
        {
            if (nombre_de_tâches[size_t(GenreTâche::GENERATION_RI)] == 0 &&
                nombre_de_tâches[size_t(GenreTâche::OPTIMISATION)] == 0 &&
                m_id_phase_courante == PhaseCompilation::TYPAGE_TERMINÉ) {
                nouvelle_phase = PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE;
            }
            break;
        }
        case GenreTâche::DORS:
        case GenreTâche::COMPILATION_TERMINÉE:
        case GenreTâche::CREATION_FONCTION_INIT_TYPE:
        case GenreTâche::CONVERSION_NOEUD_CODE:
        case GenreTâche::ENVOIE_MESSAGE:
        case GenreTâche::GENERATION_CODE_MACHINE:
        case GenreTâche::LIAISON_PROGRAMME:
        case GenreTâche::EXECUTION:
        case GenreTâche::NOMBRE_ELEMENTS:
        case GenreTâche::CALCULE_TAILLE_TYPE:
        case GenreTâche::SYNTHÉTISATION_OPÉRATEUR:
        {
            break;
        }
    }

    if (nouvelle_phase != m_id_phase_courante) {
        change_de_phase(messagère, nouvelle_phase, __func__);
    }
}

void EspaceDeTravail::regresse_phase_pour_tâche_ajoutée(GenreTâche genre_tâche,
                                                        kuri::Synchrone<Messagère> &messagère)
{
    PhaseCompilation nouvelle_phase = m_id_phase_courante;
    switch (genre_tâche) {
        case GenreTâche::CHARGEMENT:
        case GenreTâche::LEXAGE:
        case GenreTâche::PARSAGE:
        {
            nouvelle_phase = PhaseCompilation::PARSAGE_EN_COURS;
            break;
        }
        case GenreTâche::TYPAGE:
        {
            if (m_id_phase_courante > PhaseCompilation::PARSAGE_TERMINÉ) {
                nouvelle_phase = PhaseCompilation::PARSAGE_TERMINÉ;
            }
            break;
        }
        case GenreTâche::GENERATION_RI:
        case GenreTâche::OPTIMISATION:
        {
            if (m_id_phase_courante > PhaseCompilation::TYPAGE_TERMINÉ) {
                nouvelle_phase = PhaseCompilation::TYPAGE_TERMINÉ;
            }
            break;
        }
        case GenreTâche::GENERATION_CODE_MACHINE:
        {
            if (m_id_phase_courante > PhaseCompilation::APRÈS_GÉNÉRATION_OBJET) {
                nouvelle_phase = PhaseCompilation::APRÈS_GÉNÉRATION_OBJET;
            }
            break;
        }
        case GenreTâche::LIAISON_PROGRAMME:
        {
            if (m_id_phase_courante > PhaseCompilation::APRÈS_LIAISON_EXÉCUTABLE) {
                nouvelle_phase = PhaseCompilation::APRÈS_LIAISON_EXÉCUTABLE;
            }
            break;
        }
        case GenreTâche::DORS:
        case GenreTâche::COMPILATION_TERMINÉE:
        case GenreTâche::CREATION_FONCTION_INIT_TYPE:
        case GenreTâche::CONVERSION_NOEUD_CODE:
        case GenreTâche::ENVOIE_MESSAGE:
        case GenreTâche::EXECUTION:
        case GenreTâche::NOMBRE_ELEMENTS:
        case GenreTâche::CALCULE_TAILLE_TYPE:
        case GenreTâche::SYNTHÉTISATION_OPÉRATEUR:
        {
            break;
        }
    }

    if (nouvelle_phase != m_id_phase_courante) {
        id_phase += 1;
        change_de_phase(messagère, nouvelle_phase, __func__);
    }
}
#endif

// À FAIRE(phase) : comment détecter la possibilité de générer le code final ?
bool EspaceDeTravail::peut_génèrer_code_final() const
{
    if (m_id_phase_courante != PhaseCompilation::GÉNÉRATION_CODE_TERMINÉE) {
        return false;
    }

    if (NOMBRE_DE_TACHES(EXECUTION) == 0) {
        return true;
    }

    if (NOMBRE_DE_TACHES(EXECUTION) == 1 && métaprogramme) {
        return true;
    }

    return false;
}

// nul-geste
#if 0
bool EspaceDeTravail::parsage_terminé() const
{
    return NOMBRE_DE_TACHES(CHARGEMENT) == 0 && NOMBRE_DE_TACHES(LEXAGE) == 0 &&
           NOMBRE_DE_TACHES(PARSAGE) == 0;
}
#endif

Phase *EspaceDeTravail::donne_phase()
{
    return &m_phases[size_t(m_id_phase_courante)];
}

Phase *EspaceDeTravail::donne_phase(GenreTâche genre_tâche)
{
    switch (genre_tâche) {
        case GenreTâche::CHARGEMENT:
        case GenreTâche::LEXAGE:
        case GenreTâche::PARSAGE:
        {
            return &m_phases[size_t(PhaseCompilation::PARSAGE_EN_COURS)];
        }
        case GenreTâche::TYPAGE:
        {
            return &m_phases[size_t(PhaseCompilation::PARSAGE_TERMINÉ)];
        }
        case GenreTâche::GENERATION_RI:
        case GenreTâche::OPTIMISATION:
        {
            return &m_phases[size_t(PhaseCompilation::TYPAGE_TERMINÉ)];
        }
        case GenreTâche::GENERATION_CODE_MACHINE:
        {
            return &m_phases[size_t(PhaseCompilation::APRÈS_GÉNÉRATION_OBJET)];
        }
        case GenreTâche::LIAISON_PROGRAMME:
        {
            return &m_phases[size_t(PhaseCompilation::APRÈS_LIAISON_EXÉCUTABLE)];
        }
        case GenreTâche::DORS:
        case GenreTâche::COMPILATION_TERMINÉE:
        case GenreTâche::CREATION_FONCTION_INIT_TYPE:
        case GenreTâche::CONVERSION_NOEUD_CODE:
        case GenreTâche::ENVOIE_MESSAGE:
        case GenreTâche::EXECUTION:
        case GenreTâche::NOMBRE_ELEMENTS:
        case GenreTâche::CALCULE_TAILLE_TYPE:
        case GenreTâche::SYNTHÉTISATION_OPÉRATEUR:
        {
            break;
        }
    }

    return donne_phase();
}

void EspaceDeTravail::imprime_compte_tâches(std::ostream &os) const
{
    for (int i = 0; i < int(GenreTâche::NOMBRE_ELEMENTS); i++) {
        os << "-- tâche " << GenreTâche(i) << " " << nombre_de_tâches[i].load() << '\n';
    }
}

Message *EspaceDeTravail::change_de_phase(kuri::Synchrone<Messagère> &messagère,
                                          PhaseCompilation nouvelle_phase,
                                          kuri::chaine_statique fonction_appelante)
{
#define IMPRIME_CHANGEMENT_DE_PHASE(nom_espace)                                                   \
    if (nom == nom_espace) {                                                                      \
        dbg() << __func__ << " depuis " << fonction_appelante << " : de " << m_id_phase_courante  \
              << " vers " << nouvelle_phase << ", id " << id_phase;                               \
    }

    IMPRIME_CHANGEMENT_DE_PHASE("Espace 1")

    // if (m_id_phase_courante == PhaseCompilation::COMPILATION_TERMINÉE) {
    //     /* Il est possible qu'un espace ajoute des choses à compiler mais que celui-ci n'utilise
    //      * pas le code. Or, les tâches de typage subséquentes feront regresser sa phase de
    //      * compilation si la compilation est terminée pour celui-ci. Si tel est le cas, la
    //      * compilation sera infinie. Empêchons donc de modifier la phase de compilation de
    //      l'espace
    //      * si sa compilation fut déjà terminée. */
    //     return nullptr;
    // }

    m_id_phase_courante = nouvelle_phase;
    return messagère->ajoute_message_phase_compilation(this);

#undef IMPRIME_CHANGEMENT_DE_PHASE
}

SiteSource EspaceDeTravail::site_source_pour(const NoeudExpression *noeud) const
{
    if (!noeud) {
        return {};
    }

    auto lexème = noeud->lexème;
    if (!lexème && noeud->est_corps_fonction()) {
        lexème = noeud->comme_corps_fonction()->entête->lexème;
    }

    auto const fichier = this->fichier(lexème->fichier);
    return SiteSource::crée(fichier, lexème);
}

Erreur EspaceDeTravail::rapporte_avertissement(const NoeudExpression *site,
                                               kuri::chaine_statique message) const
{
    return ::rapporte_avertissement(this, site_source_pour(site), message);
}

Erreur EspaceDeTravail::rapporte_avertissement(kuri::chaine_statique chemin_fichier,
                                               int ligne,
                                               kuri::chaine_statique message) const
{
    const Fichier *f = this->fichier(chemin_fichier);
    return ::rapporte_avertissement(this, SiteSource(f, ligne), message);
}

Erreur EspaceDeTravail::rapporte_avertissement_externe(ParamètresErreurExterne const &params) const
{
    possède_erreur = true;
    return ::rapporte_avertissement(this, params);
}

Erreur EspaceDeTravail::rapporte_info(const NoeudExpression *site,
                                      kuri::chaine_statique message) const
{
    return ::rapporte_info(this, site_source_pour(site), message);
}

Erreur EspaceDeTravail::rapporte_info(kuri::chaine_statique chemin_fichier,
                                      int ligne,
                                      kuri::chaine_statique message) const
{
    const Fichier *f = this->fichier(chemin_fichier);
    return ::rapporte_info(this, SiteSource(f, ligne - 1), message);
}

Erreur EspaceDeTravail::rapporte_info(SiteSource site, kuri::chaine_statique message) const
{
    return ::rapporte_info(this, site, message);
}

Erreur EspaceDeTravail::rapporte_info_externe(ParamètresErreurExterne const &params) const
{
    possède_erreur = true;
    return ::rapporte_info(this, params);
}

Erreur EspaceDeTravail::rapporte_erreur(NoeudExpression const *site,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;

    if (!site) {
        return rapporte_erreur_sans_site(message, genre);
    }

    return ::rapporte_erreur(this, site_source_pour(site), message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(kuri::chaine_statique chemin_fichier,
                                        int ligne,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;
    auto fichier = this->fichier(chemin_fichier);
    return ::rapporte_erreur(this, SiteSource(fichier, ligne), message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(SiteSource site,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possède_erreur = true;
    return ::rapporte_erreur(this, site, message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur_sans_site(kuri::chaine_statique message,
                                                  erreur::Genre genre) const
{
    possède_erreur = true;
    return ::rapporte_erreur(this, {}, message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur_externe(ParamètresErreurExterne const &params) const
{
    possède_erreur = true;
    return ::rapporte_erreur(this, params);
}

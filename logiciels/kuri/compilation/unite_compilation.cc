/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "unite_compilation.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "metaprogramme.hh"
#include "typage.hh"
#include "validation_expression_appel.hh"
#include "validation_semantique.hh"

static constexpr auto CYCLES_MAXIMUM = 1000;

const char *chaine_raison_d_être(RaisonDÊtre raison_d_être)
{
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine)                                              \
    case RaisonDÊtre::Genre:                                                                      \
        return chaine;
    switch (raison_d_être) {
        ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
    }
#undef ENUMERE_RAISON_D_ETRE_EX
    return "ceci ne devrait pas arriver";
}

static UniteCompilation *unité_pour_attente(Attente const &attente)
{
    if (attente.info && attente.info->unité_pour_attente) {
        return attente.info->unité_pour_attente(attente);
    }
    return nullptr;
}

std::ostream &operator<<(std::ostream &os, RaisonDÊtre raison_d_être)
{
    return os << chaine_raison_d_être(raison_d_être);
}

void UniteCompilation::ajoute_attente(Attente attente)
{
    if (!est_attente_sur_symbole_précédent(attente)) {
        /* Ne remettons le cycle à zéro que si nous attendons sur autre chose que le même symbole
         * précédemment attendu ; sinon nous resterions bloqués dans une compilation infinie. */
        cycle = 0;
    }

    m_attentes.ajoute(attente);
    état = État::EN_ATTENTE;
    assert(attente.est_valide());
#ifdef ENREGISTRE_HISTORIQUE
    m_historique.ajoute({état, m_raison_d_être, __func__});
#endif
}

void UniteCompilation::marque_prête(bool préserve_cycle)
{
    état = État::EN_COURS_DE_COMPILATION;
    m_attentes.efface();
    if (!préserve_cycle) {
        cycle = 0;
    }
#ifdef ENREGISTRE_HISTORIQUE
    m_historique.ajoute({état, m_raison_d_être, __func__});
#endif
}

bool UniteCompilation::est_bloquée() const
{
    auto attente_bloquée = première_attente_bloquée();
    if (!attente_bloquée) {
        return false;
    }

    auto visitées = kuri::ensemblon<UniteCompilation const *, 16>();
    visitées.insère(this);

    auto attendue = unité_pour_attente(*attente_bloquée);
    while (attendue) {
        if (visitées.possède(attendue)) {
            /* La dépendance cyclique sera rapportée via le message d'erreur qui appelera
             * « chaine_attente_recursive() ». */
            return true;
        }
        visitées.insère(attendue);

        attente_bloquée = attendue->première_attente_bloquée();
        if (!attente_bloquée) {
            return false;
        }

        attendue = unité_pour_attente(*attente_bloquée);
    }

    return true;
}

static std::optional<ConditionBlocageAttente> condition_blocage(Attente const &attente)
{
    if (attente.info && attente.info->condition_blocage) {
        return attente.info->condition_blocage(attente);
    }
    return {};
}

Attente const *UniteCompilation::première_attente_bloquée() const
{
    POUR (m_attentes) {
        auto const condition_potentielle = condition_blocage(it);
        if (!condition_potentielle.has_value()) {
            /* Aucune condition potentielle pour notre attente, donc nous ne sommes pas bloqués. */
            continue;
        }

        auto const condition = condition_potentielle.value();
        auto const phase_espace = espace->phase_courante();
        auto const id_phase_espace = espace->id_phase_courante();

        if (id_phase_espace != id_phase_cycle) {
            /* L'espace a changé de phase, nos cycles sont invalidés. */
            id_phase_cycle = id_phase_espace;
            cycle = 0;
            continue;
        }

        if (phase_espace < condition.phase) {
            /* L'espace n'a pas dépassé la phase limite, nos cycles sont invalides. */
            cycle = 0;
            continue;
        }

        /* L'espace est sur la phase ou après. Nous avons jusqu'à CYCLES_MAXIMUM pour être
         * satisfaits.
         */
        if (cycle > CYCLES_MAXIMUM) {
            return &it;
        }
    }

    return nullptr;
}

Attente const *UniteCompilation::première_attente_bloquée_ou_non() const
{
    auto attente = première_attente_bloquée();
    if (attente) {
        return attente;
    }

    if (m_attentes.taille()) {
        return &m_attentes[0];
    }

    return nullptr;
}

static kuri::chaine commentaire_pour_attente(Attente const &attente)
{
    if (attente.info && attente.info->commentaire) {
        return attente.info->commentaire(attente);
    }
    return "ERREUR COMPILATRICE";
}

static void émets_erreur_pour_attente(UniteCompilation const *unite, Attente const &attente)
{
    if (attente.info && attente.info->émets_erreur) {
        attente.info->émets_erreur(unite, attente);
    }
}

void UniteCompilation::rapporte_erreur() const
{
    auto attente = première_attente_bloquée();
    émets_erreur_pour_attente(this, *attente);
}

kuri::chaine UniteCompilation::chaine_attentes_récursives() const
{
    Enchaineuse fc;

    auto attente = première_attente_bloquée();
    assert(attente);
    auto attendue = unité_pour_attente(*attente);
    auto commentaire = commentaire_pour_attente(*attente);

    if (!attendue) {
        fc << "    " << commentaire << " est bloquée !\n";
    }

    kuri::ensemble<UniteCompilation const *> unités_visitées;
    unités_visitées.insère(this);

    while (attendue) {
        if (attendue->est_prête()) {
            fc << "    " << commentaire << " est prête !\n";
            break;
        }

        if (unités_visitées.possède(attendue)) {
            fc << "    erreur : dépendance cyclique !\n";
            break;
        }

        attente = attendue->première_attente_bloquée_ou_non();
        fc << "    " << commentaire << " attend sur ";
        commentaire = commentaire_pour_attente(*attente);
        fc << commentaire << '\n';

        unités_visitées.insère(attendue);

        attendue = unité_pour_attente(*attente);
    }

    return fc.chaine();
}

static bool attente_est_résolue(EspaceDeTravail *espace, Attente &attente)
{
    if (attente.info && attente.info->est_résolue) {
        return attente.info->est_résolue(espace, attente);
    }
    return true;
}

UniteCompilation::ÉtatAttentes UniteCompilation::détermine_état_attentes()
{
    if (est_prête()) {
        return UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES;
    }

    auto toutes_les_attentes_sont_résolues = true;
    auto attente_sur_symbole = false;
    POUR (m_attentes) {
        if (!it.est_valide()) {
            continue;
        }

        if (it.est<AttenteSurSymbole>()) {
            toutes_les_attentes_sont_résolues = false;
            attente_sur_symbole = true;
            continue;
        }

        if (!attente_est_résolue(espace, it)) {
            toutes_les_attentes_sont_résolues = false;
            continue;
        }

        /* À FAIRE : généralise. */
        if (it.est<AttenteSurNoeudCode>()) {
            assert(m_raison_d_être == RaisonDÊtre::ENVOIE_MESSAGE);
            static_cast<MessageTypageCodeTermine *>(message)->code =
                it.noeud_code().noeud->noeud_code;
        }

        it = {};
    }

    if (toutes_les_attentes_sont_résolues) {
        marque_prête(false);
        return UniteCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES;
    }

    if (est_bloquée()) {
        return UniteCompilation::ÉtatAttentes::ATTENTES_BLOQUÉES;
    }

    if (attente_sur_symbole) {
        marque_prête_pour_attente_sur_symbole();
        return UniteCompilation::ÉtatAttentes::UN_SYMBOLE_EST_ATTENDU;
    }

    return UniteCompilation::ÉtatAttentes::ATTENTES_NON_RÉSOLUES;
}

void UniteCompilation::marque_prête_pour_attente_sur_symbole()
{
    auto attente_courante = m_attentes[0];
    auto préserve_cycle = false;

    if (attente_courante.symbole() == m_attente_sur_symbole_précédente) {
        cycle += 1;
        préserve_cycle = true;
    }

    marque_prête(préserve_cycle);
    m_attente_sur_symbole_précédente = attente_courante.symbole();
}

bool UniteCompilation::est_attente_sur_symbole_précédent(Attente attente) const
{
    if (!attente.est<AttenteSurSymbole>()) {
        return false;
    }

    return m_attente_sur_symbole_précédente == attente.symbole();
}

int64_t UniteCompilation::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += m_attentes.taille_mémoire();
#ifdef ENREGISTRE_HISTORIQUE
    résultat += m_historique.taille_mémoire();
#endif
    return résultat;
}

const char *chaine_état_unité_compilation(UniteCompilation::État état)
{
#define ENUMERE_ETAT_UNITE_COMPILATION_EX(Genre)                                                  \
    case UniteCompilation::État::Genre:                                                           \
        return #Genre;
    switch (état) {
        ENUMERE_ETAT_UNITE_COMPILATION(ENUMERE_ETAT_UNITE_COMPILATION_EX)
    }
#undef ENUMERE_ETAT_UNITE_COMPILATION_EX
    return "ceci ne devrait pas arriver";
}

std::ostream &operator<<(std::ostream &os, UniteCompilation::État état)
{
    return os << chaine_état_unité_compilation(état);
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires pour le débogage.
 * \{ */

void imprime_historique_unité(std::ostream &os, const UniteCompilation *unité)
{
    auto historique = unité->donne_historique();
    POUR (historique) {
        os << "---- " << it.fonction << " ; " << it.raison << " ; " << it.état << '\n';
    }
}

void imprime_attentes_unité(std::ostream &os, const UniteCompilation *unité)
{
    POUR (unité->donne_attentes()) {
        if (it.info && it.info->commentaire) {
            os << "---- " << it.info->commentaire(it) << '\n';
        }
        else {
            os << "---- attente sans commentaire\n";
        }
    }
}

void imprime_état_unité(std::ostream &os, const UniteCompilation *unité)
{
    os << "-- " << unité->donne_état() << '\n';
#ifdef ENREGISTRE_HISTORIQUE
    os << "-- historique :\n";
    imprime_historique_unité(os, unité);
#endif
    os << "-- attentes :\n";
    imprime_attentes_unité(os, unité);
}

void imprime_noeud_index_courant_unité(std::ostream &os, UniteCompilation const *unité)
{
    auto arbre_aplatis = unité->arbre_aplatis;
    if (!arbre_aplatis) {
        os << "-- aucun arbre aplatis\n";
        return;
    }

    if (arbre_aplatis->noeuds.taille() == 0) {
        os << "-- l'arbre est vide\n";
        return;
    }

    auto site = arbre_aplatis->noeuds[arbre_aplatis->index_courant];
    os << "-- " << *site << '\n';

    if (site->est_appel()) {
        auto appel = site->comme_appel();

        if (appel->état_résolution_appel) {
            os << "-- " << static_cast<int>(appel->état_résolution_appel->état) << '\n';
        }
        else {
            os << "-- aucun état de résolution pour l'expression d'appel\n";
        }
    }
}

/** \} */

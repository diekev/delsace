/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "unite_compilation.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "metaprogramme.hh"
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

static UnitéCompilation *unité_pour_attente(Attente const &attente)
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

void UnitéCompilation::ajoute_attente(Attente attente)
{
    if (!est_attente_sur_symbole_précédent(attente) &&
        !est_attente_sur_opérateur_précédent(attente)) {
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

void UnitéCompilation::marque_prête(bool préserve_cycle)
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

bool UnitéCompilation::est_bloquée() const
{
    auto attente_bloquée = première_attente_bloquée();
    if (!attente_bloquée) {
        return false;
    }

    auto visitées = kuri::ensemblon<UnitéCompilation const *, 16>();
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

Attente const *UnitéCompilation::première_attente_bloquée() const
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

Attente const *UnitéCompilation::première_attente_bloquée_ou_non() const
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

static void émets_erreur_pour_attente(UnitéCompilation const *unité, Attente const &attente)
{
    if (attente.info && attente.info->émets_erreur) {
        attente.info->émets_erreur(unité, attente);
    }
}

void UnitéCompilation::rapporte_erreur() const
{
    auto attente = première_attente_bloquée();
    émets_erreur_pour_attente(this, *attente);
}

kuri::chaine UnitéCompilation::chaine_attentes_récursives() const
{
    Enchaineuse fc;

    auto attente = première_attente_bloquée();
    assert(attente);
    auto attendue = unité_pour_attente(*attente);
    auto commentaire = attente->donne_commentaire();

    if (!attendue) {
        fc << "    " << commentaire << " est bloquée !\n";
    }

    kuri::ensemble<UnitéCompilation const *> unités_visitées;
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
        commentaire = attente->donne_commentaire();
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

UnitéCompilation::ÉtatAttentes UnitéCompilation::détermine_état_attentes()
{
    if (est_prête()) {
        return UnitéCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES;
    }

    auto toutes_les_attentes_sont_résolues = true;
    auto attente_sur_symbole = false;
    auto attente_sur_opérateur = std::optional<Attente>();
    POUR (m_attentes) {
        if (!it.est_valide()) {
            continue;
        }

        if (it.est<AttenteSurSymbole>()) {
            toutes_les_attentes_sont_résolues = false;
            attente_sur_symbole = true;
            continue;
        }

        if (it.est<AttenteSurOpérateur>()) {
            toutes_les_attentes_sont_résolues = false;
            attente_sur_opérateur = it;
            break;
        }

        if (!attente_est_résolue(espace, it)) {
            toutes_les_attentes_sont_résolues = false;
            continue;
        }

        /* À FAIRE : généralise. */
        if (it.est<AttenteSurNoeudCode>()) {
            assert(m_raison_d_être == RaisonDÊtre::ENVOIE_MESSAGE);
            static_cast<MessageTypageCodeTerminé *>(message)->code =
                it.noeud_code().noeud->noeud_code;
        }

        it = {};
    }

    if (toutes_les_attentes_sont_résolues) {
        marque_prête(false);
        return UnitéCompilation::ÉtatAttentes::ATTENTES_RÉSOLUES;
    }

    if (est_bloquée()) {
        return UnitéCompilation::ÉtatAttentes::ATTENTES_BLOQUÉES;
    }

    if (attente_sur_symbole) {
        marque_prête_pour_attente_sur_symbole();
        return UnitéCompilation::ÉtatAttentes::UN_SYMBOLE_EST_ATTENDU;
    }

    if (attente_sur_opérateur) {
        marque_prête_pour_attente_sur_opérateur(attente_sur_opérateur.value());
        return UnitéCompilation::ÉtatAttentes::UN_OPÉRATEUR_EST_ATTENDU;
    }

    return UnitéCompilation::ÉtatAttentes::ATTENTES_NON_RÉSOLUES;
}

void UnitéCompilation::marque_prête_pour_attente_sur_symbole()
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

bool UnitéCompilation::est_attente_sur_symbole_précédent(Attente attente) const
{
    if (!attente.est<AttenteSurSymbole>()) {
        return false;
    }

    return m_attente_sur_symbole_précédente == attente.symbole();
}

void UnitéCompilation::marque_prête_pour_attente_sur_opérateur(Attente attente)
{
    auto préserve_cycle = false;

    if (attente.opérateur() == m_attente_sur_opérateur_précédente) {
        cycle += 1;
        préserve_cycle = true;
    }

    marque_prête(préserve_cycle);
    m_attente_sur_opérateur_précédente = attente.opérateur();
}

bool UnitéCompilation::est_attente_sur_opérateur_précédent(Attente attente) const
{
    if (!attente.est<AttenteSurOpérateur>()) {
        return false;
    }

    return m_attente_sur_opérateur_précédente == attente.opérateur();
}

int64_t UnitéCompilation::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += m_attentes.taille_mémoire();
#ifdef ENREGISTRE_HISTORIQUE
    résultat += m_historique.taille_mémoire();
#endif
    return résultat;
}

const char *chaine_état_unité_compilation(UnitéCompilation::État état)
{
#define ENUMERE_ETAT_UNITE_COMPILATION_EX(Genre)                                                  \
    case UnitéCompilation::État::Genre:                                                           \
        return #Genre;
    switch (état) {
        ENUMERE_ETAT_UNITE_COMPILATION(ENUMERE_ETAT_UNITE_COMPILATION_EX)
    }
#undef ENUMERE_ETAT_UNITE_COMPILATION_EX
    return "ceci ne devrait pas arriver";
}

std::ostream &operator<<(std::ostream &os, UnitéCompilation::État état)
{
    return os << chaine_état_unité_compilation(état);
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires pour le débogage.
 * \{ */

void imprime_historique_unité(Enchaineuse &enchaineuse, const UnitéCompilation *unité)
{
    auto historique = unité->donne_historique();
    POUR (historique) {
        enchaineuse << "---- " << it.fonction << " ; " << it.raison << " ; " << it.état << '\n';
    }
}

void imprime_attentes_unité(Enchaineuse &enchaineuse, const UnitéCompilation *unité)
{
    POUR (unité->donne_attentes()) {
        if (it.info && it.info->commentaire) {
            enchaineuse << "---- " << it.info->commentaire(it) << '\n';
        }
        else {
            enchaineuse << "---- attente sans commentaire\n";
        }
    }
}

void imprime_état_unité(Enchaineuse &enchaineuse, const UnitéCompilation *unité)
{
    enchaineuse << "-- " << unité->donne_état() << '\n';
#ifdef ENREGISTRE_HISTORIQUE
    enchaineuse << "-- historique :\n";
    imprime_historique_unité(enchaineuse, unité);
#endif
    enchaineuse << "-- attentes :\n";
    imprime_attentes_unité(enchaineuse, unité);
}

void imprime_noeud_indice_courant_unité(std::ostream &os, UnitéCompilation const *unité)
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

    auto site = arbre_aplatis->noeuds[arbre_aplatis->indice_courant];
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

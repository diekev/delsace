/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "unite_compilation.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "metaprogramme.hh"
#include "typage.hh"

static constexpr auto CYCLES_MAXIMUM = 1000;

const char *chaine_raison_d_etre(RaisonDEtre raison_d_etre)
{
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine)                                              \
    case RaisonDEtre::Genre:                                                                      \
        return chaine;
    switch (raison_d_etre) {
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

std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre)
{
    return os << chaine_raison_d_etre(raison_d_etre);
}

bool UniteCompilation::est_bloquee() const
{
    auto attente_bloquée = première_attente_bloquée();
    if (!attente_bloquée) {
        return false;
    }

    auto visitees = kuri::ensemblon<UniteCompilation const *, 16>();
    visitees.insere(this);

    auto attendue = unité_pour_attente(*attente_bloquée);
    while (attendue) {
        if (visitees.possede(attendue)) {
            /* La dépendance cyclique sera rapportée via le message d'erreur qui appelera
             * « chaine_attente_recursive() ». */
            return true;
        }
        visitees.insere(attendue);

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
            /* Aucune condition potentille pour notre attente, donc nous ne sommes pas bloqués. */
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

kuri::chaine UniteCompilation::chaine_attentes_recursives() const
{
    Enchaineuse fc;

    auto attente = première_attente_bloquée();
    assert(attente);
    auto attendue = unité_pour_attente(*attente);
    auto commentaire = commentaire_pour_attente(*attente);

    if (!attendue) {
        fc << "    " << commentaire << " est bloquée !\n";
    }

    kuri::ensemble<UniteCompilation const *> unite_visite;
    unite_visite.insere(this);

    while (attendue) {
        if (attendue->est_prete()) {
            fc << "    " << commentaire << " est prête !\n";
            break;
        }

        if (unite_visite.possede(attendue)) {
            fc << "    erreur : dépendance cyclique !\n";
            break;
        }

        attente = attendue->première_attente_bloquée_ou_non();
        fc << "    " << commentaire << " attend sur ";
        commentaire = commentaire_pour_attente(*attente);
        fc << commentaire << '\n';

        unite_visite.insere(attendue);

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

void UniteCompilation::marque_prete_si_attente_resolue()
{
    if (est_prete()) {
        return;
    }

    auto toutes_les_attentes_sont_résolues = true;
    POUR (m_attentes) {
        if (!it.est_valide()) {
            continue;
        }

        if (!attente_est_résolue(espace, it)) {
            toutes_les_attentes_sont_résolues = false;
            continue;
        }

        /* À FAIRE : généralise. */
        if (it.est<AttenteSurNoeudCode>()) {
            assert(m_raison_d_etre == RaisonDEtre::ENVOIE_MESSAGE);
            static_cast<MessageTypageCodeTermine *>(message)->code =
                it.noeud_code().noeud->noeud_code;
        }

        it = {};
    }

    if (toutes_les_attentes_sont_résolues) {
        marque_prete();
    }
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
            std::cerr << "---- " << it.info->commentaire(it) << '\n';
        }
        else {
            std::cerr << "---- attente sans commentaire\n";
        }
    }
}

void imprime_état_unité(std::ostream &os, const UniteCompilation *unité)
{
    os << "-- " << unité->donne_état() << '\n';
    os << "-- historique :\n";
    imprime_historique_unité(os, unité);
    os << "-- attentes :\n";
    imprime_attentes_unité(os, unité);
}

static void imprime_noeud_index_courant_unité(
    std::ostream &os,
    kuri::tableau_statique<NoeudExpression *> const &arbre_aplatis,
    UniteCompilation const *unité)
{
    if (arbre_aplatis.taille() == 0) {
        os << "-- l'arbre est vide\n";
        return;
    }

    auto site = arbre_aplatis[unité->index_courant];
    os << "-- " << *site << '\n';

    if (site->est_appel()) {
        auto appel = site->comme_appel();

        if (appel->état_résolution_appel) {
            std::cerr << "-- " << static_cast<int>(appel->état_résolution_appel->état) << '\n';
        }
        else {
            std::cerr << "-- aucun état de résolution pour l'expression "
                         "d'appel\n";
        }
    }
}

void imprime_noeud_index_courant_unité(std::ostream &os,
                                       const NoeudDeclarationEnteteFonction *entête)
{
    imprime_noeud_index_courant_unité(os, entête->arbre_aplatis, entête->unite);
}

void imprime_noeud_index_courant_unité(std::ostream &os,
                                       const NoeudDeclarationCorpsFonction *corps)
{
    imprime_noeud_index_courant_unité(os, corps->arbre_aplatis, corps->unite);
}

/** \} */

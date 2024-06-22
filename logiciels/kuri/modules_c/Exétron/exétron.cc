/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include <condition_variable>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>

#include "exétron.h"

#include "../InterfaceCKuri/contexte_kuri.hh"

#include <tbb/flow_graph.h>
#include <tbb/parallel_for.h>

extern "C" {

struct FilExecution {
    std::function<void(void *)> rappel_;
    void *argument_;
    std::thread thread;
    bool fut_joint_ = false;
    bool termine_ = false;

    FilExecution(std::function<void(void *)> rappel, void *arg)
        : rappel_(rappel), argument_(arg), thread(&FilExecution::lance, this)
    {
    }

    FilExecution(FilExecution const &) = delete;
    FilExecution &operator=(FilExecution const &) = delete;

    ~FilExecution()
    {
        if (!fut_joint_) {
            joins();
        }
    }

    static void lance(void *arg)
    {
        auto moi = static_cast<FilExecution *>(arg);
        moi->rappel_(moi->argument_);
        moi->termine_ = true;
    }

    void annule()
    {
        thread.~thread();
    }

    void detache()
    {
        thread.detach();
    }

    bool joins()
    {
        fut_joint_ = true;

        try {
            thread.join();
            termine_ = true;
            return true;
        }
        catch (const std::system_error &) {
            termine_ = true;
            return false;
        }
    }

    bool termine() const
    {
        return termine_;
    }
};

uint32_t EXETRON_nombre_fils_materiel()
{
    return std::thread::hardware_concurrency();
}

FilExecution *EXETRON_cree_fil(ContexteKuri *ctx_kuri, void (*rappel)(void *), void *argument)
{
    return kuri_loge<FilExecution>(ctx_kuri, rappel, argument);
}

/* À FAIRE : ajout d'une durée minimum à attendre avant d'annuler si le fil ne fut pas joint. */
bool EXETRON_joins_fil(FilExecution *ptr_fil)
{
    return ptr_fil->joins();
}

void EXETRON_detache_fil(FilExecution *ptr_fil)
{
    ptr_fil->detache();
}

void EXETRON_detruit_fil(ContexteKuri *ctx_kuri, FilExecution *ptr_fil)
{
    kuri_deloge(ctx_kuri, ptr_fil);
}

struct Mutex : public std::mutex {};

Mutex *EXETRON_cree_mutex(ContexteKuri *ctx_kuri)
{
    return kuri_loge<Mutex>(ctx_kuri);
}

void EXETRON_verrouille_mutex(Mutex *mutex)
{
    mutex->lock();
}

void EXETRON_deverrouille_mutex(Mutex *mutex)
{
    mutex->unlock();
}

void EXETRON_detruit_mutex(ContexteKuri *ctx_kuri, Mutex *mutex)
{
    kuri_deloge(ctx_kuri, mutex);
}

struct VariableCondition : public std::condition_variable {};

VariableCondition *EXETRON_cree_variable_condition(ContexteKuri *ctx_kuri)
{
    return kuri_loge<VariableCondition>(ctx_kuri);
}

void EXETRON_variable_condition_notifie_tous(VariableCondition *condition_variable)
{
    condition_variable->notify_all();
}

void EXETRON_variable_condition_notifie_un(VariableCondition *condition_variable)
{
    condition_variable->notify_one();
}

void EXETRON_variable_condition_attend(VariableCondition *condition_variable, Mutex *mutex)
{
    std::unique_lock l(*static_cast<std::mutex *>(mutex));
    condition_variable->wait(l);
}

void EXETRON_detruit_variable_condition(ContexteKuri *ctx_kuri,
                                        VariableCondition *condition_variable)
{
    kuri_deloge(ctx_kuri, condition_variable);
}

static int64_t taille_plage(PlageExecution const *plage)
{
    return plage->fin - plage->debut;
}

#define RETOURNE_SI_NUL                                                                           \
    if (!donnees || !donnees->fonction || !plage) {                                               \
        return;                                                                                   \
    }

void EXETRON_boucle_serie(PlageExecution const *plage, DonneesTacheParallele *donnees)
{
    RETOURNE_SI_NUL
    donnees->fonction(donnees, plage);
}

void EXETRON_boucle_parallele(PlageExecution const *plage,
                              DonneesTacheParallele *donnees,
                              int granularite)
{
    RETOURNE_SI_NUL
    auto const taille = taille_plage(plage);
    if (taille == 0) {
        return;
    }

    /* Vérifie si une seule tache peut être créée. */
    if (taille <= granularite) {
        EXETRON_boucle_serie(plage, donnees);
        return;
    }

    using type_plage = tbb::blocked_range<int64_t>;

    tbb::parallel_for(type_plage(plage->debut, plage->fin), [&](const type_plage &plage_) {
        PlageExecution plage_execution{plage_.begin(), plage_.end()};
        donnees->fonction(donnees, &plage_execution);
    });
}

void EXETRON_boucle_parallele_legere(PlageExecution const *plage, DonneesTacheParallele *donnees)
{
    RETOURNE_SI_NUL
    EXETRON_boucle_parallele(plage, donnees, 1024);
}

void EXETRON_boucle_parallele_lourde(PlageExecution const *plage, DonneesTacheParallele *donnees)
{
    RETOURNE_SI_NUL
    EXETRON_boucle_parallele(plage, donnees, 1);
}

struct corps_fonction {
    fonction_noeud_execution m_fonction;
    void *m_données;

    corps_fonction(fonction_noeud_execution fonction, void *donnees)
        : m_fonction(fonction), m_données(donnees)
    {
    }

    void operator()(tbb::flow::continue_msg) const
    {
        if (m_fonction) {
            m_fonction(m_données);
        }
    }
};

class graphe_execution {
    tbb::flow::graph m_graphe{};
    std::vector<tbb::flow::broadcast_node<tbb::flow::continue_msg>> m_racines{};
    std::vector<tbb::flow::continue_node<tbb::flow::continue_msg>> m_noeuds{};

  public:
    uint64_t crée_racine()
    {
        m_racines.emplace_back(m_graphe);
        return m_racines.size() << 32;
    }

    uint64_t crée_noeud(fonction_noeud_execution fonction, void *donnees)
    {
        m_noeuds.emplace_back(m_graphe, corps_fonction(fonction, donnees));
        return m_noeuds.size();
    }

    void crée_connexion(uint64_t sortie, uint64_t entrée)
    {
        if (sortie >> 32 != 0 && entrée >> 32 == 0) {
            tbb::flow::make_edge(m_racines[(sortie >> 32) - 1], m_noeuds[entrée - 1]);
        }
        else if (sortie >> 32 == 0 && entrée >> 32 == 0) {
            tbb::flow::make_edge(m_noeuds[sortie - 1], m_noeuds[entrée - 1]);
        }
    }

    void exécute(uint64_t racine)
    {
        auto index_racine = (racine >> 32) - 1;
        m_racines[index_racine].try_put(tbb::flow::continue_msg());
    }

    void attend_sur_tous()
    {
        m_graphe.wait_for_all();
    }

    void exécute_et_attend_sur_tous(uint64_t racine)
    {
        exécute(racine);
        attend_sur_tous();
    }
};

struct poignee_graphe_execution *EXETRON_cree_graphe_execution()
{
    auto résultat = new graphe_execution();
    return reinterpret_cast<poignee_graphe_execution *>(résultat);
}

void EXETRON_detruit_graphe_execution(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    delete graphe_tbb;
}

void EXETRON_graphe_execute(struct poignee_graphe_execution *graphe, uint64_t racine)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    graphe_tbb->exécute(racine);
}

void EXETRON_graphe_attend_sur_tous(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    graphe_tbb->attend_sur_tous();
}

void EXETRON_graphe_execute_et_attend_sur_tous(struct poignee_graphe_execution *graphe,
                                               uint64_t racine)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    graphe_tbb->exécute_et_attend_sur_tous(racine);
}

uint64_t EXETRON_graphe_cree_racine(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    return graphe_tbb->crée_racine();
}

uint64_t EXETRON_graphe_cree_noeud(struct poignee_graphe_execution *graphe,
                                   fonction_noeud_execution fonction,
                                   void *donnees)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    return graphe_tbb->crée_noeud(fonction, donnees);
}

void EXETRON_graphe_connecte_noeud_noeud(struct poignee_graphe_execution *graphe,
                                         uint64_t sortie,
                                         uint64_t entree)
{
    auto graphe_tbb = reinterpret_cast<graphe_execution *>(graphe);
    graphe_tbb->crée_connexion(sortie, entree);
}

#if 0
struct poignee_noeud_racine;
struct poignee_noeud_execution;

struct poignee_graphe_execution *EXETRON_cree_graphe_execution()
{
    auto résultat = new tbb::flow::graph();
    return reinterpret_cast<poignee_graphe_execution *>(résultat);
}

void EXETRON_detruit_graphe_execution(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<tbb::flow::graph *>(graphe);
    delete graphe_tbb;
}

void EXETRON_graphe_attend_sur_tous(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<tbb::flow::graph *>(graphe);
    graphe_tbb->wait_for_all();
}

struct poignee_noeud_racine *EXETRON_donne_noeud_racine(struct poignee_graphe_execution *graphe)
{
    auto graphe_tbb = reinterpret_cast<tbb::flow::graph *>(graphe);
    auto résultat = new tbb::flow::broadcast_node<tbb::flow::continue_msg>(*graphe_tbb);
    return reinterpret_cast<poignee_noeud_racine *>(résultat);
}

void EXETRON_detruit_noeud_racine(struct poignee_noeud_racine *racine)
{
    auto racine_tbb = reinterpret_cast<tbb::flow::broadcast_node<tbb::flow::continue_msg> *>(
        racine);
    delete racine_tbb;
}

struct poignee_noeud_execution *EXETRON_cree_noeud_execution(
    struct poignee_graphe_execution *graphe, fonction_noeud_execution fonction, void *donnees)
{
    auto graphe_tbb = reinterpret_cast<tbb::flow::graph *>(graphe);
    auto résultat = new tbb::flow::continue_node<tbb::flow::continue_msg>(
        *graphe_tbb, corps_fonction(fonction, donnees));
    return reinterpret_cast<poignee_noeud_execution *>(résultat);
}

void EXETRON_detruit_noeud_execution(struct poignee_noeud_execution *noeud)
{
    auto noeud_tbb = reinterpret_cast<tbb::flow::continue_node<tbb::flow::continue_msg> *>(noeud);
    delete noeud_tbb;
}

void EXETRON_connecte_racine_noeud(struct poignee_noeud_racine *racine,
                                   struct poignee_noeud_execution *noeud)
{
    auto racine_tbb = reinterpret_cast<tbb::flow::broadcast_node<tbb::flow::continue_msg> *>(
        racine);
    auto noeud_tbb = reinterpret_cast<tbb::flow::continue_node<tbb::flow::continue_msg> *>(noeud);

    tbb::flow::make_edge(*racine_tbb, *noeud_tbb);
}

void EXETRON_connecte_noeud_noeud(struct poignee_noeud_execution *sortie,
                                  struct poignee_noeud_execution *entree)
{
    auto sortie_tbb = reinterpret_cast<tbb::flow::continue_node<tbb::flow::continue_msg> *>(
        sortie);
    auto entree_tbb = reinterpret_cast<tbb::flow::continue_node<tbb::flow::continue_msg> *>(
        entree);

    tbb::flow::make_edge(*sortie_tbb, *entree_tbb);
}
#endif
}

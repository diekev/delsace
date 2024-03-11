/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include <condition_variable>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>

#include "exétron.h"

#include "../InterfaceCKuri/contexte_kuri.hh"

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
}

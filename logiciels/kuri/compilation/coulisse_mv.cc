/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse_mv.hh"

#include <iostream>

#include "representation_intermediaire/code_binaire.hh"
#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/machine_virtuelle.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "metaprogramme.hh"
#include "programme.hh"

bool CoulisseMV::cree_fichier_objet(Compilatrice &compilatrice,
                                    EspaceDeTravail &espace,
                                    Programme *programme,
                                    ConstructriceRI &constructrice_ri,
                                    Broyeuse &)
{
    auto repr_inter = representation_intermediaire_programme(*programme);
    auto metaprogramme = programme->pour_metaprogramme();
    assert(metaprogramme);

    auto fonction = static_cast<AtomeFonction *>(metaprogramme->fonction->atome);

    if (!fonction) {
        espace.rapporte_erreur(metaprogramme->fonction,
                               "Impossible de trouver la fonction pour le métaprogramme");
        return false;
    }

    if (!repr_inter.globales.est_vide()) {
        auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(
            &espace, repr_inter.globales, fonction);

        if (!fonc_init) {
            return false;
        }

        repr_inter.ajoute_fonction(fonc_init);
    }

    POUR (repr_inter.fonctions) {
        metaprogramme->cibles_appels.insere(it);
    }

    std::unique_lock verrou(compilatrice.mutex_donnees_constantes_executions);

    auto convertisseuse_ri = ConvertisseuseRI(&espace, metaprogramme);
    return convertisseuse_ri.genere_code(repr_inter.fonctions);
}

bool CoulisseMV::cree_executable(Compilatrice &compilatrice,
                                 EspaceDeTravail &espace,
                                 Programme *programme)
{
    std::unique_lock verrou(compilatrice.mutex_donnees_constantes_executions);

    auto metaprogramme = programme->pour_metaprogramme();
    assert(metaprogramme);

    /* Liaison du code binaire du métaprogramme (application des patchs). */
    auto &donnees_constantes = compilatrice.donnees_constantes_executions;

    /* Copie les tableaux de données pour le métaprogramme, ceci est nécessaire car le code binaire
     * des fonctions n'est généré qu'une seule fois, mais l'exécution des métaprogrammes a besoin
     * de pointeurs valides pour trouver les globales et les constantes ; pointeurs qui seraient
     * invalidés lors de l'ajout d'autres globales ou constantes. */
    metaprogramme->donnees_globales = donnees_constantes.donnees_globales;
    metaprogramme->donnees_constantes = donnees_constantes.donnees_constantes;

    /* Nous devons utiliser nos propres données pour les globales, afin que les pointeurs utilisées
     * pour les initialisations des globales (`ptr_donnees_globales + decalage` ici-bas)
     * correspondent aux pointeurs calculés dans la Machine Virtuelle (`ptr_donnees_globales +
     * globale.adresse` là-bas). */
    auto ptr_donnees_globales = metaprogramme->donnees_globales.donnees();
    auto ptr_donnees_constantes = metaprogramme->donnees_constantes.donnees();

    // initialise les globales pour le métaprogramme
    POUR (donnees_constantes.patchs_donnees_constantes) {
        void *adresse_ou = nullptr;
        void *adresse_quoi = nullptr;

        if (it.quoi == ADRESSE_CONSTANTE) {
            adresse_quoi = ptr_donnees_constantes + it.decalage_quoi;
        }
        else {
            adresse_quoi = ptr_donnees_globales + it.decalage_quoi;
        }

        if (it.ou == DONNEES_CONSTANTES) {
            adresse_ou = ptr_donnees_constantes + it.decalage_ou;
        }
        else {
            adresse_ou = ptr_donnees_globales + it.decalage_ou;
        }

        *reinterpret_cast<void **>(adresse_ou) = adresse_quoi;
        // std::cerr << "Écris adresse : " << adresse_quoi << ", à " << adresse_ou << '\n';
    }

    return true;
}

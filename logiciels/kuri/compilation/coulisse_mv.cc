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

bool CoulisseMV::crée_fichier_objet(Compilatrice &compilatrice,
                                    EspaceDeTravail &espace,
                                    Programme *programme,
                                    ConstructriceRI &constructrice_ri,
                                    Broyeuse &)
{
    auto repr_inter = représentation_intermédiaire_programme(*programme);
    auto métaprogramme = programme->pour_métaprogramme();
    assert(métaprogramme);

    auto fonction = static_cast<AtomeFonction *>(métaprogramme->fonction->atome);

    if (!fonction) {
        espace.rapporte_erreur(métaprogramme->fonction,
                               "Impossible de trouver la fonction pour le métaprogramme");
        return false;
    }

    /* Génère les infos type manquants. Les globales représentant des infos types sont substitutées
     * par l'adresse de l'infotype. */
    POUR (repr_inter.globales) {
        if (!it->est_info_type_de) {
            continue;
        }

        auto type = it->est_info_type_de;
        type->info_type = convertisseuse_noeud_code.cree_info_type_pour(const_cast<Type *>(type));
        assert(type->info_type);
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
        métaprogramme->cibles_appels.insere(it);
    }

    std::unique_lock verrou(compilatrice.mutex_données_constantes_exécutions);

    auto convertisseuse_ri = ConvertisseuseRI(&espace, métaprogramme);
    return convertisseuse_ri.genere_code(repr_inter.fonctions);
}

bool CoulisseMV::crée_exécutable(Compilatrice &compilatrice,
                                 EspaceDeTravail &espace,
                                 Programme *programme)
{
    std::unique_lock verrou(compilatrice.mutex_données_constantes_exécutions);

    auto métaprogramme = programme->pour_métaprogramme();
    assert(métaprogramme);

    /* Liaison du code binaire du métaprogramme (application des patchs). */
    auto &données_constantes = compilatrice.données_constantes_exécutions;

    /* Copie les tableaux de données pour le métaprogramme, ceci est nécessaire car le code binaire
     * des fonctions n'est généré qu'une seule fois, mais l'exécution des métaprogrammes a besoin
     * de pointeurs valides pour trouver les globales et les constantes ; pointeurs qui seraient
     * invalidés lors de l'ajout d'autres globales ou constantes. */
    métaprogramme->données_globales = données_constantes.données_globales;
    métaprogramme->données_constantes = données_constantes.données_constantes;

    /* Nous devons utiliser nos propres données pour les globales, afin que les pointeurs utilisés
     * pour les initialisations des globales (`ptr_données_globales + decalage` ici-bas)
     * correspondent aux pointeurs calculés dans la Machine Virtuelle (`ptr_données_globales +
     * globale.adresse` là-bas). */
    auto ptr_données_globales = métaprogramme->données_globales.donnees();
    auto ptr_données_constantes = métaprogramme->données_constantes.donnees();

    // initialise les globales pour le métaprogramme
    POUR (données_constantes.patchs_données_constantes) {
        void *adresse_ou = nullptr;
        void *adresse_quoi = nullptr;

        if (it.quoi == ADRESSE_CONSTANTE) {
            adresse_quoi = ptr_données_constantes + it.décalage_quoi;
        }
        else {
            adresse_quoi = ptr_données_globales + it.décalage_quoi;
        }

        if (it.où == DONNÉES_CONSTANTES) {
            adresse_ou = ptr_données_constantes + it.décalage_où;
        }
        else {
            adresse_ou = ptr_données_globales + it.décalage_où;
        }

        *reinterpret_cast<void **>(adresse_ou) = adresse_quoi;
        // std::cerr << "Écris adresse : " << adresse_quoi << ", à " << adresse_ou << '\n';
    }

    return true;
}

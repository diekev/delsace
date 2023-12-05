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

bool CoulisseMV::génère_code_impl(const ArgsGénérationCode &args)
{
    auto &compilatrice = *args.compilatrice;
    auto &compilatrice_ri = *args.compilatrice_ri;
    auto &espace = *args.espace;
    auto const &programme = *args.programme;

    auto opt_repr_inter = représentation_intermédiaire_programme(
        espace, compilatrice_ri, programme);
    if (!opt_repr_inter.has_value()) {
        return false;
    }
    auto &repr_inter = opt_repr_inter.value();
    auto métaprogramme = programme.pour_métaprogramme();
    assert(métaprogramme);

    /* Génère les infos type manquants. Les globales représentant des infos types sont substitutées
     * par l'adresse de l'infotype. */
    POUR (repr_inter.donne_globales()) {
        if (!it->est_info_type_de) {
            continue;
        }

        auto type = it->est_info_type_de;
        type->info_type = convertisseuse_noeud_code.crée_info_type_pour(const_cast<Type *>(type));
        assert(type->info_type);
    }

    POUR (repr_inter.donne_fonctions()) {
        métaprogramme->cibles_appels.insère(it);
    }

    std::unique_lock verrou(compilatrice.mutex_données_constantes_exécutions);

    auto compilatrice_cb = CompilatriceCodeBinaire(&espace, métaprogramme);
    return compilatrice_cb.génère_code(repr_inter);
}

bool CoulisseMV::crée_fichier_objet_impl(const ArgsCréationFichiersObjets & /*args*/)
{
    return true;
}

bool CoulisseMV::crée_exécutable_impl(const ArgsLiaisonObjets &args)
{
    auto &compilatrice = *args.compilatrice;
    auto programme = args.programme;

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
        void *adresse_source = nullptr;
        if (it.source.type == ADRESSE_CONSTANTE) {
            adresse_source = ptr_données_constantes + it.source.décalage;
        }
        else {
            adresse_source = ptr_données_globales + it.source.décalage;
        }

        void *adresse_destination = nullptr;
        if (it.destination.type == DONNÉES_CONSTANTES) {
            adresse_destination = ptr_données_constantes + it.destination.décalage;
        }
        else if (it.destination.type == DONNÉES_GLOBALES) {
            adresse_destination = ptr_données_globales + it.destination.décalage;
        }
        else {
            assert(it.destination.type == CODE_FONCTION);
            adresse_destination = it.destination.fonction->données_exécution->chunk.code +
                                  it.destination.décalage;
        }

        *reinterpret_cast<void **>(adresse_destination) = adresse_source;
        // std::cerr << "Écris adresse : " << adresse_source << ", à " << adresse_destination <<
        // '\n';
    }

    return true;
}

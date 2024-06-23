/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse_mv.hh"

#include <iostream>

#include "representation_intermediaire/code_binaire.hh"
#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/machine_virtuelle.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "intrinseques.hh"
#include "metaprogramme.hh"
#include "programme.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"

std::optional<ErreurCoulisse> CoulisseMV::génère_code_impl(const ArgsGénérationCode &args)
{
    auto &compilatrice = *args.compilatrice;
    auto &espace = *args.espace;
    auto const &programme = *args.programme;
    auto &repr_inter = *args.ri_programme;
    auto métaprogramme = programme.pour_métaprogramme();
    assert(métaprogramme);

    auto convertisseuse_noeud_code = compilatrice.donne_convertisseuse_noeud_code_disponible();

    /* Génère les infos type manquants. Les globales représentant des infos types sont substitutées
     * par l'adresse de l'infotype. */
    POUR (repr_inter.donne_globales()) {
        if (!it->est_info_type_de) {
            continue;
        }

        auto type = it->est_info_type_de;
        type->info_type = convertisseuse_noeud_code->crée_info_type_pour(compilatrice.typeuse,
                                                                         const_cast<Type *>(type));
        assert(type->info_type);
    }

    POUR (repr_inter.donne_fonctions()) {
        if (!it->est_intrinsèque()) {
            continue;
        }

        if (!intrinsèque_est_supportée_pour_métaprogramme(it->decl->ident)) {
            auto message = enchaine(
                "Utilisation d'une intrinsèque non-implémentée pour les métaprogrammes : ",
                it->decl->ident->nom,
                ".");
            return ErreurCoulisse{message};
        }
    }

    compilatrice.dépose_convertisseuse(convertisseuse_noeud_code);

    POUR (repr_inter.donne_fonctions()) {
        métaprogramme->cibles_appels.insère(it);
    }

    std::unique_lock verrou(compilatrice.mutex_données_constantes_exécutions);

    auto compilatrice_cb = CompilatriceCodeBinaire(&espace, métaprogramme);
    if (!compilatrice_cb.génère_code(repr_inter)) {
        return ErreurCoulisse{"Impossible de générer le code binaire."};
    }

    if (compilatrice.arguments.émets_code_binaire) {
        /* Tri les fonctions selon leurs noms. */
        auto fonctions_repr_inter = repr_inter.donne_fonctions();

        kuri::tableau<AtomeFonction *> fonctions;
        fonctions.réserve(fonctions_repr_inter.taille());

        POUR (fonctions_repr_inter) {
            if (it->est_externe) {
                continue;
            }

            fonctions.ajoute(it);
        }

        kuri::tri_rapide(
            kuri::tableau_statique<AtomeFonction *>(fonctions),
            [](AtomeFonction const *a, AtomeFonction const *b) { return a->nom < b->nom; });

        /* Émets le code binaire. */
        auto &logueuse = programme.pour_métaprogramme()->donne_logueuse(
            TypeLogMétaprogramme::CODE_BINAIRE);
        POUR (fonctions) {
            logueuse << désassemble(it->données_exécution->chunk, it->nom);
        }
    }

    return {};
}

std::optional<ErreurCoulisse> CoulisseMV::crée_fichier_objet_impl(
    const ArgsCréationFichiersObjets & /*args*/)
{
    return {};
}

std::optional<ErreurCoulisse> CoulisseMV::crée_exécutable_impl(const ArgsLiaisonObjets &args)
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
    auto ptr_données_globales = métaprogramme->données_globales.données();
    auto ptr_données_constantes = métaprogramme->données_constantes.données();

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
        // dbg() << "Écris adresse : " << adresse_source << ", à " << adresse_destination;
    }

    return {};
}

int64_t CoulisseMV::mémoire_utilisée() const
{
    return 0;
}

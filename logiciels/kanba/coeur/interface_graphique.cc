/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "interface_graphique.hh"

namespace KNB {

/* ------------------------------------------------------------------------- */
/** \name InterfaceGraphique
 * \{ */

InterfaceGraphique::~InterfaceGraphique()
{
    memoire::deloge("Disposition", m_disposition);
}

void InterfaceGraphique::initialise_interface_par_défaut(InterfaceGraphique &interface)
{
    auto disposition = Disposition::crée(DirectionDisposition::HORIZONTAL);

    auto région_vues_2d_3d = disposition->ajoute_région_conteneur_éditrice();
    région_vues_2d_3d->ajoute_une_éditrice(TypeÉditrice::VUE_3D);
    région_vues_2d_3d->ajoute_une_éditrice(TypeÉditrice::VUE_2D);

    auto région_droite = disposition->ajoute_région_conteneur_région(
        DirectionDisposition::VERTICAL);
    auto disposition_droite = région_droite->donne_disposition();

    auto région_params_brosse = disposition_droite->ajoute_région_conteneur_éditrice();
    région_params_brosse->ajoute_une_éditrice(TypeÉditrice::PARAMÈTRES_BROSSE);

    auto région_calques = disposition_droite->ajoute_région_conteneur_éditrice();
    région_calques->ajoute_une_éditrice(TypeÉditrice::CALQUES);

    interface.installe_disposition(disposition);
}

void InterfaceGraphique::installe_disposition(Disposition *nouvelle_disposition)
{
    memoire::deloge("Disposition", m_disposition);
    m_disposition = nouvelle_disposition;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Disposition
 * \{ */

Disposition *Disposition::crée(DirectionDisposition direction)
{
    auto résultat = memoire::loge<Disposition>("Disposition");
    résultat->m_direction = direction;
    return résultat;
}

Disposition::~Disposition()
{
    for (auto région : m_régions) {
        memoire::deloge("RégionInterface", région);
    }
}

RégionInterface *Disposition::ajoute_région_conteneur_éditrice()
{
    auto région = RégionInterface::crée_conteneur_éditrice();
    m_régions.ajoute(région);
    return région;
}

RégionInterface *Disposition::ajoute_région_conteneur_région(DirectionDisposition direction)
{
    auto région = RégionInterface::crée_conteneur_région(direction);
    m_régions.ajoute(région);
    return région;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name RégionInterface
 * \{ */

RégionInterface *RégionInterface::crée_conteneur_éditrice()
{
    auto résultat = memoire::loge<RégionInterface>("RégionInterface");
    résultat->m_type = TypeRégion::CONTENEUR_ÉDITRICE;
    return résultat;
}

RégionInterface *RégionInterface::crée_conteneur_région(DirectionDisposition direction)
{
    auto résultat = memoire::loge<RégionInterface>("RégionInterface");
    résultat->m_type = TypeRégion::CONTENEUR_RÉGION;
    résultat->m_disposition = Disposition::crée(direction);
    return résultat;
}

RégionInterface::~RégionInterface()
{
    for (auto éditrice : m_éditrices) {
        memoire::deloge("Éditrice", éditrice);
    }

    memoire::deloge("Disposition", m_disposition);
}

Éditrice *RégionInterface::ajoute_une_éditrice(TypeÉditrice type)
{
    assert(this->m_type == TypeRégion::CONTENEUR_ÉDITRICE);
    auto éditrice = Éditrice::crée(type);
    m_éditrices.ajoute(éditrice);
    return éditrice;
}

void RégionInterface::supprime_éditrice_à_l_index(int index)
{
    assert(m_type == TypeRégion::CONTENEUR_ÉDITRICE);
    assert(index >= 0);
    assert(index < m_éditrices.taille());

    memoire::deloge("Éditrice", m_éditrices[index]);
    m_éditrices.erase(m_éditrices.debut() + index);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Éditrice
 * \{ */

dls::chaine nom_pour_type_éditrice(TypeÉditrice type)
{
#define GERE_CAS(valeur, nom)                                                                     \
    case TypeÉditrice::valeur:                                                                    \
        return nom

    switch (type) {
        GERE_CAS(VUE_2D, "Vue Image");
        GERE_CAS(VUE_3D, "Vue 3D");
        GERE_CAS(PARAMÈTRES_BROSSE, "Paramètres Brosse");
        GERE_CAS(CALQUES, "Calques");
        GERE_CAS(INFORMATIONS, "Informations");
    }
#undef GERE_CAS

    return "Inconnu";
}

Éditrice *Éditrice::crée(TypeÉditrice type)
{
    auto résultat = memoire::loge<Éditrice>("Éditrice");
    résultat->m_type = type;
    résultat->m_nom = nom_pour_type_éditrice(type);
    return résultat;
}

/** \} */

}  // namespace KNB

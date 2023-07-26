/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

#include "biblinternes/patrons_conception/repondant_commande.h"

#include "danjo/fournisseuse_icones.hh"

#include "base_editrice.h"

#include "coeur/jorjala.hh"

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice)
{
    auto données = donne_données_programme(jorjala);

    if (données->editrice_active) {
        données->editrice_active->actif(false);
    }

    données->editrice_active = editrice;
}

void appele_commande(JJL::Jorjala &jorjala,
                     dls::chaine const &nom_commande,
                     dls::chaine const &métadonnée)
{
    donne_repondant_commande(jorjala)->repond_clique(nom_commande, métadonnée);
}

/* ------------------------------------------------------------------------- */

class FournisseuseIcôneJorjala final : public danjo::FournisseuseIcône {
    JJL::Jorjala &m_jorjala;

  public:
    FournisseuseIcôneJorjala(JJL::Jorjala &jorjala) : m_jorjala(jorjala)
    {
    }

    std::optional<QIcon> icone_pour_bouton_animation(danjo::ÉtatIcône /*état*/)
    {
        // À FAIRE : icône chronomètre
        return {};
    }

    std::optional<QIcon> icone_pour_echelle_valeur(danjo::ÉtatIcône /*état*/)
    {
        // À FAIRE : icône échelle
        return {};
    }

    std::optional<QIcon> icone_pour_identifiant(std::string const &identifiant,
                                                danjo::ÉtatIcône /*état*/)
    {
        auto chemin = m_jorjala.donne_chemin_pour_identifiant_icône(identifiant.c_str());
        if (chemin.vers_std_string().empty()) {
            return {};
        }

        return QIcon(chemin.vers_std_string().c_str());
    }
};

void initialise_fournisseuse_icône(JJL::Jorjala &jorjala)
{
    auto fournisseuse = memoire::loge<FournisseuseIcôneJorjala>("FournisseuseIcôneJorjala",
                                                                jorjala);
    danjo::définis_fournisseuse_icone(*fournisseuse);
}

/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

#include "biblinternes/patrons_conception/repondant_commande.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#pragma GCC diagnostic pop

#include "danjo/fournisseuse_icones.hh"

#include "base_editrice.h"

#include "coeur/jorjala.hh"

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice)
{
    auto données = accède_données_programme(jorjala);

    if (données->editrice_active) {
        données->editrice_active->actif(false);
    }

    données->editrice_active = editrice;
}

dls::chaine affiche_dialogue(int type, dls::chaine const &filtre)
{
    auto parent = static_cast<QWidget *>(nullptr);
    auto caption = "";
    auto dir = "";

    /* À FAIRE : sort ça de la classe. */
    if (type == FICHIER_OUVERTURE) {
        auto const chemin = QFileDialog::getOpenFileName(parent, caption, dir, filtre.c_str());
        return chemin.toStdString();
    }

    if (type == FICHIER_SAUVEGARDE) {
        auto const chemin = QFileDialog::getSaveFileName(parent, caption, dir, filtre.c_str());
        return chemin.toStdString();
    }

    return "";
}

void appele_commande(JJL::Jorjala &jorjala,
                     dls::chaine const &nom_commande,
                     dls::chaine const &métadonnée)
{
    repondant_commande(jorjala)->repond_clique(nom_commande, métadonnée);
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
    danjo::définit_fournisseuse_icone(*fournisseuse);
}

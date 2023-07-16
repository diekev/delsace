/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

namespace dls {
struct chaine;
}

namespace KNB {

enum class type_evenement : int;
enum class TypeCurseur : int;

class GestionnaireFenetre {
  public:
    virtual ~GestionnaireFenetre() = default;

    virtual void notifie_observatrices(type_evenement evenement) = 0;

    virtual void notifie_erreur(dls::chaine const &message) = 0;

    virtual void change_curseur(TypeCurseur curseur) = 0;

    virtual void restaure_curseur() = 0;

    virtual void définit_titre_application(dls::chaine const &titre) = 0;

    virtual void définit_texte_état_logiciel(dls::chaine const &texte) = 0;

    virtual bool demande_permission_avant_de_fermer() = 0;
};

}  // namespace KNB

/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/patrons_conception/commande.h"

namespace KNB {
struct Kanba;
enum class TypeCurseur : int;
}  // namespace KNB

enum class ModeInsertionHistorique {
    /* L'exécution de la commande ne résulte pas en l'insertion d'un changement dans l'historique.
     */
    IGNORE,
    /* L'exécution de la commande ne résulte en l'insertion d'un changement dans l'historique que
     * si les changements affectant uniquement l'interface (par exemple bouger la caméra 3D)
     * doivent être ajoutés à l'historique. */
    INSÈRE_SI_INTERFACE_VOULUE,
    /* L'exécution de la commande résulte toujours en l'insertion d'un changement dans
     * l'historique.
     */
    INSÈRE_TOUJOURS,
};

class CommandeKanba : public Commande {
  protected:
    virtual ModeInsertionHistorique donne_mode_insertion_historique() const = 0;

    /* Fonctions virtuelles correspondantes à celles requises, remplaçant std::any par
     * KNB::Kanba. */
    virtual int execute_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees);
    virtual void ajourne_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees);
    virtual void termine_execution_modale_kanba(KNB::Kanba &kanba, DonneesCommande const &donnees);
    virtual bool evalue_predicat_kanba(KNB::Kanba &kanba, dls::chaine const &metadonnee);

    virtual KNB::TypeCurseur type_curseur_modal();

  private:
    /* Fonctions requises pas Commande. */
    int execute(std::any const &pointeur, DonneesCommande const &donnees) override;
    void ajourne_execution_modale(std::any const &pointeur,
                                  DonneesCommande const &donnees) override;
    void termine_execution_modale(std::any const &pointeur,
                                  DonneesCommande const &donnees) override;
    bool evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee) override;
};

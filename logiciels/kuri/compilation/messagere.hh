/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/tableau_page.hh"

#include "message.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct MétaProgramme;
struct Module;
struct NoeudExpression;
struct UniteCompilation;

struct Messagère {
  private:
    kuri::tableau_page<MessageFichier> messages_fichiers{};
    kuri::tableau_page<MessageModule> messages_modules{};
    kuri::tableau_page<MessageTypageCodeTerminé> messages_typage_code{};
    kuri::tableau_page<MessagePhaseCompilation> messages_phase_compilation{};
    kuri::tableau_page<MessageEspaceCréé> messages_espace_créé{};

    kuri::tableau<MétaProgramme *> métaprogrammes{};

    int entreceveurs = 0;

    Compilatrice *m_compilatrice = nullptr;

  public:
    Messagère() = default;
    Messagère(const Messagère &) = delete;
    Messagère &operator=(const Messagère &) = delete;

    Messagère(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
    {
    }

    void ajoute_message_fichier_ouvert(EspaceDeTravail *espace, kuri::chaine_statique chemin);
    void ajoute_message_fichier_fermé(EspaceDeTravail *espace, kuri::chaine_statique chemin);
    void ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module);
    void ajoute_message_module_fermé(EspaceDeTravail *espace, Module *module);
    void ajoute_message_espace_créé(EspaceDeTravail *espace, EspaceDeTravail *nouvelle_espace);
    Message *ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud);
    Message *ajoute_message_phase_compilation(EspaceDeTravail *espace);

    int64_t mémoire_utilisée() const;

    void commence_interception(EspaceDeTravail *espace, MétaProgramme *métaprogramme);

    void termine_interception(EspaceDeTravail *espace, MétaProgramme *métaprogramme);

    void envoie_message(Message *message, UniteCompilation *unité);
};

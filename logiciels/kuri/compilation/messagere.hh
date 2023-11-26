/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/noeud_code.hh"

#include "structures/file.hh"

#include "message.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct Module;

struct Messagère {
  private:
    tableau_page<MessageFichier> messages_fichiers{};
    tableau_page<MessageModule> messages_modules{};
    tableau_page<MessageTypageCodeTermine> messages_typage_code{};
    tableau_page<MessagePhaseCompilation> messages_phase_compilation{};

    int64_t pic_de_message = 0;

    bool interception_commencée = false;

    Compilatrice *m_compilatrice = nullptr;

    kuri::file<Message const *> file_message{};

  public:
    Messagère() = default;
    Messagère(const Messagère &) = delete;
    Messagère &operator=(const Messagère &) = delete;

    Messagère(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
    {
    }

    void ajoute_message_fichier_ouvert(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_fichier_fermé(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module);
    void ajoute_message_module_fermé(EspaceDeTravail *espace, Module *module);
    Message *ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud);
    Message *ajoute_message_phase_compilation(EspaceDeTravail *espace);

    int64_t mémoire_utilisée() const;

    inline bool possède_message() const
    {
        return !file_message.est_vide();
    }

    inline bool est_interception_commencée() const
    {
        return interception_commencée;
    }

    Message const *defile();

    void commence_interception(EspaceDeTravail *espace);

    void termine_interception(EspaceDeTravail *espace);

    void purge_messages();
    void envoie_message(Message *message);
};

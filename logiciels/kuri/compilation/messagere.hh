/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/noeud_code.hh"

#include "structures/file.hh"

#include "message.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct Module;

struct Messagere {
  private:
    tableau_page<MessageFichier> messages_fichiers{};
    tableau_page<MessageModule> messages_modules{};
    tableau_page<MessageTypageCodeTermine> messages_typage_code{};
    tableau_page<MessagePhaseCompilation> messages_phase_compilation{};

    int64_t pic_de_message = 0;

    bool interception_commencee = false;

    Compilatrice *m_compilatrice = nullptr;

    kuri::file<Message const *> file_message{};

  public:
    Messagere() = default;
    Messagere(const Messagere &) = delete;
    Messagere &operator=(const Messagere &) = delete;

    Messagere(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
    {
    }

    void ajoute_message_fichier_ouvert(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_fichier_ferme(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module);
    void ajoute_message_module_ferme(EspaceDeTravail *espace, Module *module);
    Message *ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud);
    Message *ajoute_message_phase_compilation(EspaceDeTravail *espace);

    int64_t memoire_utilisee() const;

    inline bool possede_message() const
    {
        return !file_message.est_vide();
    }

    Message const *defile();

    void commence_interception(EspaceDeTravail *espace);

    void termine_interception(EspaceDeTravail *espace);

    void purge_messages();
    void envoie_message(Message *message);
};

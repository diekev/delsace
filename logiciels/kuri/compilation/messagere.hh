/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/file.hh"

#include "arbre_syntaxique/noeud_code.hh"

#include "message.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct Module;
struct NoeudDeclaration;
struct UniteCompilation;

struct Messagere {
  private:
    tableau_page<MessageFichier> messages_fichiers{};
    tableau_page<MessageModule> messages_modules{};
    tableau_page<MessageTypageCodeTermine> messages_typage_code{};
    tableau_page<MessagePhaseCompilation> messages_phase_compilation{};

    long pic_de_message = 0;

    bool interception_commencee = false;

    Compilatrice *m_compilatrice = nullptr;
    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

    dls::file<Message const *> file_message{};

  public:
    Messagere() = default;
    Messagere(const Messagere &) = default;
    Messagere &operator=(const Messagere &) = default;

    Messagere(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
    {
    }

    void ajoute_message_fichier_ouvert(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_fichier_ferme(EspaceDeTravail *espace, kuri::chaine const &chemin);
    void ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module);
    void ajoute_message_module_ferme(EspaceDeTravail *espace, Module *module);
    Message *ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud);
    void ajoute_message_phase_compilation(EspaceDeTravail *espace);

    long memoire_utilisee() const;

    inline bool possede_message() const
    {
        return !file_message.est_vide();
    }

    Message const *defile();

    void commence_interception(EspaceDeTravail *espace);

    void termine_interception(EspaceDeTravail *espace);

    void purge_messages();
};

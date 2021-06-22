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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/chaine.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "attente.hh"

struct EspaceDeTravail;
struct Fichier;
struct Lexeme;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudExpressionReference;
struct NoeudExpression;
struct Type;

#define ENUMERE_RAISON_D_ETRE(O)                                                                  \
    O(AUCUNE, aucune_raison, "aucune raison")                                                     \
    O(CHARGEMENT_FICHIER, chargement_fichier, "chargement fichier")                               \
    O(LEXAGE_FICHIER, lexage_fichier, "lexage fichier")                                           \
    O(PARSAGE_FICHIER, parsage_fichier, "parsage fichier")                                        \
    O(TYPAGE, typage, "typage")                                                                   \
    O(GENERATION_RI, generation_ri, "génération RI")                                              \
    O(EXECUTION, execution, "exécution")

enum class RaisonDEtre {
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine) Genre,
    ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
#undef ENUMERE_RAISON_D_ETRE_EX
};

const char *chaine_rainson_d_etre(RaisonDEtre raison_d_etre);
std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre);

struct UniteCompilation {
    // ------------- nouvelle interface
  private:
    RaisonDEtre m_raison_d_etre = RaisonDEtre::AUCUNE;
    bool m_prete = true;
    Attente m_attente = {};

  public:
    explicit UniteCompilation(EspaceDeTravail *esp) : espace(esp)
    {
    }

    void mute_attente(Attente attente)
    {
        m_attente = attente;
        m_prete = false;
    }

    void marque_prete()
    {
        m_prete = true;
        m_attente = {};
    }

    bool est_prete() const
    {
        return m_prete;
    }

    void mute_raison_d_etre(RaisonDEtre nouvelle_raison)
    {
        m_raison_d_etre = nouvelle_raison;
    }

    RaisonDEtre raison_d_etre() const
    {
        return m_raison_d_etre;
    }

    inline bool attend_sur_symbole(IdentifiantCode *ident)
    {
        return m_attente.attend_sur_symbole && m_attente.attend_sur_symbole->ident == ident;
    }

    inline bool attend_sur_message(Message *message)
    {
        return m_attente.attend_sur_message == message;
    }

    inline bool attend_sur_declaration(NoeudDeclaration *declaration)
    {
        return m_attente.attend_sur_declaration == declaration;
    }

    inline bool attend_sur_interface_kuri(IdentifiantCode *ident_interface)
    {
        return m_attente.attend_sur_interface_kuri == ident_interface;
    }

    inline bool attend_sur_type(Type *type)
    {
        return m_attente.attend_sur_type == type;
    }

#define DEFINIS_DISCRIMINATION(Genre, nom, chaine)                                                \
    inline bool est_pour_##nom() const                                                            \
    {                                                                                             \
        return m_raison_d_etre == RaisonDEtre::Genre;                                             \
    }

    ENUMERE_RAISON_D_ETRE(DEFINIS_DISCRIMINATION)

#undef DEFINIS_DISCRIMINATION

    // ------------- ancienne interface
    EspaceDeTravail *espace = nullptr;
    Fichier *fichier = nullptr;
    NoeudExpression *noeud = nullptr;
    MetaProgramme *metaprogramme = nullptr;
    bool message_recu = false;

    int index_courant = 0;
    int index_precedent = 0;
    int cycle = 0;

    bool est_bloquee() const;

    kuri::chaine commentaire() const;

    UniteCompilation *unite_attendue() const;
};

kuri::chaine chaine_attentes_recursives(UniteCompilation *unite);

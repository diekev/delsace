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

#include <iostream>

#include "structures/chaine.hh"

#include "attente.hh"

struct EspaceDeTravail;
struct Fichier;
struct MetaProgramme;
struct NoeudExpression;
struct Programme;

#define ENUMERE_RAISON_D_ETRE(O)                                                                  \
    O(AUCUNE, aucune_raison, "aucune raison")                                                     \
    O(CHARGEMENT_FICHIER, chargement_fichier, "chargement fichier")                               \
    O(LEXAGE_FICHIER, lexage_fichier, "lexage fichier")                                           \
    O(PARSAGE_FICHIER, parsage_fichier, "parsage fichier")                                        \
    O(CREATION_FONCTION_INIT_TYPE, creation_fonction_init_type, "création fonction init type")    \
    O(TYPAGE, typage, "typage")                                                                   \
    O(CONVERSION_NOEUD_CODE, conversion_noeud_code, "conversion noeud code")                      \
    O(ENVOIE_MESSAGE, envoie_message, "envoie message")                                           \
    O(GENERATION_RI, generation_ri, "génération RI")                                              \
    O(GENERATION_RI_PRINCIPALE_MP, generation_ri_principale_mp, "génération RI principale mp")    \
    O(EXECUTION, execution, "exécution")                                                          \
    O(LIAISON_PROGRAMME, liaison_programme, "liaison programme")                                  \
    O(GENERATION_CODE_MACHINE, generation_code_machine, "génération code machine")

enum class RaisonDEtre : unsigned char {
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine) Genre,
    ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
#undef ENUMERE_RAISON_D_ETRE_EX
};

const char *chaine_rainson_d_etre(RaisonDEtre raison_d_etre);
std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre);

struct UniteCompilation {
    int index_courant = 0;
    int index_precedent = 0;
    int cycle = 0;
    bool tag = false;
    bool annule = false;

  private:
    RaisonDEtre m_raison_d_etre = RaisonDEtre::AUCUNE;
    bool m_prete = true;
    Attente m_attente = {};

  public:
    EspaceDeTravail *espace = nullptr;
    Fichier *fichier = nullptr;
    NoeudExpression *noeud = nullptr;
    MetaProgramme *metaprogramme = nullptr;
    Programme *programme = nullptr;
    Message *message = nullptr;
    Type *type = nullptr;

    explicit UniteCompilation(EspaceDeTravail *esp) : espace(esp)
    {
    }

    void mute_attente(Attente attente)
    {
        m_attente = attente;
        m_prete = false;
        cycle = 0;
        assert(attente.est_valide());
    }

    void marque_prete()
    {
        m_prete = true;
        m_attente = {};
        cycle = 0;
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

    inline bool attend_sur_message(Message const *message_)
    {
        return m_attente.est<AttenteSurMessage>() && m_attente.message() == message_;
    }

    inline bool attend_sur_un_message() const
    {
        return m_attente.est<AttenteSurMessage>();
    }

    inline bool attend_sur_noeud_code(NoeudCode **code)
    {
        return m_attente.est<AttenteSurNoeudCode>() && m_attente.noeud_code() == code;
    }

#define DEFINIS_DISCRIMINATION(Genre, nom, chaine)                                                \
    inline bool est_pour_##nom() const                                                            \
    {                                                                                             \
        return m_raison_d_etre == RaisonDEtre::Genre;                                             \
    }

    ENUMERE_RAISON_D_ETRE(DEFINIS_DISCRIMINATION)

#undef DEFINIS_DISCRIMINATION

    bool est_bloquee() const;

    kuri::chaine commentaire() const;

    UniteCompilation *unite_attendue() const;

    void rapporte_erreur() const;

    void marque_prete_si_attente_resolue();

  private:
    bool attente_est_bloquee() const;
};

kuri::chaine chaine_attentes_recursives(UniteCompilation const *unite);

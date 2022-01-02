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

#include "message.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "unite_compilation.hh"

void Messagere::ajoute_message_fichier_ouvert(EspaceDeTravail *espace, const kuri::chaine &chemin)
{
    if (!interception_commencee) {
        return;
    }

    auto message = messages_fichiers.ajoute_element();
    message->genre = GenreMessage::FICHIER_OUVERT;
    message->espace = espace;
    message->chemin = chemin;

    file_message.enfile({nullptr, message});
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_fichier_ferme(EspaceDeTravail *espace, const kuri::chaine &chemin)
{
    if (!interception_commencee) {
        return;
    }

    auto message = messages_fichiers.ajoute_element();
    message->genre = GenreMessage::FICHIER_FERME;
    message->espace = espace;
    message->chemin = chemin;

    file_message.enfile({nullptr, message});
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module)
{
    if (!interception_commencee) {
        return;
    }

    auto message = messages_modules.ajoute_element();
    message->genre = GenreMessage::MODULE_OUVERT;
    message->espace = espace;
    message->chemin = module->chemin();
    message->module = module;

    file_message.enfile({nullptr, message});
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_module_ferme(EspaceDeTravail *espace, Module *module)
{
    if (!interception_commencee) {
        return;
    }

    auto message = messages_modules.ajoute_element();
    message->genre = GenreMessage::MODULE_FERME;
    message->espace = espace;
    message->chemin = module->chemin();
    message->module = module;

    file_message.enfile({nullptr, message});
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

Message *Messagere::ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    if (!interception_commencee) {
        return nullptr;
    }

    auto message = messages_typage_code.ajoute_element();
    message->genre = GenreMessage::TYPAGE_CODE_TERMINE;
    message->espace = espace;

    /* Les messages de typages ne sont pas directement envoyés. */

    return message;
}

void Messagere::envoie_message(Message *message)
{
    file_message.enfile(message);
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

Message *Messagere::ajoute_message_phase_compilation(EspaceDeTravail *espace)
{
    if (!interception_commencee) {
        return nullptr;
    }

    auto message = messages_phase_compilation.ajoute_element();
    message->genre = GenreMessage::PHASE_COMPILATION;
    message->espace = espace;
    message->phase = espace->phase_courante();

    file_message.enfile(message);
    pic_de_message = std::max(file_message.taille(), pic_de_message);

    return message;
}

long Messagere::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += messages_fichiers.memoire_utilisee();
    memoire += messages_modules.memoire_utilisee();
    memoire += messages_typage_code.memoire_utilisee();
    memoire += messages_phase_compilation.memoire_utilisee();
    memoire += pic_de_message * taille_de(void *);
    return memoire;
}

Message const *Messagere::defile()
{
    if (!interception_commencee) {
        return nullptr;
    }

    return file_message.defile();
}

void Messagere::commence_interception(EspaceDeTravail * /*espace*/)
{
    interception_commencee = true;
}

void Messagere::termine_interception(EspaceDeTravail * /*espace*/)
{
    interception_commencee = false;
    file_message.efface();
    m_compilatrice->gestionnaire_code->interception_message_terminee();
}

void Messagere::purge_messages()
{
    file_message.efface();
}

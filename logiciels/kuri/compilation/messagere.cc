/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "message.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "unite_compilation.hh"

void Messagère::ajoute_message_fichier_ouvert(EspaceDeTravail *espace,
                                              kuri::chaine_statique chemin)
{
    if (!interception_commencée) {
        return;
    }

    auto message = messages_fichiers.ajoute_élément();
    message->genre = GenreMessage::FICHIER_OUVERT;
    message->espace = espace;
    message->chemin = chemin;

    envoie_message(message);
}

void Messagère::ajoute_message_fichier_fermé(EspaceDeTravail *espace, kuri::chaine_statique chemin)
{
    if (!interception_commencée) {
        return;
    }

    auto message = messages_fichiers.ajoute_élément();
    message->genre = GenreMessage::FICHIER_FERMÉ;
    message->espace = espace;
    message->chemin = chemin;

    envoie_message(message);
}

void Messagère::ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module)
{
    if (!interception_commencée) {
        return;
    }

    auto message = messages_modules.ajoute_élément();
    message->genre = GenreMessage::MODULE_OUVERT;
    message->espace = espace;
    message->chemin = module->chemin();
    message->module = module;

    envoie_message(message);
}

void Messagère::ajoute_message_module_fermé(EspaceDeTravail *espace, Module *module)
{
    if (!interception_commencée) {
        return;
    }

    auto message = messages_modules.ajoute_élément();
    message->genre = GenreMessage::MODULE_FERMÉ;
    message->espace = espace;
    message->chemin = module->chemin();
    message->module = module;

    envoie_message(message);
}

Message *Messagère::ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    if (!interception_commencée) {
        return nullptr;
    }

    auto message = messages_typage_code.ajoute_élément();
    message->genre = GenreMessage::TYPAGE_CODE_TERMINÉ;
    message->espace = espace;

    /* Les messages de typages ne sont pas directement envoyés. */

    return message;
}

void Messagère::envoie_message(Message *message)
{
    file_message.enfile(message);
    pic_de_message = std::max(file_message.taille(), pic_de_message);
}

Message *Messagère::ajoute_message_phase_compilation(EspaceDeTravail *espace)
{
    if (!interception_commencée) {
        return nullptr;
    }

    auto message = messages_phase_compilation.ajoute_élément();
    message->genre = GenreMessage::PHASE_COMPILATION;
    message->espace = espace;
    message->phase = espace->phase_courante();

    envoie_message(message);

    return message;
}

int64_t Messagère::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += messages_fichiers.mémoire_utilisée();
    résultat += messages_modules.mémoire_utilisée();
    résultat += messages_typage_code.mémoire_utilisée();
    résultat += messages_phase_compilation.mémoire_utilisée();
    résultat += pic_de_message * taille_de(void *);
    return résultat;
}

Message const *Messagère::defile()
{
    if (!interception_commencée) {
        return nullptr;
    }

    return file_message.defile();
}

void Messagère::commence_interception(EspaceDeTravail * /*espace*/)
{
    interception_commencée = true;
}

void Messagère::termine_interception(EspaceDeTravail * /*espace*/)
{
    interception_commencée = false;
    purge_messages();
}

void Messagère::purge_messages()
{
    file_message.efface();
}

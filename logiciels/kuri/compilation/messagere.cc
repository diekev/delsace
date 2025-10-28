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
    if (entreceveurs == 0) {
        return;
    }

    auto message = messages_fichiers.ajoute_élément();
    message->genre = GenreMessage::FICHIER_OUVERT;
    message->espace = espace->id;
    message->chemin = chemin;

    envoie_message(message);
}

void Messagère::ajoute_message_fichier_fermé(EspaceDeTravail *espace, kuri::chaine_statique chemin)
{
    if (entreceveurs == 0) {
        return;
    }

    auto message = messages_fichiers.ajoute_élément();
    message->genre = GenreMessage::FICHIER_FERMÉ;
    message->espace = espace->id;
    message->chemin = chemin;

    envoie_message(message);
}

void Messagère::ajoute_message_module_ouvert(EspaceDeTravail *espace, Module *module)
{
    if (entreceveurs == 0) {
        return;
    }

    auto message = messages_modules.ajoute_élément();
    message->genre = GenreMessage::MODULE_OUVERT;
    message->espace = espace->id;
    message->chemin = module->chemin();
    message->module = module;

    envoie_message(message);
}

void Messagère::ajoute_message_module_fermé(EspaceDeTravail *espace, Module *module)
{
    if (entreceveurs == 0) {
        return;
    }

    auto message = messages_modules.ajoute_élément();
    message->genre = GenreMessage::MODULE_FERMÉ;
    message->espace = espace->id;
    message->chemin = module->chemin();
    message->module = module;

    envoie_message(message);
}

void Messagère::ajoute_message_espace_créé(EspaceDeTravail *espace, EspaceDeTravail *nouvel_espace)
{
    if (entreceveurs != 0) {
        auto message = messages_espace_créé.ajoute_élément();
        message->genre = GenreMessage::ESPACE_CRÉÉ;
        message->espace = espace->id;
        message->nouvel_espace = nouvel_espace->id;

        envoie_message(message);
    }
}

Message *Messagère::ajoute_message_typage_code(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    if (entreceveurs == 0) {
        return nullptr;
    }

    auto message = messages_typage_code.ajoute_élément();
    message->genre = GenreMessage::TYPAGE_CODE_TERMINÉ;
    message->espace = espace->id;

    /* Les messages de typages ne sont pas directement envoyés. */

    return message;
}

void Messagère::envoie_message(Message *message)
{
    POUR (métaprogrammes) {
        std::unique_lock verrou(it->mutex_file_message);
        it->file_message.enfile(message);
    }
}

Message *Messagère::ajoute_message_phase_compilation(EspaceDeTravail *espace)
{
    if (entreceveurs == 0) {
        return nullptr;
    }

    auto message = messages_phase_compilation.ajoute_élément();
    message->genre = GenreMessage::PHASE_COMPILATION;
    message->espace = espace->id;
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
    return résultat;
}

void Messagère::commence_interception(EspaceDeTravail * /*espace*/, MetaProgramme *métaprogramme)
{
    entreceveurs += 1;
    métaprogrammes.ajoute(métaprogramme);
}

void Messagère::termine_interception(EspaceDeTravail * /*espace*/, MetaProgramme *métaprogramme)
{
    assert(entreceveurs >= 1);
    entreceveurs -= 1;

    {
        std::unique_lock verrou(métaprogramme->mutex_file_message);
        métaprogramme->file_message.efface();
    }

    POUR_INDICE (métaprogrammes) {
        if (it == métaprogramme) {
            std::swap(métaprogrammes[indice_it], métaprogrammes[métaprogrammes.taille() - 1]);
            métaprogrammes.supprime_dernier();
            break;
        }
    }
}

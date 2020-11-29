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

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"
#include "modules.hh"

const char *chaine_phase_compilation(PhaseCompilation phase)
{
#define ENUMERE_PHASE(x) case PhaseCompilation::x: return #x;

	switch (phase) {
		ENUMERE_PHASES_COMPILATION
	}

#undef ENUMERE_PHASE
	return "PhaseCompilation inconnue";
}

std::ostream &operator<<(std::ostream &os, PhaseCompilation phase)
{
	os << chaine_phase_compilation(phase);
	return os;
}

void Messagere::ajoute_message_fichier_ouvert(EspaceDeTravail *espace, const kuri::chaine &chemin)
{
	if (!interception_commencee) {
		return;
	}

	auto message = messages_fichiers.ajoute_element();
	message->genre = GenreMessage::FICHIER_OUVERT;
	message->espace = espace;
	message->chemin = chemin;

	file_message.enfile({ nullptr, message });
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

	file_message.enfile({ nullptr, message });
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

	file_message.enfile({ nullptr, message });
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

	file_message.enfile({ nullptr, message });
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

bool Messagere::ajoute_message_typage_code(EspaceDeTravail *espace, NoeudDeclaration *noeud_decl, UniteCompilation *unite)
{
	if (!interception_commencee) {
		return false;
	}

	convertisseuse_noeud_code.converti_noeud_syntaxique(espace, noeud_decl);

	auto message = messages_typage_code.ajoute_element();
	message->genre = GenreMessage::TYPAGE_CODE_TERMINE;
	message->espace = espace;
	message->noeud_code = noeud_decl->noeud_code;

	assert(unite);

	file_message.enfile({ unite, message});
	pic_de_message = std::max(file_message.taille(), pic_de_message);

	return true;
}

void Messagere::ajoute_message_phase_compilation(EspaceDeTravail *espace)
{
	if (!interception_commencee) {
		return;
	}

	auto message = messages_phase_compilation.ajoute_element();
	message->genre = GenreMessage::PHASE_COMPILATION;
	message->espace = espace;
	message->phase = espace->phase_courante();

	file_message.enfile({ nullptr, message });
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

long Messagere::memoire_utilisee() const
{
	auto memoire = 0l;
	memoire += messages_fichiers.memoire_utilisee();
	memoire += messages_modules.memoire_utilisee();
	memoire += messages_typage_code.memoire_utilisee();
	memoire += messages_phase_compilation.memoire_utilisee();
	memoire += pic_de_message * taille_de(void *);
	memoire += convertisseuse_noeud_code.memoire_utilisee();
	return memoire;
}

Message const *Messagere::defile()
{
	if (!interception_commencee) {
		return nullptr;
	}

	auto message = file_message.defile();
	derniere_unite = message.unite;

	// marque le message comme reçu avant de l'envoyer pour éviter d'être bloqué dans les tâcheronnes
	// j'ai voulu faire en sorte que le message ne soit marqué comme reçu que quand nous envoyons le
	// message suivant, mais si ce message est le dernier, il n'y a pas de message suivant et les tâcheronnes
	// sont bloquées sur un message marqué comme non-reçu
	if (derniere_unite) {
		derniere_unite->message_recu = true;
	}

	return message.message;
}

void Messagere::commence_interception(EspaceDeTravail */*espace*/)
{
	interception_commencee = true;
}

void Messagere::termine_interception(EspaceDeTravail */*espace*/)
{
	interception_commencee = false;

	/* purge tous les messages puisque nous ne sommes plus écouté */
	while (!file_message.est_vide()) {
		auto m = file_message.defile();

		/* indique que le message a été reçu au cas où une tâcheronne serait en
		 * train d'essayer d'émettre un message */
		if (m.unite) {
			m.unite->message_recu = true;
		}
	}
}

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

void Messagere::ajoute_message_fichier_ouvert(const kuri::chaine &chemin)
{
	auto message = messages_fichiers.ajoute_element();
	message->genre = GenreMessage::FICHIER_OUVERT;
	message->chemin = chemin;

	file_message.enfile(message);
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_fichier_ferme(const kuri::chaine &chemin)
{
	auto message = messages_fichiers.ajoute_element();
	message->genre = GenreMessage::FICHIER_FERME;
	message->chemin = chemin;

	file_message.enfile(message);
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_module_ouvert(const kuri::chaine &chemin)
{
	auto message = messages_fichiers.ajoute_element();
	message->genre = GenreMessage::MODULE_OUVERT;
	message->chemin = chemin;

	file_message.enfile(message);
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_module_ferme(const kuri::chaine &chemin)
{
	auto message = messages_fichiers.ajoute_element();
	message->genre = GenreMessage::MODULE_FERME;
	message->chemin = chemin;

	file_message.enfile(message);
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

void Messagere::ajoute_message_typage_code(NoeudCode *noeud_code)
{
	auto message = messages_typage_code.ajoute_element();
	message->genre = GenreMessage::TYPAGE_CODE_TERMINE;
	message->noeud_code = noeud_code;

	file_message.enfile(message);
	pic_de_message = std::max(file_message.taille(), pic_de_message);
}

size_t Messagere::memoire_utilisee() const
{
	auto memoire = 0ul;
	memoire += messages_fichiers.memoire_utilisee();
	memoire += messages_typage_code.memoire_utilisee();
	memoire += static_cast<size_t>(pic_de_message) * sizeof(void *);
	return memoire;
}

Message const *Messagere::defile()
{
	return file_message.defile();
}

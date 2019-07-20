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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#include "morceaux.hh"

#include "biblinternes/structures/dico_fixe.hh"

static auto paires_mots_cles = dls::cree_dico(
	dls::paire{ dls::vue_chaine("dans"), ID_DANS },
	dls::paire{ dls::vue_chaine("finpour"), ID_FINPOUR },
	dls::paire{ dls::vue_chaine("finsi"), ID_FINSI },
	dls::paire{ dls::vue_chaine("pour"), ID_POUR },
	dls::paire{ dls::vue_chaine("si"), ID_SI },
	dls::paire{ dls::vue_chaine("sinon"), ID_SINON },
	dls::paire{ dls::vue_chaine("étend"), ID_ETEND }
);

const char *chaine_identifiant(int id)
{
	switch (id) {
		case ID_DANS:
			return "ID_DANS";
		case ID_FINPOUR:
			return "ID_FINPOUR";
		case ID_FINSI:
			return "ID_FINSI";
		case ID_POUR:
			return "ID_POUR";
		case ID_SI:
			return "ID_SI";
		case ID_SINON:
			return "ID_SINON";
		case ID_ETEND:
			return "ID_ETEND";
		case ID_DEBUT_VARIABLE:
			return "ID_DEBUT_VARIABLE";
		case ID_DEBUT_EXPRESSION:
			return "ID_DEBUT_EXPRESSION";
		case ID_FIN_VARIABLE:
			return "ID_FIN_VARIABLE";
		case ID_FIN_EXPRESSION:
			return "ID_FIN_EXPRESSION";
		case ID_CHAINE_CARACTERE:
			return "ID_CHAINE_CARACTERE";
		case ID_INCONNU:
			return "ID_INCONNU";
	};

	return "ERREUR";
}

static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_mots_cles[i] = false;
	}

    {
	    auto plg = paires_mots_cles.plage();

	    while (!plg.est_finie()) {
		    tables_mots_cles[static_cast<unsigned char>(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}
}

int id_chaine(const dls::vue_chaine &chaine)
{
	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return ID_CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return ID_CHAINE_CARACTERE;
}

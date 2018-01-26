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

#include "decoupeur.h"

#include <algorithm>
#include <iostream>

#include "erreur.h"
#include "morceaux.h"

//#define DEBOGUE_DECOUPEUR

namespace kangao {

/* étiquette : afficher une phrase dans l'interface
 *     étiquette(texte="Texte étiquette")
 *
 * curseur : afficher une barre de selection
 *     curseur(min="0"; max="100"; pas="1"; valeur="5"; attache=""; infobulle="")
 *
 * curseur_décimal : afficher une barre de selection
 *     curseur_décimal(min="0.0"; max="1.0"; pas="0.01"; precision="5"; valeur="5"; attache=""; infobulle="")
 *
 * liste : afficher une liste déroulante
 *     liste(valeur="jpeg"; items=[ { nom="", valeur=""}, ]; attache=""; infobulle="")
 *
 * case : afficher une case à cocher
 *     case(valeur="true"; attache=""; infobulle="")
 *
 * chaine :
 *     chaine(valeur=""; attache=""; infobulle="")
 *
 * selection fichier entrée :
 *     fichier_entree(valeur=""; attache=""; infobulle="")
 *
 * sélection fichier sortie :
 *     fichier_sortie(valeur=""; attache=""; infobulle="")
 *
 * sélection couleur :
 *     couleur(valeur="0,0,0"; attache=""; infobulle="")
 *
 * sélection vecteur :
 *     vecteur(valeur="0,0,0"; attache=""; infobulle="")
 *
 * bouton :
 *    bouton(valeur=""; attache=""; infobulle="")
 */

/* ************************************************************************** */

struct PaireIndentifiantChaine {
	int identifiant;
	std::string chaine;
};

static PaireIndentifiantChaine paires_identifiant[] = {
	{ IDENTIFIANT_DISPOSITION, "disposition" },
	{ IDENTIFIANT_MENU, "menu" },
	{ IDENTIFIANT_BARRE_OUTILS, "barre_outils" },
	{ IDENTIFIANT_COLONNE, "colonne" },
	{ IDENTIFIANT_LIGNE, "ligne" },
	{ IDENTIFIANT_DOSSIER, "dossier" },
	{ IDENTIFIANT_ONGLET, "onglet" },
	{ IDENTIFIANT_CONTROLE_ETIQUETTE, "étiquette" },
	{ IDENTIFIANT_ACCOLADE_OUVERTE, "{" },
	{ IDENTIFIANT_ACCOLADE_FERMEE, "}" },
	{ IDENTIFIANT_PARENTHESE_OUVERTE, "(" },
	{ IDENTIFIANT_PARENTHESE_FERMEE, ")" },
	{ IDENTIFIANT_CROCHET_OUVERT, "[" },
	{ IDENTIFIANT_CROCHET_FERME, "]" },
	{ IDENTIFIANT_EGAL, "=" },
	{ IDENTIFIANT_POINT_VIRGULE, ";" },
	{ IDENTIFIANT_VIRGULE, "," },
	{ IDENTIFIANT_CONTROLE_CURSEUR, "entier" },
	{ IDENTIFIANT_CONTROLE_CURSEUR_DECIMAL, "décimal" },
	{ IDENTIFIANT_CONTROLE_LISTE, "liste" },
	{ IDENTIFIANT_CONTROLE_CASE_COCHER, "case" },
	{ IDENTIFIANT_CONTROLE_CHAINE, "chaine" },
	{ IDENTIFIANT_CONTROLE_FICHIER_ENTREE, "fichier_entrée" },
	{ IDENTIFIANT_CONTROLE_FICHIER_SORTIE, "fichier_sortie" },
	{ IDENTIFIANT_CONTROLE_COULEUR, "couleur" },
	{ IDENTIFIANT_CONTROLE_VECTEUR, "vecteur" },
	{ IDENTIFIANT_CONTROLE_BOUTON, "bouton" },
	{ IDENTIFIANT_CONTROLE_ACTION, "action" },
	{ IDENTIFIANT_CONTROLE_SEPARATEUR, "séparateur" },
	{ IDENTIFIANT_PROPRIETE_INFOBULLE, "infobulle" },
	{ IDENTIFIANT_PROPRIETE_MIN, "min" },
	{ IDENTIFIANT_PROPRIETE_MAX, "max" },
	{ IDENTIFIANT_PROPRIETE_VALEUR, "valeur" },
	{ IDENTIFIANT_PROPRIETE_ATTACHE, "attache" },
	{ IDENTIFIANT_PROPRIETE_PRECISION, "précision" },
	{ IDENTIFIANT_PROPRIETE_PAS, "pas" },
	{ IDENTIFIANT_PROPRIETE_ITEM, "items" },
	{ IDENTIFIANT_PROPRIETE_NOM, "nom" },
	{ IDENTIFIANT_PROPRIETE_METADONNEE, "métadonnée" },
	{ IDENTIFIANT_PROPRIETE_ICONE, "icône" },
};

static bool comparaison_paires_identifiant(
		const PaireIndentifiantChaine &a,
		const PaireIndentifiantChaine &b)
{
	return a.chaine < b.chaine;
}

template <typename Iterateur, typename T, typename TypeComparaison>
Iterateur recherche_binaire(
		Iterateur debut,
		Iterateur fin,
		const T &valeur,
		const TypeComparaison &comparaison)
{
	auto iterateur = std::lower_bound(debut, fin, valeur, comparaison);

	if (iterateur != fin && !(*iterateur < valeur)) {
		return iterateur;
	}

	return fin;
}

static int trouve_identifiant(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
				std::begin(paires_identifiant),
				std::end(paires_identifiant),
				PaireIndentifiantChaine{IDENTIFIANT_NUL, chaine},
				comparaison_paires_identifiant);

	if (iterateur != std::end(paires_identifiant)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return IDENTIFIANT_NUL;
}

/* ************************************************************************** */

size_t trouve_taille_ligne(const std::string_view &chaine, size_t pos)
{
	const auto debut = pos;

	while (pos < chaine.size() && chaine[pos] != '\n') {
		++pos;
	}

	return pos - debut;
}

Decoupeur::Decoupeur(const std::string_view &chaine)
	: m_chaine(chaine)
	, m_position(0)
	, m_position_ligne(0)
	, m_ligne(0)
	, m_fini(false)
	, m_caractere_courant('\0')
{
	std::sort(std::begin(paires_identifiant),
			  std::end(paires_identifiant),
			  comparaison_paires_identifiant);

	m_lignes.push_back({&m_chaine[m_position],
						trouve_taille_ligne(m_chaine, m_position)});
}

void Decoupeur::impression_debogage(const std::string &quoi)
{
	std::cout << "Trouvé symbole " << quoi
			  << ", ligne : " << m_ligne
			  << ", position : " << m_position_ligne << ".\n";
}

void Decoupeur::saute_espaces_blancs()
{
	auto caractere = m_chaine[m_position];

	while (caractere == ' ' || caractere == '\n' || caractere == '\t') {
		++m_position;
		++m_position_ligne;

		if (m_position == m_chaine.size()) {
			m_fini = true;
			break;
		}

		if (caractere == '\n') {
			m_lignes.push_back({&m_chaine[m_position + 1],
								trouve_taille_ligne(m_chaine, m_position + 1)});

			++m_ligne;
			m_position_ligne = 0;
		}

		caractere = m_chaine[m_position];
	}
}

char Decoupeur::caractere_suivant()
{
	m_caractere_courant = m_chaine[m_position++];
	++m_position_ligne;

	if (m_position == m_chaine.size()) {
		m_fini = true;
	}

	return m_caractere_courant;
}

char Decoupeur::caractere_courant()
{
	return m_caractere_courant;
}

std::string Decoupeur::decoupe_chaine_litterale()
{
	std::string mot_courant;

	while (caractere_suivant() != '"') {
		mot_courant.push_back(caractere_courant());
	}

	return mot_courant;
}

void Decoupeur::decoupe()
{
#ifdef DEBOGUE_DECOUPEUR
	std::cout << "Démarrage de l'analyse d'une chaîne de "
			  << m_chaine.size() << " caractères.\n";
#endif

	std::string mot_courant;
	m_ligne = 0;
	m_position = 0;
	m_position_ligne = 0;
	int identifiant = IDENTIFIANT_NUL;

	while (!m_fini) {
		saute_espaces_blancs();
		const auto caractere = caractere_suivant();

		if (caractere == '"') {
			if (!mot_courant.empty()) {
				throw ErreurFrappe(
							m_lignes[m_ligne],
							m_ligne,
							m_position_ligne,
							"Identifiant inconnu !");
			}

			mot_courant = decoupe_chaine_litterale();
			identifiant = IDENTIFIANT_CHAINE_CARACTERE;
		}
		else {
			mot_courant.push_back(caractere);
			identifiant = trouve_identifiant(mot_courant);
		}

		if (identifiant != IDENTIFIANT_NUL) {
#ifdef DEBOGUE_DECOUPEUR
			impression_debogage(mot_courant);
#endif

			ajoute_identifiant(
						identifiant,
						m_lignes[m_ligne],
						m_ligne,
						m_position_ligne,
						mot_courant);

			mot_courant = "";
		}
	}

#ifdef DEBOGUE_DECOUPEUR
	std::cout << "Fin de l'analyse. Ligne : " << m_ligne
			  << ". Position : " << m_position << ".\n";
#endif

	if (!mot_courant.empty()) {
		throw ErreurFrappe(
					m_lignes[m_ligne],
					m_ligne,
					m_position_ligne,
					"Le script ne semble pas terminé !");
	}
}

void Decoupeur::ajoute_identifiant(int identifiant, const std::string_view &ligne, int numero_ligne, int position_ligne, const std::string &contenu)
{
	m_identifiants.push_back({identifiant, numero_ligne, position_ligne, contenu, ligne});
}

const std::vector<DonneesMorceaux> &Decoupeur::morceaux() const
{
	return m_identifiants;
}

}  /* namespace kangao */

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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "bibliotheque.hh"

#include "arbre_syntaxique.hh"
#include "espace_de_travail.hh"

static bool fichier_existe(kuri::chaine const &chemin)
{
	const auto std_string = std::string(chemin.pointeur(), static_cast<size_t>(chemin.taille()));
	const auto std_path = std::filesystem::path(std_string);

	if (std::filesystem::exists(std_path)) {
		return true;
	}

	return false;
}

bool Symbole::charge(EspaceDeTravail *espace, NoeudExpression const *site)
{
	if (etat_recherche == EtatRechercheSymbole::TROUVE) {
		return true;
	}

	if (bibliotheque->etat_recherche != EtatRechercheBibliotheque::TROUVEE) {
		if (!bibliotheque->charge(espace)) {
			return false;
		}
	}

	try {
		auto ptr_symbole = bibliotheque->bib(dls::chaine(nom.pointeur(), nom.taille()));
		this->ptr_fonction = reinterpret_cast<Symbole::type_fonction>(ptr_symbole.ptr());
		etat_recherche = EtatRechercheSymbole::TROUVE;
	}
	catch (...) {
		espace->rapporte_erreur(site, "Impossible de trouver un symbole !")
				.ajoute_message("La bibliothèque « ", bibliotheque->ident->nom, " » ne possède pas le symbole « ", nom, " » !\n");
		etat_recherche = EtatRechercheSymbole::INTROUVE;
		return false;
	}

	return true;
}

Symbole *Bibliotheque::cree_symbole(kuri::chaine_statique nom_symbole)
{
	POUR_TABLEAU_PAGE (symboles) {
		if (it.nom == nom_symbole) {
			return &it;
		}
	}

	auto symbole = symboles.ajoute_element();
	symbole->nom = nom_symbole;
	symbole->bibliotheque = this;
	return symbole;
}

bool Bibliotheque::charge(EspaceDeTravail *espace)
{
	if (etat_recherche == EtatRechercheBibliotheque::TROUVEE) {
		return true;
	}

	if (chemin_dynamique == "") {
		espace->rapporte_erreur(site, "Impossible de charger une bibliothèque dynamique pour l'exécution du code")
				.ajoute_message("La bibliothèque « ", ident->nom, " » n'a pas de version dynamique !\n");
		return false;
	}

	try {
		this->bib = dls::systeme_fichier::shared_library(dls::chaine(chemin_dynamique).c_str());
		etat_recherche = EtatRechercheBibliotheque::TROUVEE;
	}
	catch (...) {
		espace->rapporte_erreur(site, "Impossible de charger la bibliothèque !\n");
		etat_recherche = EtatRechercheBibliotheque::INTROUVEE;
		return false;
	}

	return true;
}

Bibliotheque *GestionnaireBibliotheques::trouve_bibliotheque(IdentifiantCode *ident)
{
	POUR_TABLEAU_PAGE (bibliotheques) {
		if (it.ident == ident) {
			return &it;
		}
	}

	return nullptr;
}

Bibliotheque *GestionnaireBibliotheques::trouve_ou_cree_bibliotheque(IdentifiantCode *ident)
{
	return cree_bibliotheque(nullptr, ident, "");
}

Bibliotheque *GestionnaireBibliotheques::cree_bibliotheque(NoeudExpression *site)
{
	return cree_bibliotheque(site, site->ident, "");
}

Bibliotheque *GestionnaireBibliotheques::cree_bibliotheque(NoeudExpression *site, IdentifiantCode *ident, kuri::chaine_statique nom)
{
	auto bibliotheque = trouve_bibliotheque(ident);

	if (bibliotheque) {
		if (nom != "" && bibliotheque->etat_recherche == EtatRechercheBibliotheque::NON_RECHERCHEE) {
			bibliotheque->site = site;
			bibliotheque->ident = ident;
			bibliotheque->nom = nom;
			resoud_chemins_bibliotheque(site, bibliotheque);
		}

		return bibliotheque;
	}

	bibliotheque = bibliotheques.ajoute_element();

	if (nom != "") {
		bibliotheque->nom = nom;
		resoud_chemins_bibliotheque(site, bibliotheque);
	}

	bibliotheque->site = site;
	bibliotheque->ident = ident;
	return bibliotheque;
}

void GestionnaireBibliotheques::resoud_chemins_bibliotheque(NoeudExpression *site, Bibliotheque *bibliotheque)
{
	// regarde soit dans le module courant, soit dans le chemin système
	// chemin_système : /lib/x86_64-linux-gnu/ pour 64-bit
	//                  /lib/i386-linux-gnu/ pour 32-bit

	dls::tablet<kuri::chaine_statique, 2> dossiers;
	dossiers.ajoute("/lib/x86_64-linux-gnu/");
	dossiers.ajoute("/usr/lib/x86_64-linux-gnu/");
	// pour les tables r16...
	dossiers.ajoute("/tmp/lib/x86_64-linux-gnu/");

	if (site) {
		const auto fichier = espace.fichier(site->lexeme->fichier);
		const auto module = fichier->module;
		dossiers.ajoute(module->nom()->nom);
	}

	// essaye de déterminer le chemin
	// pour un fichier statique :
	// /chemin/de/base/libnom.a
	// pour un fichier dynamique :
	// /chemin/de/base/libnom.so

	const auto nom_statique = enchaine("lib", bibliotheque->nom, ".a");
	const auto nom_dynamique = enchaine("lib", bibliotheque->nom, ".so");

	kuri::chaine chemin_statique;
	kuri::chaine chemin_dynamique;

	auto chemin_statique_trouve = false;
	auto chemin_dynamique_trouve = false;

	POUR (dossiers) {
		if (!chemin_statique_trouve) {
			const auto chemin_statique_test = enchaine(it, nom_statique);
			if (fichier_existe(chemin_statique_test)) {
				chemin_statique_trouve = true;
				chemin_statique = chemin_statique_test;
			}
		}

		if (!chemin_dynamique_trouve) {
			const auto chemin_dynamique_test = enchaine(it, nom_dynamique);
			if (fichier_existe(chemin_dynamique_test)) {
				chemin_dynamique_trouve = true;
				chemin_dynamique = chemin_dynamique_test;
			}
		}
	}

	if (!chemin_statique_trouve && !chemin_dynamique_trouve) {
		espace.rapporte_erreur(site, "Impossible de résoudre le chemin vers une bibliothèque")
				.ajoute_message("La bibliothèque en question est « ", bibliotheque->nom, " »\n");
		return;
	}

	std::cerr << "Création d'une bibliothèque pour " << bibliotheque->nom << '\n';
	std::cerr << "-- chemin statique  : " << chemin_statique << '\n';
	std::cerr << "-- chemin dynamique : " << chemin_dynamique << '\n';
	bibliotheque->chemin_statique = chemin_statique;
	bibliotheque->chemin_dynamique = chemin_dynamique;
}

void GestionnaireBibliotheques::rassemble_statistiques(Statistiques &stats) const
{
	// À FAIRE
}

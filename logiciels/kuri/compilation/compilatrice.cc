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

#include "compilatrice.hh"

#include <stdarg.h>

#include "biblinternes/flux/outils.h"

#include "erreur.h"
#include "lexeuse.hh"
#include "statistiques.hh"

/* ************************************************************************** */

static Compilatrice *ptr_compilatrice = nullptr;

Compilatrice::Compilatrice()
	: ordonnanceuse(this)
{
	this->bibliotheques_dynamiques->ajoute("pthread");
	this->definitions->ajoute("_REENTRANT");

	ptr_compilatrice = this;
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace, const kuri::chaine &nom, NoeudExpression const *site)
{
	auto chemin = dls::chaine(nom.pointeur(), nom.taille());

	if (!std::filesystem::exists(chemin.c_str())) {
		/* essaie dans la racine kuri */
		chemin = dls::chaine(racine_kuri.pointeur(), racine_kuri.taille()) + "/modules/" + chemin;

		if (!std::filesystem::exists(chemin.c_str())) {
			erreur::lance_erreur(
						"Impossible de trouver le dossier correspondant au module",
						*espace,
						site,
						erreur::Genre::MODULE_INCONNU);
		}
	}

	if (!std::filesystem::is_directory(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du module ne pointe pas vers un dossier",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
	auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
	auto nom_dossier = chemin_absolu.filename();

	// @concurrence critique
	auto module = espace->trouve_ou_cree_module(sys_module, ptr_compilatrice->table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier.c_str()), chemin_absolu.c_str());

	if (module->importe) {
		return module;
	}

	module->importe = true;

	messagere->ajoute_message_module_ouvert(espace, module);

	for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
		auto chemin_entree = entree.path();

		if (!std::filesystem::is_regular_file(chemin_entree)) {
			continue;
		}

		if (chemin_entree.extension() != ".kuri") {
			continue;
		}

		auto resultat = espace->trouve_ou_cree_fichier(sys_module, module, chemin_entree.stem().c_str(), chemin_entree.c_str(), importe_kuri);

		if (resultat.tag_type() == FichierNeuf::tag) {
			ordonnanceuse->cree_tache_pour_chargement(espace, resultat.t2().fichier);
		}
	}

	if (module->nom() == ID::Kuri) {
		auto resultat = espace->trouve_ou_cree_fichier(sys_module, module, "constantes", "constantes.kuri", false);

		if (resultat.tag_type() == FichierNeuf::tag) {
			auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
			if (!donnees_fichier->fut_charge) {
				const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
				donnees_fichier->charge_tampon(lng::tampon_source(source));
			}

			ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
		}
	}

	messagere->ajoute_message_module_ferme(espace, module);

	return module;
}

/* ************************************************************************** */

dls::chaine charge_fichier(
		const dls::chaine &chemin,
		EspaceDeTravail &espace,
		NoeudExpression const *site)
{
	std::ifstream fichier;
	fichier.open(chemin.c_str());

	if (!fichier.is_open()) {
		erreur::lance_erreur(
					"Impossible d'ouvrir le fichier correspondant au module",
					espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	return dls::chaine(std::istreambuf_iterator<char>(fichier), std::istreambuf_iterator<char>());
}

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace, const kuri::chaine &nom, Module *module, NoeudExpression const *site)
{
	auto chemin = dls::chaine(module->chemin()) + dls::chaine(nom) + ".kuri";

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au module",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	/* trouve le chemin absolu du fichier */
	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, nom, chemin_absolu.c_str(), importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		ordonnanceuse->cree_tache_pour_chargement(espace, resultat.t2().fichier);
	}
}

/* ************************************************************************** */

long Compilatrice::memoire_utilisee() const
{
	auto memoire = taille_de(Compilatrice);

	memoire += bibliotheques_dynamiques->taille() * taille_de(kuri::chaine);
	POUR (*bibliotheques_dynamiques.verrou_lecture()) {
		memoire += it.taille();
	}

	memoire += bibliotheques_statiques->taille() * taille_de(kuri::chaine);
	POUR (*bibliotheques_statiques.verrou_lecture()) {
		memoire += it.taille();
	}

	memoire += chemins->taille() * taille_de(dls::vue_chaine_compacte);
	memoire += definitions->taille() * taille_de(dls::vue_chaine_compacte);

	memoire += ordonnanceuse->memoire_utilisee();
	memoire += table_identifiants->memoire_utilisee();

	memoire += gerante_chaine->memoire_utilisee();

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		memoire += it.memoire_utilisee();
	}

	memoire += messagere->memoire_utilisee();

	memoire += sys_module->memoire_utilisee();

	return memoire;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
	stats.memoire_compilatrice = memoire_utilisee();

	POUR_TABLEAU_PAGE ((*espaces_de_travail.verrou_lecture())) {
		it.rassemble_statistiques(stats);
	}

	stats.nombre_identifiants = table_identifiants->taille();

	sys_module->rassemble_stats(stats);
}

/* ************************************************************************** */

EspaceDeTravail *Compilatrice::demarre_un_espace_de_travail(OptionsCompilation const &options, const kuri::chaine &nom)
{
	auto espace = espaces_de_travail->ajoute_element(options);
	espace->nom = nom;

	importe_module(espace, "Kuri", {});

	return espace;
}

/* ************************************************************************** */

OptionsCompilation *obtiens_options_compilation()
{
	return &ptr_compilatrice->espace_de_travail_defaut->options;
}

void ajourne_options_compilation(OptionsCompilation *options)
{
	ptr_compilatrice->espace_de_travail_defaut->options = *options;
}

void compilatrice_ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
	auto chaine = dls::chaine(c.pointeur(), c.taille());

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->ajoute(kuri::chaine(c.pointeur(), c.taille()));

	auto module = espace->trouve_ou_cree_module(ptr_compilatrice->sys_module, ID::chaine_vide, "");
	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, "métaprogramme", "", ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
		if (!donnees_fichier->fut_charge) {
			donnees_fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
		}
		ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
	}
}

void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine_statique c)
{
	auto chaine = dls::chaine(c.pointeur(), c.taille());

	ptr_compilatrice->chaines_ajoutees_a_la_compilation->ajoute(kuri::chaine(c.pointeur(), c.taille()));

	auto resultat = espace->trouve_ou_cree_fichier(ptr_compilatrice->sys_module, module, "métaprogramme", "", ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierNeuf::tag) {
		auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
		if (!donnees_fichier->fut_charge) {
			donnees_fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
		}
		ptr_compilatrice->ordonnanceuse->cree_tache_pour_lexage(espace, resultat.t2().fichier);
	}
}

void compilatrice_ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
	auto vue = dls::chaine(c.pointeur(), c.taille());
	auto chemin = std::filesystem::current_path() / vue.c_str();

	if (!std::filesystem::exists(chemin)) {
		std::cerr << "Le fichier " << chemin << " n'existe pas !\n";
		ptr_compilatrice->possede_erreur = true;
		return;
	}

	auto module = espace->trouve_ou_cree_module(ptr_compilatrice->sys_module, ID::chaine_vide, "");
	ptr_compilatrice->ajoute_fichier_a_la_compilation(espace, chemin.stem().c_str(), module, {});
}

// fonction pour tester les appels de fonctions variadiques externe dans la machine virtuelle
int fonction_test_variadique_externe(int sentinel, ...)
{
	va_list ap;
	va_start(ap, sentinel);

	int i = 0;
	for (;; ++i) {
		int t = va_arg(ap, int);

		if (t == sentinel) {
			break;
		}
	}

	va_end(ap);

	return i;
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y retourner le message disponible */
Message const *compilatrice_attend_message()
{
	assert(false);
	return nullptr;
}

EspaceDeTravail *demarre_un_espace_de_travail(kuri::chaine_statique nom, OptionsCompilation *options)
{
	return ptr_compilatrice->demarre_un_espace_de_travail(*options, kuri::chaine(nom.pointeur(), nom.taille()));
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y renseigner dans l'espace le métaprogramme en cours
 * d'exécution */
void compilatrice_commence_interception(EspaceDeTravail * /*espace*/)
{
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les
 * MachineVirtuelles, et y vérifier que le métaprogramme terminant l'interception
 * est bel et bien celui l'ayant commencé */
void compilatrice_termine_interception(EspaceDeTravail * /*espace*/)
{
}

EspaceDeTravail *espace_defaut_compilation()
{
	return ptr_compilatrice->espace_de_travail_defaut;
}

void compilatrice_rapporte_erreur(EspaceDeTravail *espace, kuri::chaine_statique fichier, int ligne, kuri::chaine_statique message)
{
	::rapporte_erreur(espace, fichier, ligne, message);
}

static kuri::tableau<kuri::Lexeme> converti_tableau_lexemes(kuri::tableau<Lexeme, int> const &lexemes)
{
	auto resultat = kuri::tableau<kuri::Lexeme>();
	resultat.reserve(lexemes.taille());

	POUR (lexemes) {
		resultat.ajoute({ static_cast<int>(it.genre), it.chaine });
	}

	return resultat;
}

kuri::tableau<kuri::Lexeme> compilatrice_lexe_fichier(kuri::chaine_statique chemin_donne, NoeudExpression const *site)
{
	auto espace = ptr_compilatrice->espace_de_travail_defaut;
	auto chemin = dls::chaine(chemin_donne.pointeur(), chemin_donne.taille());

	if (!std::filesystem::exists(chemin.c_str())) {
		erreur::lance_erreur(
					"Impossible de trouver le fichier correspondant au chemin",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	if (!std::filesystem::is_regular_file(chemin.c_str())) {
		erreur::lance_erreur(
					"Le nom du fichier ne pointe pas vers un fichier régulier",
					*espace,
					site,
					erreur::Genre::MODULE_INCONNU);
	}

	auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

	auto module = espace->module(ID::chaine_vide);

	auto resultat = espace->trouve_ou_cree_fichier(
				ptr_compilatrice->sys_module,
				module,
				chemin_absolu.stem().c_str(),
				chemin_absolu.c_str(),
				ptr_compilatrice->importe_kuri);

	if (resultat.tag_type() == FichierExistant::tag) {
		auto donnees_fichier = resultat.t1().fichier->donnees_constantes;
		return converti_tableau_lexemes(donnees_fichier->lexemes);
	}

	auto donnees_fichier = resultat.t2().fichier->donnees_constantes;
	auto tampon = charge_fichier(chemin, *espace, site);
	donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

	auto lexeuse = Lexeuse(*ptr_compilatrice, donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
	lexeuse.performe_lexage();

	return converti_tableau_lexemes(donnees_fichier->lexemes);
}

/* cette fonction est symbolique, afin de pouvoir la détecter dans les Machines Virtuelles, et y retourner l'espace du métaprogramme */
EspaceDeTravail *compilatrice_espace_courant()
{
	assert(false);
	return nullptr;
}

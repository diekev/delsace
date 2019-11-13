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

#pragma once

#include <thread>

#include "biblinternes/patrons_conception/observation.hh"
#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

#include "base_de_donnees.hh"

#include "chef_execution.hh"
#include "gestionnaire_fichier.hh"
#include "usine_operatrice.h"

#include "evaluation/reseau.hh"

class BaseEditrice;
class FenetrePrincipale;
class Graphe;
class Manipulatrice3D;
class Manipulatrice2D;
class Noeud;
class ProjectSettings;
class RepondantCommande;
class TaskNotifier;

namespace vision {
class Camera2D;
class Camera3D;
}  /* namespace vision */

namespace danjo {
class GestionnaireInterface;
}  /* namespace danjo */

namespace lcc {
struct LCC;
}

enum {
	FICHIER_OUVERTURE,
	FICHIER_SAUVEGARDE,
};

struct Jorjala : public Sujette {
private:
	UsineCommande m_usine_commande{};
	UsineOperatrice m_usine_operatrices;
	RepondantCommande *m_repondant_commande = nullptr;

	dls::tableau<dls::chaine> m_fichiers_recents{};
	dls::chaine m_chemin_projet{};

	bool m_projet_ouvert = false;

public:
	Jorjala();
	~Jorjala();

	/* Les usines et le répondant commande peuvent et doivent être partagés. */
	Jorjala(Jorjala const &autre) = default;
	Jorjala &operator=(Jorjala const &autre) = default;

	void initialise();

	UsineCommande &usine_commandes();

	UsineOperatrice &usine_operatrices();

	dls::chaine requiers_dialogue(int type, dls::chaine const &filtre);
	void affiche_erreur(dls::chaine const &message);

	dls::chaine chemin_projet() const;

	void chemin_projet(dls::chaine const &chemin);

	dls::tableau<dls::chaine> const &fichiers_recents();
	void ajoute_fichier_recent(dls::chaine const &chemin);

	bool projet_ouvert() const;

	void projet_ouvert(bool ouinon);

	RepondantCommande *repondant_commande() const;

	/* entreface */
	FenetrePrincipale *fenetre_principale;
	BaseEditrice *editrice_active;
	danjo::GestionnaireInterface *gestionnaire_entreface;

	/* Preferences and settings. */
	ProjectSettings *project_settings;

	/* Information de sortie. */
	dls::chaine chemin_sortie = "";
	dls::chaine nom_calque_sortie = "";

	/* vue 2d */
	vision::Camera2D *camera_2d = nullptr;

	/* vue 3d */
	vision::Camera3D *camera_3d = nullptr;

	/* temps */
	int temps_debut = 1;
	int temps_courant = 1;
	int temps_fin = 250;
	double cadence = 24.0;
	bool animation = false;

	/* contexte graphe */
	Graphe *graphe = nullptr;
	Noeud *noeud = nullptr;

	/* manipulation objets 2d */
	bool manipulation_2d_activee = false;
	int type_manipulation_2d = 0;
	Manipulatrice2D *manipulatrice_2d = nullptr;

	/* manipulation objets 3d */
	bool manipulation_3d_activee = false;
	int type_manipulation_3d = 0;
	Manipulatrice3D *manipulatrice_3d = nullptr;

	/* chemin du graphe courant */
	dls::chaine chemin_courant = "";

	/* pour les tâches */
	bool tache_en_cours = false;
	bool interrompu = false;

	/* thread utilisé pour jouer des animations */
	std::thread *thread_animation{};

	TaskNotifier *notifiant_thread{};

	GestionnaireFichier gestionnaire_fichier{};

	ChefExecution chef_execution;

	BaseDeDonnees bdd{};

	/* Pour la compilation des scripts LCC */
	lcc::LCC *lcc = nullptr;

	/* pour l'évaluation du graphe d'objets */
	Reseau reseau{};

	void ajourne_pour_nouveau_temps(const char *message);

	struct EtatLogiciel {
		BaseDeDonnees bdd{};
	};

	dls::pile<EtatLogiciel> pile_defait{};
	dls::pile<EtatLogiciel> pile_refait{};

	EtatLogiciel etat_courant();

	void empile_etat();

	void defait();

	void refait();
};

inline Jorjala *extrait_jorjala(std::any const &any)
{
	return std::any_cast<Jorjala *>(any);
}

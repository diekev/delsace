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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/resultat.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include <any>

#include "image.hh"

#include "danjo/manipulable.h"

#include "outils_entreface.hh"

enum class type_prise : int;

class ChefExecution;
class Corps;
class Graphe;
class Manipulatrice3D;
class Noeud;
class PriseEntree;
class CompilatriceReseau;
class NoeudReseau;
class UsineOperatrice;

struct ContexteEvaluation;
struct DonneesAval;

enum class res_exec : int {
	REUSSIE = 0,
	ECHOUEE = 1,
};

enum {
	OPERATRICE_IMAGE,
	OPERATRICE_CORPS,
	OPERATRICE_GRAPHE_DETAIL,
	OPERATRICE_DETAIL,
	OPERATRICE_SIMULATION,
};

/* ************************************************************************** */

/**
 * La class EntreeOperatrice est une simple enveloppe autour d'une PriseEntree
 * afin de séparer certaines logiques de traverse du graphe. L'exécution du
 * graphe se fait de manière récursive à travers ces entrées en ce que chaque
 * entrée appelle la fonction d'exécution du noeud connecté à celle-ci, noeud
 * dont les entrées prendront le relais pour continuer l'exécution du graphe.
 */
class EntreeOperatrice {
	PriseEntree *m_ptr = nullptr;
	dls::tableau<dls::chaine> m_liste_noms_calques{};

public:
	/**
	 * Crée une EntreeOperatrice sans pointeur associé, ce constructeur n'existe
	 * que pour pouvoir stocker des EntreeOperatrices dans des conteneurs et
	 * n'est pas à être utilisé autrement.
	 */
	EntreeOperatrice() = default;

	EntreeOperatrice(EntreeOperatrice const &autre) = default;
	EntreeOperatrice &operator=(EntreeOperatrice const &autre) = default;

	/**
	 * Crée une instance d'EntreeOperatrice pour la PriseEntree spécifiée.
	 */
	explicit EntreeOperatrice(PriseEntree *prise);

	/**
	 * Retourne vrai si la prise est connectée.
	 */
	bool connectee() const;

	/**
	 * Retourne le nombre de connexions du lien pointé.
	 */
	long nombre_connexions() const;

	/**
	 * Requiers l'image du noeud connecté à cette prise en exécutant ledit noeud
	 * avant de lui prendre son image. La liste de nom de calque est mise à jour
	 * selon l'image obtenue.
	 */	
	Image const *requiers_image(ContexteEvaluation const &contexte, DonneesAval *donnees_aval, int index = 0);

	Image *requiers_copie_image(Image &image, ContexteEvaluation const &contexte, DonneesAval *donnees_aval, int index = 0);

	const Corps *requiers_corps(ContexteEvaluation const &contexte, DonneesAval *donnees_aval, int index = 0);

	Corps *requiers_copie_corps(Corps *corps, ContexteEvaluation const &contexte, DonneesAval *donnees_aval, int index = 0);

	/**
	 * Place la liste de calque de l'image transitante par cette entrée dans le
	 * vecteur de chaines spécifié.
	 */
	void obtiens_liste_calque(dls::tableau<dls::chaine> &chaines) const;

	/**
	 * Place la liste d'attributs du corps transitant par cette entrée dans le
	 * vecteur de chaines spécifié.
	 */
	void obtiens_liste_attributs(dls::tableau<dls::chaine> &chaines) const;

	/**
	 * Place la liste de groupes de primitives du corps transitant par cette
	 * entrée dans le vecteur de chaines spécifié.
	 */
	void obtiens_liste_groupes_prims(dls::tableau<dls::chaine> &chaines) const;

	/**
	 * Place la liste de groupes de points du corps transitant par cette entrée
	 * dans le vecteur de chaines spécifié.
	 */
	void obtiens_liste_groupes_points(dls::tableau<dls::chaine> &chaines) const;

	PriseEntree *pointeur();

	void signale_cache(ChefExecution *chef) const;
};

struct PriseSortie;

class SortieOperatrice {
	PriseSortie *m_ptr = nullptr;

public:
	SortieOperatrice() = default;

	explicit SortieOperatrice(PriseSortie *prise);

	SortieOperatrice(SortieOperatrice const &autre) = default;

	PriseSortie *pointeur();
};

/* ************************************************************************** */

class OperatriceImage : public danjo::Manipulable {
	long m_num_inputs = 2;
	long m_num_outputs = 1;

	dls::tableau<EntreeOperatrice> m_input_data{};
	dls::tableau<SortieOperatrice> m_sorties{};

	dls::tableau<dls::chaine> m_avertissements{};

	UsineOperatrice *m_usine = nullptr;

protected:
	Graphe &m_graphe_parent;
	Noeud &noeud;
	Image m_image{};
	bool m_execute_toujours = false;

	REMBOURRE(6);

public:
	/* Prevent creating an operator without an accompanying node. */
	OperatriceImage() = delete;

	OperatriceImage(Graphe &graphe_parent, Noeud &n);

	OperatriceImage(OperatriceImage const &) = default;
	OperatriceImage &operator=(OperatriceImage const &) = default;

	virtual ~OperatriceImage() = default;

	bool cache_est_invalide = true;

	/* L'usine est utilisé pour pouvoir supprimer correctement l'opératrice.
	 * Voir supprime_operatrice_image. */
	void usine(UsineOperatrice *usine_op);
	UsineOperatrice *usine() const;

	bool execute_toujours() const;

	virtual int type() const;

	/* gestion des entrées */

	void entrees(long number);

	long entrees() const;

	virtual const char *nom_entree(int n);

	virtual type_prise type_entree(int n) const;

	virtual bool connexions_multiples(int n) const;

	EntreeOperatrice *entree(long index);

	const EntreeOperatrice *entree(long index) const;

	void donnees_entree(long index, PriseEntree *socket);

	/* gestion des sorties */

	void sorties(long number);

	long sorties() const;

	virtual const char *nom_sortie(int n);

	virtual type_prise type_sortie(int n) const;

	SortieOperatrice *sortie(long index);

	void donnees_sortie(long index, PriseSortie *prise);

	/* informations sur cette opératrice */

	virtual const char *nom_classe() const = 0;
	virtual const char *texte_aide() const = 0;
	virtual ResultatCheminEntreface chemin_entreface() const;

	/* la logique principale d'exécution de cette opératrice */

	virtual res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) = 0;

	void transfere_image(Image &image);

	void ajoute_avertissement(dls::chaine const &avertissement);

	template <typename T, typename... Ts>
	void ajoute_avertissement(T &&t, Ts &&... ts)
	{
		auto flux = dls::flux_chaine();
		ajoute_avertissement_flux(flux, t, ts...);
		m_avertissements.ajoute(flux.chn());
	}

	void reinitialise_avertisements();

	dls::tableau<dls::chaine> const &avertissements() const;

	Image *image();

	Image const *image() const;

	virtual Corps *corps();

	/**
	 * Retourne vrai si l'opératrice possède une manipulatrice 3d du type
	 * spécifié.
	 */
	virtual bool possede_manipulatrice_3d(int type) const;

	/**
	 * Retourne un pointeur vers la manipulatrice 3D de cette opératrice du
	 * type spécifié, si l'opératrice en possède une. Autrement, retourne un
	 * pointeur nul. Il est considéré que la manipulatrice retournée soit à jour
	 * par rapport aux données qu'elle est censée modifier (position, rotation,
	 * etc.).
	 */
	virtual Manipulatrice3D *manipulatrice_3d(int type);

	/**
	 * Ajourne les propriétés de cette opératrice en fonction de sa
	 * manipulatrice 3D du type spécifié. Cette méthode est appelée à chaque que
	 * la manipulatrice est modifiée dans la scène 3D.
	 */
	virtual void ajourne_selon_manipulatrice_3d(int type, const int temps);

	/**
	 * Obtiens la liste de calque en entrée de l'opératrice selon l'attache
	 * spécifiée.
	 */
	virtual void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &attache,
			dls::tableau<dls::chaine> &chaines);

	virtual void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau);

	virtual bool depend_sur_temps() const;

	virtual void amont_change(PriseEntree *entree);

	virtual void parametres_changes();

	virtual void libere_memoire();

private:
	template <typename T>
	void ajoute_avertissement_flux(dls::flux_chaine &flux, T &&t)
	{
		flux << t;
	}

	template <typename T, typename... Ts>
	void ajoute_avertissement_flux(dls::flux_chaine &flux, T &&t, Ts &&... ts)
	{
		flux << t;
		ajoute_avertissement_flux(flux, ts...);
	}
};

calque_image const *cherche_calque(
		OperatriceImage &op,
		Image const *image,
		dls::chaine const &nom_calque);

/* ************************************************************************** */

inline OperatriceImage *extrait_opimage(std::any const &any)
{
	return std::any_cast<OperatriceImage *>(any);
}

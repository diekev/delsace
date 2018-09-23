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

#include <danjo/manipulable.h>

#include <image/pixel.h>
#include <math/matrice/matrice.h>

#include <list>
#include <vector>

#include "bibliotheques/geometrie/rectangle.h"
#include "bibliotheques/outils/iterateurs.h"

class Collection;
class Corps;
class Maillage;
class Manipulatrice3D;
class Noeud;
class PriseEntree;
class Scene;
class TextureImage;

namespace vision {
class Camera3D;
}  /* namespace vision */

struct Objet;

using type_image = numero7::math::matrice<numero7::image::Pixel<float>>;

enum {
	EXECUTION_REUSSIE = 0,
	EXECUTION_ECHOUEE = 1,
};

enum {
	OPERATRICE_IMAGE,
	OPERATRICE_SORTIE_IMAGE,
	OPERATRICE_PIXEL,
	OPERATRICE_GRAPHE_PIXEL,
	OPERATRICE_SCENE,
	OPERATRICE_OBJET,
	OPERATRICE_CAMERA,
	OPERATRICE_CORPS,
	OPERATRICE_SORTIE_CORPS,
	OPERATRICE_GRAPHE_MAILLAGE,
};

/* ************************************************************************** */

struct Calque {
	std::string nom;
	type_image tampon;

	/**
	 * Retourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	numero7::image::Pixel<float> valeur(int x, int y) const;

	/**
	 * Ajourne la valeur du tampon de ce calque à la position <x, y>.
	 */
	void valeur(int x, int y, const numero7::image::Pixel<float> &pixel);

	/**
	 * Échantillonne le tampon de ce calque à la position <x, y> en utilisant
	 * une entrepolation linéaire entre les pixels se trouvant entre les quatre
	 * coins de la position spécifiée.
	 */
	numero7::image::Pixel<float> echantillone(float x, float y) const;
};

/* ************************************************************************** */

class Image {
	std::list<Calque *> m_calques;
	std::string m_nom_calque;

public:
	using plage_calques = plage_iterable<std::list<Calque *>::iterator>;
	using plage_calques_const = plage_iterable<std::list<Calque *>::const_iterator>;

	~Image();

	/**
	 * Ajoute un calque à cette image avec le nom spécifié. La taille du calque
	 * est définie par le rectangle passé en paramètre. Retourne un pointeur
	 * vers le calque ajouté.
	 */
	Calque *ajoute_calque(const std::string &nom, const Rectangle &rectangle);

	/**
	 * Retourne un pointeur vers le calque portant le nom passé en paramètre. Si
	 * aucun calque ne portant ce nom est trouvé, retourne nullptr.
	 */
	Calque *calque(const std::string &nom) const;

	/**
	 * Retourne une plage itérable sur la liste de calques de cette Image.
	 */
	plage_calques calques();

	/**
	 * Retourne une plage itérable constante sur la liste de calques de cette Image.
	 */
	plage_calques_const calques() const;

	/**
	 * Vide la liste de calques de cette image. Si garde_memoires est faux,
	 * les calques seront supprimés. Cette méthode est à utiliser pour
	 * transférer la propriété des calques d'une image à une autre.
	 */
	void reinitialise(bool garde_memoires = false);

	/**
	 * Renseigne le nom du calque actif.
	 */
	void nom_calque_actif(const std::string &nom);

	/**
	 * Retourne le nom du calque actif.
	 */
	const std::string &nom_calque_actif() const;
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
	std::vector<std::string> m_liste_noms_calques{};

public:
	/**
	 * Crée une EntreeOperatrice sans pointeur associé, ce constructeur n'existe
	 * que pour pouvoir stocker des EntreeOperatrices dans des conteneurs et
	 * n'est pas à être utilisé autrement.
	 */
	EntreeOperatrice() = default;

	/**
	 * Crée une instance d'EntreeOperatrice pour la PriseEntree spécifiée.
	 */
	explicit EntreeOperatrice(PriseEntree *prise);

	/**
	 * Retourne vrai si la prise est connectée.
	 */
	bool connectee() const;

	/**
	 * Requiers l'image du noeud connecté à cette prise en exécutant ledit noeud
	 * avant de lui prendre son image. La liste de nom de calque est mise à jour
	 * selon l'image obtenue.
	 */
	void requiers_image(
			Image &image,
			const Rectangle &rectangle,
			const int temps);

	/**
	 * Requiers la caméra du noeud connecté à cette prise en exécutant ledit
	 * noeud avant de retourner un pointeur vers la caméra. Si aucune caméra
	 * n'est créée par le noeud, ou si aucune connexion n'existe, retourne
	 * nullptr.
	 */
	vision::Camera3D *requiers_camera(
			const Rectangle &rectangle,
			const int temps);

	/**
	 * Requiers l'objet du noeud connecté à cette prise en exécutant ledit
	 * noeud avant de retourner un pointeur vers l'objet. Si aucun objet
	 * n'est créé par le noeud, ou si aucune connexion n'existe, retourne
	 * nullptr.
	 */
	Objet *requiers_objet(
			const Rectangle &rectangle,
			const int temps);

	/**
	 * Requiers la texture du noeud connecté à cette prise en exécutant ledit
	 * noeud avant de retourner un pointeur vers la texture. Si aucune texture
	 * n'est créée par le noeud, ou si aucune connexion n'existe, retourne
	 * nullptr.
	 */
	TextureImage *requiers_texture(const Rectangle &rectangle, const int temps);

	void requiers_collection(Collection &collection, const Rectangle &rectangle, const int temps);

	const Corps *requiers_corps(const Rectangle &rectangle, const int temps);

	Corps *requiers_copie_corps(Corps *corps, const Rectangle &rectangle, const int temps);

	/**
	 * Place la liste de calque de l'image transitante par cette entrée dans le
	 * vecteur de chaines spécifié.
	 */
	void obtiens_liste_calque(std::vector<std::string> &chaines) const;

	PriseEntree *pointeur()
	{
		return m_ptr;
	}
};

struct PriseSortie;

class SortieOperatrice {
	PriseSortie *m_ptr = nullptr;

public:
	SortieOperatrice() = default;

	explicit SortieOperatrice(PriseSortie *prise)
		: m_ptr(prise)
	{}

	SortieOperatrice(const SortieOperatrice &autre) = default;

	PriseSortie *pointeur()
	{
		return m_ptr;
	}
};

/* ************************************************************************** */

class OperatriceImage : public danjo::Manipulable {
	int m_num_inputs = 2;
	int m_num_outputs = 1;

	std::vector<EntreeOperatrice> m_input_data;
	std::vector<SortieOperatrice> m_sorties;

	std::vector<std::string> m_avertissements;

protected:
	Image m_image;

public:
	/* Prevent creating an operator without an accompanying node. */
	OperatriceImage() = delete;

	explicit OperatriceImage(Noeud *node);

	virtual ~OperatriceImage() = default;

	virtual int type() const;

	/* Input handling. */

	void inputs(int number);

	int inputs() const;

	virtual const char *nom_entree(int n);

	virtual int type_entree(int n) const;

	EntreeOperatrice *input(size_t index);

	const EntreeOperatrice *input(size_t index) const;

	void set_input_data(size_t index, PriseEntree *socket);

	SortieOperatrice *output(size_t index);

	void set_output_data(size_t index, PriseSortie *prise);

	/* Output handling. */

	void outputs(int number);

	int outputs() const;

	virtual const char *nom_sortie(int n);

	virtual int type_sortie(int n) const;

	/* Information about this operator. */

	virtual const char *class_name() const = 0;
	virtual const char *help_text() const = 0;
	virtual const char *chemin_entreface() const;

	/* The main processing logic of this operator. */

	virtual int execute(const Rectangle &rectangle, const int temps) = 0;

	void transfere_image(Image &image);

	void ajoute_avertissement(const std::string &avertissement);

	void reinitialise_avertisements();

	const std::vector<std::string> &avertissements() const;

	virtual vision::Camera3D *camera();

	virtual Scene *scene();

	virtual TextureImage *texture();

	virtual Objet *objet();

	virtual Collection *collection();

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
			const std::string &attache,
			std::vector<std::string> &chaines) const;
};

/* ************************************************************************** */

void supprime_operatrice_image(void *pointeur);

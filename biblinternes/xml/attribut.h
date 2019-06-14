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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "paire_string.h"

#include "erreur.h"

namespace dls {
namespace xml {

class MemPool;

/**
 * Un atrribut est une paire nom-valeur. Les éléments ont un nombre arbitraire
 * d'attributs, chacun ayant un nom unique.
 *
 * Les attributs ne sont pas des Noeuds. On ne peut qu'accéder l'attribut
 * suivant dans la liste.
 */
class Attribut {
	friend class Element;

	mutable PaireString m_nom{};
	mutable PaireString m_valeur{};

	Attribut *m_next = nullptr;

	enum {
		TAILLE_TAMPON = 200
	};

	MemPool *_memPool{};

public:
	Attribut(const Attribut&) = delete;
	Attribut &operator=(const Attribut&) = delete;

	/**
	 * Retourne le nom de l'attribut, sous forme d'une chaîne de caractère
	 * terminée par zéro.
	 */
	const char *nom() const;

	/**
	 * Retourne la valeur de l'attribut, sous forme d'une chaîne de caractère
	 * terminée par zéro.
	 */
	const char *valeur() const;

	/**
	 * Retourne un pointeur vers l'attribut suivant dans la liste. Retourne
	 * nullptr s'il n'y a pas d'attribut suivant.
	 */
	const Attribut *suivant() const;

	/**
	 * Interprête l'attribute en tant qu'int, et retourne la valeur. Si la
	 * valeur n'est pas un int, retourne 0. Il n'y a pas de vérification
	 * d'erreur, pour ceci utilisez requiers_valeur_int(int*).
	 */
	int valeur_int() const;

	/**
	 * Interprête l'attribute en tant qu'unsigned, et retourne la valeur. Si la
	 * valeur n'est pas un unsigned, retourne 0u. Il n'y a pas de vérification
	 * d'erreur, pour ceci utilisez requiers_valeur_unsigned(unsigned*).
	 */
	unsigned valeur_unsigned() const;

	/**
	 * Interprête l'attribute en tant que bool, et retourne la valeur. Si la
	 * valeur n'est pas un bool, retourne false. Il n'y a pas de vérification
	 * d'erreur, pour ceci utilisez requiers_valeur_bool(bool*).
	 */
	bool valeur_bool() const;

	/**
	 * Interprête l'attribute en tant que double, et retourne la valeur. Si la
	 * valeur n'est pas un double, retourne 0.0. Il n'y a pas de vérification
	 * d'erreur, pour ceci utilisez requiers_valeur_double(double*).
	 */
	double valeur_double() const;

	/**
	 * Interprête l'attribute en tant que float, et retourne la valeur. Si la
	 * valeur n'est pas un float, retourne 0.0f. Il n'y a pas de vérification
	 * d'erreur, pour ceci utilisez requiers_valeur_float(float*).
	 */
	float valeur_float() const;

	/**
	 * Interprête l'attribute en tant que int, et retourne la valeur dans
	 * le paramètre spécifié. La fonction retournera Erreur::AUCUNE_ERREUR si
	 * réussite, autrement Erreur::MAUVAIS_TYPE_ATTRIBUT si la conversion n'est
	 * pas réussie.
	 */
	XMLError requiers_valeur_int(int *valeur) const;

	/**
	 * Interprête l'attribute en tant que unsigned, et retourne la valeur dans
	 * le paramètre spécifié. La fonction retournera Erreur::AUCUNE_ERREUR si
	 * réussite, autrement Erreur::MAUVAIS_TYPE_ATTRIBUT si la conversion n'est
	 * pas réussie.
	 */
	XMLError requiers_valeur_unsigned(unsigned int *valeur) const;

	/**
	 * Interprête l'attribute en tant que bool, et retourne la valeur dans le
	 * paramètre spécifié. La fonction retournera Erreur::AUCUNE_ERREUR si
	 * réussite, autrement Erreur::MAUVAIS_TYPE_ATTRIBUT si la conversion n'est
	 * pas réussie.
	 */
	XMLError requiers_valeur_bool(bool *valeur) const;

	/**
	 * Interprête l'attribute en tant que double, et retourne la valeur dans
	 * le paramètre spécifié. La fonction retournera Erreur::AUCUNE_ERREUR si
	 * réussite, autrement Erreur::MAUVAIS_TYPE_ATTRIBUT si la conversion n'est
	 * pas réussie.
	 */
	XMLError requiers_valeurr_double(double *valeur) const;

	/**
	 * Interprête l'attribute en tant que float, et retourne la valeur dans
	 * le paramètre spécifié. La fonction retournera Erreur::AUCUNE_ERREUR si
	 * réussite, autrement Erreur::MAUVAIS_TYPE_ATTRIBUT si la conversion n'est
	 * pas réussie.
	 */
	XMLError requiers_valeur_float(float *valeur) const;

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(const char *valeur);

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(int valeur);

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(unsigned valeur);

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(bool valeur);

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(double valeur);

	/**
	 * Ajourne l'attribut avec la valeur passé en paramètre.
	 */
	void ajourne_valeur(float valeur);

private:
	Attribut() = default;
	~Attribut() = default;

	void nom(const char *name);

	char *analyse_profonde(char* p, bool traite_entites);
};

}  /* namespace xml */
}  /* namespace dls */

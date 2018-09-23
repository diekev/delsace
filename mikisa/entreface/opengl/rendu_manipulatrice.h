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

class ContexteRendu;
class Manipulatrice3D;
class TamponRendu;

/* ************************************************************************** */

/**
 * La classe RenduManipulatricePosition contient la logique de rendu d'une
 * manipulatrice de position dans la scène 3D.
 */
class RenduManipulatricePosition {
	TamponRendu *m_tampon_axe_x = nullptr;
	TamponRendu *m_tampon_axe_y = nullptr;
	TamponRendu *m_tampon_axe_z = nullptr;
	TamponRendu *m_tampon_poignee_xyz = nullptr;

	Manipulatrice3D *m_pointeur = nullptr;

public:
	/**
	 * Construit une instance de RenduManipulatricePosition selon les dimensions
	 * spécifiées. La construction implique la création de tampons OpenGL, donc
	 * elle doit se faire dans un contexte OpenGL valide.
	 */
	RenduManipulatricePosition();

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduManipulatricePosition();

	void manipulatrice(Manipulatrice3D *pointeur);

	/**
	 * Dessine la manipulatrice dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};

/* ************************************************************************** */

/**
 * La classe RenduManipulatriceEchelle contient la logique de rendu d'une
 * manipulatrice d'échelle dans la scène 3D.
 */
class RenduManipulatriceEchelle {
	TamponRendu *m_tampon_axe_x = nullptr;
	TamponRendu *m_tampon_axe_y = nullptr;
	TamponRendu *m_tampon_axe_z = nullptr;
	TamponRendu *m_tampon_poignee_xyz = nullptr;

	Manipulatrice3D *m_pointeur = nullptr;

public:
	/**
	 * Construit une instance de RenduManipulatriceEchelle selon les dimensions
	 * spécifiées. La construction implique la création de tampons OpenGL, donc
	 * elle doit se faire dans un contexte OpenGL valide.
	 */
	RenduManipulatriceEchelle();

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduManipulatriceEchelle();

	void manipulatrice(Manipulatrice3D *pointeur);

	/**
	 * Dessine la manipulatrice dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};

/* ************************************************************************** */

/**
 * La classe RenduManipulatriceRotation contient la logique de rendu d'une
 * manipulatrice de rotation dans la scène 3D.
 */
class RenduManipulatriceRotation {
	TamponRendu *m_tampon_axe_x = nullptr;
	TamponRendu *m_tampon_axe_y = nullptr;
	TamponRendu *m_tampon_axe_z = nullptr;

	Manipulatrice3D *m_pointeur = nullptr;

public:
	/**
	 * Construit une instance de RenduManipulatriceRotation selon les dimensions
	 * spécifiées. La construction implique la création de tampons OpenGL, donc
	 * elle doit se faire dans un contexte OpenGL valide.
	 */
	RenduManipulatriceRotation();

	/**
	 * Détruit les données de l'instance. Les tampons de rendu sont détruits et
	 * utiliser l'instance crashera le programme.
	 */
	~RenduManipulatriceRotation();

	void manipulatrice(Manipulatrice3D *pointeur);

	/**
	 * Dessine la manipulatrice dans le contexte spécifié.
	 */
	void dessine(const ContexteRendu &contexte);
};

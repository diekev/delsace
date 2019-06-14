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

#include "noeud.h"

namespace dls {
namespace xml {

class Visiteur;

/**
 * En XML correct, la déclaration est la première entrée dans la fichier.
 * Savoir : '<?xml version="1.0" standalone="yes"?>'.
 *
 * Pourtant, cette bibliothèque lira et écrira sans problème des fichiers sans
 * déclaration.
 *
 * Le texte de la déclaration n'est pas interprêté. C'est analysé et écrit en
 * tant que chaîne de caractère.
 */
class Declaration final : public Noeud {
	friend class Document;

protected:
	explicit Declaration(Document* doc);
	~Declaration() = default;

	char* ParseDeep(char*, PaireString* endTag);

public:
	Declaration(const Declaration&) = delete;
	Declaration& operator=(const Declaration&) = delete;

	Declaration *ToDeclaration() override;
	const Declaration *ToDeclaration() const override;

	bool Accept(Visiteur *visiteur) const override;

	Noeud* ShallowClone(Document* document) const override;
	bool ShallowEqual(const Noeud* compare) const override;
};

}  /* namespace xml */
}  /* namespace dls */


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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "transformation_type.hh"

#include "contexte_generation_code.h"
#include "outils_lexemes.hh"

const char *chaine_transformation(TypeTransformation type)
{
#define CAS_TYPE(x) case TypeTransformation::x: return #x;
	switch (type) {
		CAS_TYPE(INUTILE)
		CAS_TYPE(IMPOSSIBLE)
		CAS_TYPE(CONSTRUIT_EINI)
		CAS_TYPE(CONSTRUIT_EINI_TABLEAU)
		CAS_TYPE(EXTRAIT_EINI)
		CAS_TYPE(CONSTRUIT_TABL_OCTET)
		CAS_TYPE(PREND_PTR_RIEN)
		CAS_TYPE(CONVERTI_TABLEAU)
		CAS_TYPE(FONCTION)
		CAS_TYPE(PREND_REFERENCE)
		CAS_TYPE(DEREFERENCE)
		CAS_TYPE(AUGMENTE_TAILLE_TYPE)
	}

	return "ERREUR";
#undef CAS_TYPE
}

/* Trouve la transformation nécessaire pour aller d'un type à un autre de
 * manière conditionnelle.
 *
 * L'utilisation du graphe de dépendance pour trouver les conversions entre
 * types fut considéré mais la vitesse de compilation souffra énormément.
 * Ce fut 50x plus lent que la méthode ci-bas où toutes les relations possibles
 * sont codées en dur. Un profilage montra que la plupart du temps était passé à
 * enfiler et défiler les noeuds à visiter, mais aussi à vérifier si un noeud a
 * déjà été visité.
 *
 * Un point fort du graphe serait d'avoir des conversion complexes entre par
 * exemple des unités de mesure (p.e. de centimètre (cm) vers pieds (ft)) sans
 * devoir explicitement avoir cette conversion dans le code tant que types se
 * trouvent entre les deux (p.e. cm -> dm -> m -> ft). Ou encore automatiquement
 * construire des unions depuis une valeur d'un type membre.
 *
 * Cette méthode est limitée, mais largement plus rapide que celle utilisant un
 * graphe, qui sera sans doute révisée plus tard.
 */
TransformationType cherche_transformation(
		ContexteGenerationCode const &contexte,
		long type_de,
		long type_vers)
{
	if (type_de == type_vers) {
		return TypeTransformation::INUTILE;
	}

	auto const &typeuse = contexte.typeuse;
	auto const &dt_de = typeuse[type_de];
	auto const &dt_vers = typeuse[type_vers];

	if (est_type_entier_relatif(dt_de.type_base()) && est_type_entier_relatif(dt_vers.type_base())) {
		if (taille_octet_type(contexte, dt_de) < taille_octet_type(contexte, dt_vers)) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (est_type_entier_naturel(dt_de.type_base()) && est_type_entier_naturel(dt_vers.type_base())) {
		if (taille_octet_type(contexte, dt_de) < taille_octet_type(contexte, dt_vers)) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (est_type_reel(dt_de.type_base()) && est_type_reel(dt_vers.type_base())) {
		/* cas spéciaux pour R16 */
		if (dt_de.type_base() == TypeLexeme::R16) {
			if (dt_vers.type_base() == TypeLexeme::R32) {
				return "DLS_vers_r32";
			}

			if (dt_vers.type_base() == TypeLexeme::R64) {
				return "DLS_vers_r64";
			}

			return TypeTransformation::IMPOSSIBLE;
		}

		/* cas spéciaux pour R16 */
		if (dt_vers.type_base() == TypeLexeme::R16) {
			if (dt_de.type_base() == TypeLexeme::R32) {
				return "DLS_depuis_r32";
			}

			if (dt_de.type_base() == TypeLexeme::R64) {
				return "DLS_depuis_r64";
			}

			return TypeTransformation::IMPOSSIBLE;
		}

		if (taille_octet_type(contexte, dt_de) < taille_octet_type(contexte, dt_vers)) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_vers == typeuse[TypeBase::EINI]) {
		if (est_type_tableau_fixe(dt_de)) {
			return TypeTransformation::CONSTRUIT_EINI_TABLEAU;
		}

		return TypeTransformation::CONSTRUIT_EINI;
	}

	if (type_de == typeuse[TypeBase::EINI]) {
		return { TypeTransformation::EXTRAIT_EINI, type_vers };
	}

	if (dt_vers.type_base() == TypeLexeme::FONC) {
		/* x : fonc()rien = nul; */
		if (dt_de.type_base() == TypeLexeme::POINTEUR && dt_de.dereference().front() == TypeLexeme::NUL) {
			return TypeTransformation::INUTILE;
		}

		/* Nous savons que les types sont différents, donc si l'un des deux est un
		 * pointeur fonction, nous pouvons retourner faux. */
		return TypeTransformation::IMPOSSIBLE;
	}

	if (dt_vers.type_base() == TypeLexeme::REFERENCE) {
		if (dt_vers.dereference() == dt_de) {
			return TypeTransformation::PREND_REFERENCE;
		}
	}

	if (dt_de.type_base() == TypeLexeme::REFERENCE) {
		if (dt_de.dereference() == dt_vers) {
			return TypeTransformation::DEREFERENCE;
		}
	}

	if (dt_vers.type_base() == TypeLexeme::TABLEAU) {
		if (dt_vers.dereference().front() == TypeLexeme::OCTET) {
			return TypeTransformation::CONSTRUIT_TABL_OCTET;
		}

		if ((dt_de.type_base() & 0xff) != TypeLexeme::TABLEAU) {
			return TypeTransformation::IMPOSSIBLE;
		}

		if (dt_vers.dereference() == dt_de.dereference()) {
			return TypeTransformation::CONVERTI_TABLEAU;
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (dt_vers.type_base() == TypeLexeme::POINTEUR) {
		if (dt_de.type_base() == TypeLexeme::POINTEUR) {
			/* x = nul; */
			if (dt_de.dereference().front() == TypeLexeme::NUL) {
				return TypeTransformation::INUTILE;
			}

			/* x : *z8 = y (*rien) */
			if (dt_de.dereference().front() == TypeLexeme::RIEN) {
				return TypeTransformation::INUTILE;
			}

			/* x : *rien = y; */
			if (dt_vers.dereference().front() == TypeLexeme::RIEN) {
				return TypeTransformation::INUTILE;
			}

			/* x : *octet = y; */
			if (dt_vers.dereference().front() == TypeLexeme::OCTET) {
				return TypeTransformation::INUTILE;
			}

			/* x : *nul = y */
			if (dt_vers.dereference().front() == TypeLexeme::NUL) {
				return TypeTransformation::INUTILE;
			}
		}
	}

	return TypeTransformation::IMPOSSIBLE;
}

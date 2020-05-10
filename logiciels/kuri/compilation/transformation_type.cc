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
		CAS_TYPE(CONSTRUIT_UNION)
		CAS_TYPE(EXTRAIT_UNION)
		CAS_TYPE(CONSTRUIT_EINI)
		CAS_TYPE(EXTRAIT_EINI)
		CAS_TYPE(CONSTRUIT_TABL_OCTET)
		CAS_TYPE(CONVERTI_TABLEAU)
		CAS_TYPE(FONCTION)
		CAS_TYPE(PREND_REFERENCE)
		CAS_TYPE(DEREFERENCE)
		CAS_TYPE(AUGMENTE_TAILLE_TYPE)
		CAS_TYPE(CONVERTI_VERS_BASE)
		CAS_TYPE(CONVERTI_ENTIER_CONSTANT)
		CAS_TYPE(CONVERTI_VERS_PTR_RIEN)
		CAS_TYPE(CONVERTI_VERS_TYPE_CIBLE)
	}

	return "ERREUR";
#undef CAS_TYPE
}

static bool est_type_de_base(TypeStructure *type_de, TypeStructure *type_vers)
{
	POUR (type_de->types_employes) {
		if (it == type_vers) {
			return true;
		}
	}

	return false;
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
		Type *type_de,
		Type *type_vers)
{
	if (type_de == type_vers) {
		return TypeTransformation::INUTILE;
	}

	if (type_de->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_vers)) {
		return { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers };
	}

	if (type_de->genre == GenreType::ENTIER_NATUREL && type_vers->genre == GenreType::ENTIER_NATUREL) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_de->genre == GenreType::ENTIER_RELATIF && type_vers->genre == GenreType::ENTIER_RELATIF) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_de->genre == GenreType::REEL && type_vers->genre == GenreType::REEL) {
		/* cas spéciaux pour R16 */
		if (type_de->taille_octet == 2) {
			if (type_vers->taille_octet == 4) {
				return { "DLS_vers_r32", type_vers };
			}

			if (type_vers->taille_octet == 8) {
				return { "DLS_vers_r64", type_vers };
			}

			return TypeTransformation::IMPOSSIBLE;
		}

		/* cas spéciaux pour R16 */
		if (type_vers->taille_octet == 2) {
			if (type_de->taille_octet == 4) {
				return { "DLS_depuis_r32", type_vers };
			}

			if (type_de->taille_octet == 8) {
				return { "DLS_depuis_r64", type_vers };
			}

			return TypeTransformation::IMPOSSIBLE;
		}

		if (type_de->taille_octet < type_vers->taille_octet) {
			return { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_vers->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeUnion *>(type_vers);

		auto index_membre = 0l;

		POUR (type_union->membres) {
			if (it.type == type_de) {
				return { TypeTransformation::CONSTRUIT_UNION, type_vers, index_membre };
			}

			if (est_type_entier(it.type) && type_de->genre == GenreType::ENTIER_CONSTANT) {
				return { TypeTransformation::CONSTRUIT_UNION, type_vers, index_membre };
			}

			index_membre += 1;
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_vers->genre == GenreType::EINI) {
		return TypeTransformation::CONSTRUIT_EINI;
	}

	if (type_de->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeUnion *>(type_de);

		auto index_membre = 0l;

		POUR (type_union->membres) {
			if (it.type == type_vers) {
				return { TypeTransformation::EXTRAIT_UNION, type_vers, index_membre };
			}

			index_membre += 1;
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_de->genre == GenreType::EINI) {
		return { TypeTransformation::EXTRAIT_EINI, type_vers };
	}

	if (type_vers->genre == GenreType::FONCTION) {
		/* x : fonc()rien = nul; */
		if (type_de->genre == GenreType::POINTEUR && static_cast<TypePointeur *>(type_de)->type_pointe == nullptr) {
			return { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
		}

		/* Nous savons que les types sont différents, donc si l'un des deux est un
		 * pointeur fonction, nous pouvons retourner faux. */
		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_vers->genre == GenreType::REFERENCE) {
		if (static_cast<TypeReference *>(type_vers)->type_pointe == type_de) {
			return TypeTransformation::PREND_REFERENCE;
		}
	}

	if (type_de->genre == GenreType::REFERENCE) {
		if (static_cast<TypeReference *>(type_de)->type_pointe == type_vers) {
			return TypeTransformation::DEREFERENCE;
		}
	}

	if (type_vers->genre == GenreType::TABLEAU_DYNAMIQUE) {
		auto type_pointe = static_cast<TypeTableauDynamique *>(type_vers)->type_pointe;

		if (type_pointe->genre == GenreType::OCTET) {
			return TypeTransformation::CONSTRUIT_TABL_OCTET;
		}

		if (type_de->genre != GenreType::TABLEAU_FIXE) {
			return TypeTransformation::IMPOSSIBLE;
		}

		if (type_pointe == static_cast<TypeTableauFixe *>(type_de)->type_pointe) {
			return TypeTransformation::CONVERTI_TABLEAU;
		}

		return TypeTransformation::IMPOSSIBLE;
	}

	if (type_vers->genre == GenreType::POINTEUR && type_de->genre == GenreType::POINTEUR) {
		auto type_pointe_de = static_cast<TypePointeur *>(type_de)->type_pointe;
		auto type_pointe_vers = static_cast<TypePointeur *>(type_vers)->type_pointe;

		/* x = nul; */
		if (type_pointe_de == nullptr) {
			return { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
		}

		/* x : *z8 = y (*rien) */
		if (type_pointe_de->genre == GenreType::RIEN) {
			return TypeTransformation::INUTILE;
		}

		/* x : *nul = y */
		if (type_pointe_vers == nullptr) {
			return TypeTransformation::INUTILE;
		}

		/* x : *rien = y; */
		if (type_pointe_vers->genre == GenreType::RIEN) {
			return TypeTransformation::CONVERTI_VERS_PTR_RIEN;
		}

		/* x : *octet = y; */
		if (type_pointe_vers->genre == GenreType::OCTET) {
			return TypeTransformation::INUTILE;
		}

		if (type_pointe_de->genre == GenreType::STRUCTURE && type_pointe_vers->genre == GenreType::STRUCTURE) {
			auto ts_de = static_cast<TypeStructure *>(type_pointe_de);
			auto ts_vers = static_cast<TypeStructure *>(type_pointe_vers);

			// À FAIRE : gère le décalage dans la structure, ceci ne peut
			// fonctionner que si la structure de base est au début de la
			// structure dérivée
			if (est_type_de_base(ts_de, ts_vers)) {
				return { TypeTransformation::CONVERTI_VERS_BASE, type_vers };
			}
		}
	}

	return TypeTransformation::IMPOSSIBLE;
}

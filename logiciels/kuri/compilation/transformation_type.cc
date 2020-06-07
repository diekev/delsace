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

#include "compilatrice.hh"
#include "outils_lexemes.hh"
#include "profilage.hh"
#include "validation_semantique.hh"

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
bool cherche_transformation(
		Compilatrice &compilatrice,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	PROFILE_FONCTION;

	if (type_de == type_vers) {
		transformation = TypeTransformation::INUTILE;
		return false;
	}

	/* nous avons un type de données pour chaque type connu lors de la
	 * compilation, donc testons manuellement la compatibilité */
	if (type_de->genre == GenreType::TYPE_DE_DONNEES && type_vers->genre == GenreType::TYPE_DE_DONNEES) {
		transformation = TypeTransformation::INUTILE;
		return false;
	}

	if (type_de->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_vers)) {
		transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers };
		return false;
	}

	if (type_de->genre == GenreType::ENTIER_NATUREL && type_vers->genre == GenreType::ENTIER_NATUREL) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_de->genre == GenreType::ENTIER_RELATIF && type_vers->genre == GenreType::ENTIER_RELATIF) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_de->genre == GenreType::REEL && type_vers->genre == GenreType::REEL) {
		auto retourne_fonction = [&](NoeudDeclarationFonction const *fonction) -> bool
		{
			if (fonction == nullptr) {
				contexte.unite->attend_sur_interface_kuri();
				return true;
			}

			contexte.donnees_dependance.fonctions_utilisees.insere(fonction);
			transformation = { fonction, type_vers };
			return false;
		};

		/* cas spéciaux pour R16 */
		if (type_de->taille_octet == 2) {
			if (type_vers->taille_octet == 4) {
				return retourne_fonction(compilatrice.interface_kuri.decl_dls_vers_r32);
			}

			if (type_vers->taille_octet == 8) {
				return retourne_fonction(compilatrice.interface_kuri.decl_dls_vers_r64);
			}

			transformation = TypeTransformation::IMPOSSIBLE;
			return false;
		}

		/* cas spéciaux pour R16 */
		if (type_vers->taille_octet == 2) {
			if (type_de->taille_octet == 4) {
				return retourne_fonction(compilatrice.interface_kuri.decl_dls_depuis_r32);
			}

			if (type_de->taille_octet == 8) {
				return retourne_fonction(compilatrice.interface_kuri.decl_dls_depuis_r64);
			}

			transformation = TypeTransformation::IMPOSSIBLE;
			return false;
		}

		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeUnion *>(type_vers);

		if ((type_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
			contexte.unite->attend_sur_type(type_vers);
			return true;
		}

		auto index_membre = 0l;

		POUR (type_union->membres) {
			if (it.type == type_de) {
				transformation = { TypeTransformation::CONSTRUIT_UNION, type_vers, index_membre };
				return false;
			}

			if (est_type_entier(it.type) && type_de->genre == GenreType::ENTIER_CONSTANT) {
				transformation = { TypeTransformation::CONSTRUIT_UNION, type_vers, index_membre };
				return false;
			}

			index_membre += 1;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::EINI) {
		transformation = TypeTransformation::CONSTRUIT_EINI;
		return false;
	}

	if (type_de->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeUnion *>(type_de);

		if ((type_union->drapeaux & TYPE_FUT_VALIDE) == 0) {
			contexte.unite->attend_sur_type(type_union);
			return true;
		}

		auto index_membre = 0l;

		POUR (type_union->membres) {
			if (it.type == type_vers) {
				if (!type_union->est_nonsure) {
					if (compilatrice.interface_kuri.decl_panique_membre_union == nullptr) {
						contexte.unite->attend_sur_interface_kuri();
						return true;
					}

					contexte.donnees_dependance.fonctions_utilisees.insere(compilatrice.interface_kuri.decl_panique_membre_union);
				}

				transformation = { TypeTransformation::EXTRAIT_UNION, type_vers, index_membre };
				return false;
			}

			index_membre += 1;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_de->genre == GenreType::EINI) {
		transformation = { TypeTransformation::EXTRAIT_EINI, type_vers };
		return false;
	}

	if (type_vers->genre == GenreType::FONCTION) {
		/* x : fonc()rien = nul; */
		if (type_de->genre == GenreType::POINTEUR && static_cast<TypePointeur *>(type_de)->type_pointe == nullptr) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		/* Nous savons que les types sont différents, donc si l'un des deux est un
		 * pointeur fonction, nous pouvons retourner faux. */
		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::REFERENCE) {
		if (static_cast<TypeReference *>(type_vers)->type_pointe == type_de) {
			transformation = TypeTransformation::PREND_REFERENCE;
			return false;
		}
	}

	if (type_de->genre == GenreType::REFERENCE) {
		if (static_cast<TypeReference *>(type_de)->type_pointe == type_vers) {
			transformation = TypeTransformation::DEREFERENCE;
			return false;
		}
	}

	if (type_vers->genre == GenreType::TABLEAU_DYNAMIQUE) {
		auto type_pointe = static_cast<TypeTableauDynamique *>(type_vers)->type_pointe;

		if (type_pointe->genre == GenreType::OCTET) {
			// a : []octet = nul, voir bug19
			if (type_de->genre == GenreType::POINTEUR) {
				auto type_pointe_de = static_cast<TypePointeur *>(type_de)->type_pointe;

				if (type_pointe_de == nullptr) {
					transformation = TypeTransformation::IMPOSSIBLE;
				}
			}

			transformation = TypeTransformation::CONSTRUIT_TABL_OCTET;
			return false;
		}

		if (type_de->genre != GenreType::TABLEAU_FIXE) {
			transformation = TypeTransformation::IMPOSSIBLE;
			return false;
		}

		if (type_pointe == static_cast<TypeTableauFixe *>(type_de)->type_pointe) {
			transformation = TypeTransformation::CONVERTI_TABLEAU;
			return false;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::POINTEUR && type_de->genre == GenreType::POINTEUR) {
		auto type_pointe_de = static_cast<TypePointeur *>(type_de)->type_pointe;
		auto type_pointe_vers = static_cast<TypePointeur *>(type_vers)->type_pointe;

		/* x = nul; */
		if (type_pointe_de == nullptr) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		/* x : *z8 = y (*rien) */
		if (type_pointe_de->genre == GenreType::RIEN) {
			transformation = TypeTransformation::INUTILE;
			return false;
		}

		/* x : *nul = y */
		if (type_pointe_vers == nullptr) {
			transformation = TypeTransformation::INUTILE;
			return false;
		}

		/* x : *rien = y; */
		if (type_pointe_vers->genre == GenreType::RIEN) {
			transformation = TypeTransformation::CONVERTI_VERS_PTR_RIEN;
			return false;
		}

		/* x : *octet = y; */
		if (type_pointe_vers->genre == GenreType::OCTET) {
			transformation = TypeTransformation::INUTILE;
			return false;
		}

		if (type_pointe_de->genre == GenreType::STRUCTURE && type_pointe_vers->genre == GenreType::STRUCTURE) {
			auto ts_de = static_cast<TypeStructure *>(type_pointe_de);
			auto ts_vers = static_cast<TypeStructure *>(type_pointe_vers);

			if ((ts_de->drapeaux & TYPE_FUT_VALIDE) == 0) {
				contexte.unite->attend_sur_type(ts_de);
				return true;
			}

			if ((ts_vers->drapeaux & TYPE_FUT_VALIDE) == 0) {
				contexte.unite->attend_sur_type(ts_vers);
				return true;
			}

			// À FAIRE : gère le décalage dans la structure, ceci ne peut
			// fonctionner que si la structure de base est au début de la
			// structure dérivée
			if (est_type_de_base(ts_de, ts_vers)) {
				transformation = { TypeTransformation::CONVERTI_VERS_BASE, type_vers };
				return false;
			}
		}
	}

	transformation = TypeTransformation::IMPOSSIBLE;
	return false;
}

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
#include "validation_semantique.hh"

const char *chaine_transformation(TypeTransformation type)
{
	switch (type) {
#define ENUMERE_TYPE_TRANSFORMATION_EX(type) case TypeTransformation::type: return #type;
		ENUMERE_TYPES_TRANSFORMATION
#undef ENUMERE_TYPE_TRANSFORMATION_EX
	}
	return "ERREUR";
}

std::ostream &operator<<(std::ostream &os, TypeTransformation type)
{
	os << chaine_transformation(type);
	return os;
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
template <bool POUR_TRANSTYPAGE>
bool cherche_transformation(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
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

	// À FAIRE : type R16
	if (type_de->genre == GenreType::ENTIER_CONSTANT && (est_type_entier(type_vers) || type_vers->est_octet() || type_vers->est_reel())) {
		transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers };
		return false;
	}

	if (POUR_TRANSTYPAGE) {
		if (est_type_entier(type_vers) && type_de->genre == GenreType::OCTET) {
			if (type_vers->taille_octet > type_de->taille_octet) {
				transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
				return false;
			}

			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		if (est_type_entier(type_de)) {
			if (type_vers->genre == GenreType::OCTET) {
				if (type_vers->taille_octet < type_de->taille_octet) {
					transformation = { TypeTransformation::REDUIT_TAILLE_TYPE, type_vers };
					return false;
				}

				transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
				return false;
			}

			if (type_vers->genre == GenreType::BOOL) {
				transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
				return false;
			}
		}

		if (type_vers->genre == GenreType::ERREUR && type_de->genre == GenreType::ENTIER_CONSTANT) {
			transformation = { TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_vers };
			return false;
		}

		if (type_de->genre == GenreType::BOOL && est_type_entier(type_vers)) {
			if (type_vers->taille_octet > type_de->taille_octet) {
				transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			}
			else {
				transformation = TypeTransformation::INUTILE;
			}

			return false;
		}

		if (type_de->genre == GenreType::ENUM) {
			if (type_vers == type_de->comme_enum()->type_donnees) {
				// on pourrait se passer de la conversion, ou normaliser le type
				transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
				return false;
			}
		}

		if (type_de->genre == GenreType::ERREUR) {
			if (type_vers == type_de->comme_erreur()->type_donnees) {
				// on pourrait se passer de la conversion, ou normaliser le type
				transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
				return false;
			}
		}

		if (type_vers->genre == GenreType::ENUM && type_vers->comme_enum()->type_donnees == type_de) {
			// on pourrait se passer de la conversion, ou normaliser le type
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}
	}

	if (type_de->genre == GenreType::ENTIER_CONSTANT && type_vers->genre == GenreType::ENUM) {
		// on pourrait se passer de la conversion, ou normaliser le type
		transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
		return false;
	}

	if (type_de->genre == GenreType::ENTIER_NATUREL && type_vers->genre == GenreType::ENTIER_NATUREL) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		if (POUR_TRANSTYPAGE) {
			if (type_de->taille_octet > type_vers->taille_octet) {
				transformation = { TypeTransformation::REDUIT_TAILLE_TYPE, type_vers };
				return false;
			}
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_de->genre == GenreType::ENTIER_RELATIF && type_vers->genre == GenreType::ENTIER_RELATIF) {
		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		if (POUR_TRANSTYPAGE) {
			if (type_de->taille_octet > type_vers->taille_octet) {
				transformation = { TypeTransformation::REDUIT_TAILLE_TYPE, type_vers };
				return false;
			}
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (POUR_TRANSTYPAGE) {
		if (est_type_entier(type_de) && type_vers->genre == GenreType::REEL) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		if (est_type_entier(type_vers) && type_de->genre == GenreType::REEL) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		// converti relatif <-> naturel
		if (est_type_entier(type_de) && est_type_entier(type_vers)) {
			if (type_de->taille_octet > type_vers->taille_octet) {
				transformation = { TypeTransformation::REDUIT_TAILLE_TYPE, type_vers };
			}
			else if (type_de->taille_octet < type_vers->taille_octet) {
				transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			}
			else {
				transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			}

			return false;
		}
	}

	if (type_de->genre == GenreType::REEL && type_vers->genre == GenreType::REEL) {
		auto retourne_fonction = [&](NoeudDeclarationEnteteFonction const *fonction, const char *nom_fonction) -> bool
		{
			if (fonction == nullptr) {
				contexte.unite->attend_sur_interface_kuri(nom_fonction);
				return true;
			}

			contexte.donnees_dependance.fonctions_utilisees.insere(fonction);
			transformation = { fonction, type_vers };
			return false;
		};

		/* cas spéciaux pour R16 */
		if (type_de->taille_octet == 2) {
			if (type_vers->taille_octet == 4) {
				return retourne_fonction(espace.interface_kuri->decl_dls_vers_r32, "DLS_vers_r32");
			}

			if (type_vers->taille_octet == 8) {
				return retourne_fonction(espace.interface_kuri->decl_dls_vers_r64, "DLS_vers_r64");
			}

			transformation = TypeTransformation::IMPOSSIBLE;
			return false;
		}

		/* cas spéciaux pour R16 */
		if (type_vers->taille_octet == 2) {
			if (type_de->taille_octet == 4) {
				return retourne_fonction(espace.interface_kuri->decl_dls_depuis_r32, "DLS_depuis_r32");
			}

			if (type_de->taille_octet == 8) {
				return retourne_fonction(espace.interface_kuri->decl_dls_depuis_r64, "DLS_depuis_r64");
			}

			transformation = TypeTransformation::IMPOSSIBLE;
			return false;
		}

		if (type_de->taille_octet < type_vers->taille_octet) {
			transformation = { TypeTransformation::AUGMENTE_TAILLE_TYPE, type_vers };
			return false;
		}

		if (POUR_TRANSTYPAGE) {
			if (type_de->taille_octet > type_vers->taille_octet) {
				transformation = { TypeTransformation::REDUIT_TAILLE_TYPE, type_vers };
				return false;
			}
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::UNION) {
		auto type_union = type_vers->comme_union();

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
		auto type_union = type_de->comme_union();

		if ((type_union->drapeaux & TYPE_FUT_VALIDE) == 0) {
			contexte.unite->attend_sur_type(type_union);
			return true;
		}

		auto index_membre = 0l;

		POUR (type_union->membres) {
			if (it.type == type_vers) {
				if (!type_union->est_nonsure) {
					auto decl_panique_membre_union = espace.interface_kuri->decl_panique_membre_union;
					if (decl_panique_membre_union == nullptr) {
						contexte.unite->attend_sur_interface_kuri("panique_membre_union");
						return true;
					}
					else if (decl_panique_membre_union->corps->unite == nullptr) {
						contexte.m_compilatrice.ordonnanceuse->cree_tache_pour_typage(&espace, decl_panique_membre_union->corps);
					}

					contexte.donnees_dependance.fonctions_utilisees.insere(decl_panique_membre_union);
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
		if (type_de->genre == GenreType::POINTEUR && type_de->comme_pointeur()->type_pointe == nullptr) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		/* Nous savons que les types sont différents, donc si l'un des deux est un
		 * pointeur fonction, nous pouvons retourner faux. */
		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::REFERENCE) {
		if (type_vers->comme_reference()->type_pointe == type_de) {
			transformation = TypeTransformation::PREND_REFERENCE;
			return false;
		}
	}

	if (type_de->genre == GenreType::REFERENCE) {
		if (type_de->comme_reference()->type_pointe == type_vers) {
			transformation = TypeTransformation::DEREFERENCE;
			return false;
		}
	}

	if (type_vers->genre == GenreType::TABLEAU_DYNAMIQUE) {
		auto type_pointe = type_vers->comme_tableau_dynamique()->type_pointe;

		if (type_pointe->genre == GenreType::OCTET) {
			// a : []octet = nul, voir bug19
			if (type_de->genre == GenreType::POINTEUR) {
				auto type_pointe_de = type_de->comme_pointeur()->type_pointe;

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

		if (type_pointe == type_de->comme_tableau_fixe()->type_pointe) {
			transformation = TypeTransformation::CONVERTI_TABLEAU;
			return false;
		}

		transformation = TypeTransformation::IMPOSSIBLE;
		return false;
	}

	if (type_vers->genre == GenreType::POINTEUR && type_de->genre == GenreType::POINTEUR) {
		auto type_pointe_de = type_de->comme_pointeur()->type_pointe;
		auto type_pointe_vers = type_vers->comme_pointeur()->type_pointe;

		/* x = nul; */
		if (type_pointe_de == nullptr) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		/* x : *z8 = y (*rien) */
		if (type_pointe_de->genre == GenreType::RIEN) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
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
		// À FAIRE : pour transtypage uniquement
		if (type_pointe_vers->genre == GenreType::OCTET) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		if (type_pointe_de->genre == GenreType::STRUCTURE && type_pointe_vers->genre == GenreType::STRUCTURE) {
			auto ts_de = type_pointe_de->comme_structure();
			auto ts_vers = type_pointe_vers->comme_structure();

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

			if (POUR_TRANSTYPAGE) {
				if (est_type_de_base(ts_vers, ts_de)) {
					transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
					return false;
				}
			}
		}

		if (POUR_TRANSTYPAGE) {
			// À FAIRE : pour les einis, nous devrions avoir une meilleure sûreté de type
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}
	}

	if (POUR_TRANSTYPAGE) {
		if ((type_de->est_pointeur() || type_de->est_fonction()) && est_type_entier(type_vers) && type_vers->taille_octet == 8) {
			// À FAIRE : POINTEUR_VERS_ENTIER
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}

		// À FAIRE : ENTIER_VERS_POINTEUR
		if (type_vers->genre == GenreType::POINTEUR && (est_type_entier(type_de) || type_de->genre == GenreType::ENTIER_CONSTANT)) {
			transformation = { TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_vers };
			return false;
		}
	}

	transformation = TypeTransformation::IMPOSSIBLE;
	return false;
}

bool cherche_transformation(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	return cherche_transformation<false>(espace, contexte, type_de, type_vers, transformation);
}

bool cherche_transformation_pour_transtypage(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	return cherche_transformation<true>(espace, contexte, type_de, type_vers, transformation);
}

#if 0
static bool transformation_impossible(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	transformation = TypeTransformation::IMPOSSIBLE;
	return false;
}

static bool transformation_inutile(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{
	transformation = TypeTransformation::INUTILE;
	return false;
}

static bool tansformation_type_vers_eini(
		EspaceDeTravail &espace,
		ContexteValidationCode &contexte,
		Type *type_de,
		Type *type_vers,
		TransformationType &transformation)
{

}

template <typename T, int N>
struct matrice {
	T donnees[N][N];

	constexpr void remplis(T d)
	{
		for (auto i = 0; i < N; ++i) {
			for (auto j = 0; j < N; ++j) {
				donnees[i][j] = d;
			}
		}
	}

	constexpr T *operator[](long i)
	{
		return donnees[i];
	}
};

#define comme_int(x) static_cast<int>(GenreType::x)

static constexpr auto table_transformation_type = [] {
	using type_fonction = bool(*)(EspaceDeTravail &, ContexteValidationCode &, Type *, Type *, TransformationType &);

	matrice<type_fonction, static_cast<int>(GenreType::TOTAL)> table{};
	table.remplis(transformation_impossible);

	/*
	ENUMERE_GENRE_TYPE_EX(ENTIER_NATUREL) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_RELATIF) \
	ENUMERE_GENRE_TYPE_EX(ENTIER_CONSTANT) \
	ENUMERE_GENRE_TYPE_EX(REEL) \
	ENUMERE_GENRE_TYPE_EX(EINI) \
	ENUMERE_GENRE_TYPE_EX(ENUM) \
	ENUMERE_GENRE_TYPE_EX(ERREUR)
	 */

	/* transformations entre types similaires */

	table[comme_int(OCTET)][comme_int(OCTET)] = transformation_inutile;
	table[comme_int(RIEN)][comme_int(RIEN)] = transformation_inutile;
	table[comme_int(BOOL)][comme_int(BOOL)] = transformation_inutile;
	table[comme_int(CHAINE)][comme_int(CHAINE)] = transformation_inutile;
	table[comme_int(EINI)][comme_int(EINI)] = transformation_inutile;
	table[comme_int(TYPE_DE_DONNEES)][comme_int(TYPE_DE_DONNEES)] = transformation_inutile;
	table[comme_int(ENTIER_NATUREL)][comme_int(ENTIER_NATUREL)] = transformation_entre_naturel;
	table[comme_int(ENTIER_RELATIF)][comme_int(ENTIER_RELATIF)] = transformation_entre_relatif;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_CONSTANT)] = transformation_inutile;
	table[comme_int(REEL)][comme_int(REEL)] = transformation_entre_reel;
	table[comme_int(POINTEUR)][comme_int(POINTEUR)] = transformation_entre_pointeur;
	table[comme_int(REFERENCE)][comme_int(REFERENCE)] = transformation_inutile;
	table[comme_int(TABLEAU_FIXE)][comme_int(TABLEAU_FIXE)] = transformation_entre_tableau_fixe;
	table[comme_int(TABLEAU_DYNAMIQUE)][comme_int(TABLEAU_DYNAMIQUE)] = transformation_entre_tableau_dyn;

	/* transformations entre types différents */
	table[comme_int(POINTEUR)][comme_int(FONCTION)] = transformation_pointeur_vers_fonction;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_NATUREL)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(ENTIER_RELATIF)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(REEL)] = transformation_vers_type_cible;
	table[comme_int(ENTIER_CONSTANT)][comme_int(POINTEUR)] = transformation_vers_type_cible;
	table[comme_int(ENUM)][comme_int(ENTIER_NATUREL)] = transformation_enum_vers_entier;
	table[comme_int(ENUM)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;
	table[comme_int(ERREUR)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;
	table[comme_int(ERREUR)][comme_int(ENTIER_RELATIF)] = transformation_enum_vers_entier;

	return table;
}();
#endif

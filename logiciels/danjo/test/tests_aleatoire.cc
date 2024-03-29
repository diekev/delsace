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

#include <iostream>
#include <random>

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/tests/test_aleatoire.hh"

#include "coeur/danjo/compilation/analyseuse_disposition.h"
#include "coeur/danjo/compilation/morceaux.h"

namespace analyse_aleatoire {

void init(u_char *tampon, size_t taille)
{
    auto rd = std::random_device{};
    auto dist = std::uniform_int_distribution<int>{0,
                                                   static_cast<int>(danjo::id_morceau::INCONNU)};

    const auto nombre_morceaux_max = taille / sizeof(danjo::id_morceau);

    auto ptr_morceaux = reinterpret_cast<danjo::id_morceau *>(tampon);

    for (size_t i = 0ul; i < nombre_morceaux_max; ++i) {
        auto id = static_cast<danjo::id_morceau>(dist(rd));
        *ptr_morceaux++ = id;
    }
}

int test(const u_char *tampon, size_t taille)
{
    const auto nombre_morceaux_max = taille / sizeof(danjo::id_morceau);
    auto ptr_morceaux = reinterpret_cast<const danjo::id_morceau *>(tampon);

    auto donnees_morceaux = dls::tableau<danjo::DonneesMorceaux>{};
    donnees_morceaux.reserve(static_cast<long>(nombre_morceaux_max));

    for (size_t i = 0ul; i < nombre_morceaux_max; ++i) {
        auto dm = danjo::DonneesMorceaux{};
        dm.chaine = "analyse_aleatoire";
        dm.genre = *ptr_morceaux++;
    }

    auto tampon_donnees = lng::tampon_source{"analyse_aleatoire"};

    auto analyseuse = danjo::AnalyseuseDisposition(tampon_donnees, donnees_morceaux);

    try {
        analyseuse.lance_analyse(std::cerr);
    }
    catch (...) {
    }

    return 0;
}

} /* namespace analyse_aleatoire */

namespace analyse_structuree {

/* À FAIRE : création d'un arbre pour générer une structure aléatoire. */

#if 0
struct Noeud {
	dls::tableau<Noeud *> enfants;

	virtual ~Noeud() = default;
	virtual size_t genere_morceaux(danjo::id_morceau *&ptr_morceaux, size_t taille) = 0;
};

struct NoeudDispostion final : public Noeud {
	virtual size_t genere_morceaux(danjo::id_morceau *&ptr_morceaux, size_t taille) override
	{
		if (taille <= 4) {
			return 0;
		}

		auto nombre = 4ul;

		*ptr_morceaux++ = danjo::id_morceau::DISPOSITION;
		*ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_OUVRANTE;

		for (auto enfant : enfants) {
			nombre += enfant->genere_morceaux(ptr_morceaux, taille - 4);
		}

		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;

		return nombre;
	}
};

struct NoeudLigne final : public Noeud {
	virtual size_t genere_morceaux(danjo::id_morceau *&ptr_morceaux, size_t taille) override
	{
		if (taille <= 4) {
			return 0;
		}

		auto nombre = 4ul;

		*ptr_morceaux++ = danjo::id_morceau::LIGNE;
		*ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_OUVRANTE;

		for (auto enfant : enfants) {
			nombre += enfant->genere_morceaux(ptr_morceaux, taille - 4);
		}

		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;

		return nombre;
	}
};

struct NoeudColonne final : public Noeud {
	virtual size_t genere_morceaux(danjo::id_morceau *&ptr_morceaux, size_t taille) override
	{
		if (taille <= 4) {
			return 0;
		}

		auto nombre = 4ul;

		*ptr_morceaux++ = danjo::id_morceau::COLONNE;
		*ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_OUVRANTE;

		for (auto enfant : enfants) {
			nombre += enfant->genere_morceaux(ptr_morceaux, taille - 4);
		}

		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;

		return nombre;
	}
};

struct NoeudControle final : public Noeud {
	virtual size_t genere_morceaux(danjo::id_morceau *&ptr_morceaux, size_t taille) override
	{
		if (taille <= 4) {
			return 0;
		}

		auto nombre = 4ul;

		*ptr_morceaux++ = danjo::id_morceau::ENTIER;
		*ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
		*ptr_morceaux++ = danjo::id_morceau::PARENTHESE_OUVRANTE;

		for (auto enfant : enfants) {
			nombre += enfant->genere_morceaux(ptr_morceaux, taille - 4);
		}

		*ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;

		return nombre;
	}
};

struct Arbre {
	dls::tableau<Noeud *> noeuds{};
	Noeud *racine;

	Arbre()
		: racine(new NoeudDispostion{})
	{
		noeuds.ajoute(racine);
	}
};
#endif

static size_t genere_ligne(danjo::id_morceau *ptr_morceaux, size_t taille)
{
    if (taille < 4) {
        return 0;
    }

    *ptr_morceaux++ = danjo::id_morceau::LIGNE;
    *ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
    *ptr_morceaux++ = danjo::id_morceau::ACCOLADE_OUVRANTE;

    auto nombre = genere_ligne(ptr_morceaux, taille - 4);

    *ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;

    return nombre;
}

void init(u_char *tampon, size_t taille)
{
    auto rd = std::random_device{};
    //	auto dist = std::uniform_int_distribution<int>{
    //				0,
    //				static_cast<int>(danjo::id_morceau::INCONNU)};

    auto nombre_morceaux_max = taille / sizeof(danjo::id_morceau);

    auto ptr_morceaux = reinterpret_cast<danjo::id_morceau *>(tampon);

    *ptr_morceaux++ = danjo::id_morceau::DISPOSITION;
    *ptr_morceaux++ = danjo::id_morceau::CHAINE_CARACTERE;
    *ptr_morceaux++ = danjo::id_morceau::ACCOLADE_OUVRANTE;

    for (size_t i = 0ul; i < nombre_morceaux_max - 4; ++i) {
        i += genere_ligne(ptr_morceaux, nombre_morceaux_max - 4 - i);
    }

    *ptr_morceaux++ = danjo::id_morceau::ACCOLADE_FERMANTE;
}

} /* namespace analyse_structuree */

int main()
{
    auto testeuse = dls::test_aleatoire::Testeuse{};
    testeuse.ajoute_tests("analyse_aleatoire", analyse_aleatoire::init, analyse_aleatoire::test);
    //	testeuse.ajoute_tests("analyse_structuree", analyse_structuree::init,
    //analyse_aleatoire::test);

    return testeuse.performe_tests(std::cerr);
}

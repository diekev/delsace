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

#include "biblinternes/graphe/compileuse_graphe.h"

#include "analyseuse_grammaire.h"
#include "decoupeuse.h"
#include "execution_pile.hh"
#include "modules.hh"

/* Tests à écrire :
 * - découpage/analyse
 * - déclaration variable
 * - accès variable
 * - création attribut via @
 * - modification données entrées (P, attributs, etc...)
 * - appel fonction (foo(a), a.foo())
 * - multiples valeurs retour
 * - CLAS tableaux (et chaînes quand il faut)
 * - opérateurs mathématiques, et assignations opérées (+=, -=, ...)
 * - accès en lecture seule de certains attributs (temps, cadence, index, ...)
 * - accès en lecture seule des propriétés extra des Manipulables
 * - boucles, contrôle de flux
 * - fonctions de modifications de Corps
 * - math matrices
 * - accès membres
 * - construction types défaut (vecN, matN)
 */

int main()
{
	auto &os = std::cerr;

	auto contexte = ContexteGenerationCode{};

	auto donnees_module = contexte.cree_module("racine");
	//donnees_module->tampon = TamponSource("offset = traduit($P, 0, 1, -1, 1); $P = bruit_turbulent(7, offset, $P);");
	//donnees_module->tampon = TamponSource("v0, v1, v2 = sépare_vecteur(v);");
	//donnees_module->tampon = TamponSource("v0, v1 = base_orthonormale($P); $P = réfléchi(v0, v1);");
	//donnees_module->tampon = TamponSource("a = vec3(1.5, 6.7, 9.8); b = vec3(25.0, 16.0, 92.0); $P = -(a * b);\n");
	//donnees_module->tampon = TamponSource("tabl = [4, 3, 5, 7, 9];\n");
	//donnees_module->tampon = TamponSource("pour i dans 0...10 { $P.x = 2.0 * i; } $P.z = 5.0;\n");
	//donnees_module->tampon = TamponSource("@C.r = 1.0;\n");
	//donnees_module->tampon = TamponSource("$P *= 2.0;\n");
	donnees_module->tampon = TamponSource("x = 0; y = 0; pour i dans 0...32 { $P.x = 2.0 * i; x += 5; } pour i dans 0...32 { $P.x = 2.0 * i; } \n");

	try {
		auto decoupeuse = decoupeuse_texte(donnees_module);
		decoupeuse.genere_morceaux();
		decoupeuse.imprime_morceaux(os);

		auto assembleuse = assembleuse_arbre(contexte);

		auto analyseuse = analyseuse_grammaire(contexte, donnees_module);

		analyseuse.lance_analyse(os);

		assembleuse.imprime_code(os);

		auto &gest_props = contexte.gest_props;
		auto &gest_attrs = contexte.gest_attrs;

		auto compileuse = compileuse_lng{};

		auto idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::VEC3));
		gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);

		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::DEC));
		gest_props.ajoute_propriete("index", lcc::type_var::DEC, idx);

		idx = compileuse.donnees().loge_donnees(taille_type(lcc::type_var::COULEUR));
		gest_attrs.ajoute_propriete("C", lcc::type_var::COULEUR, idx);

		assembleuse.genere_code(contexte, compileuse);

		os << "------------------------------------------------------------------\n";
		os << "Taille pile données      : " << compileuse.donnees().taille() << '\n';
		os << "Taille pile instructions : " << compileuse.instructions().taille() << '\n';

		dls::math::vec3f entrees[8] = {
			normalise(dls::math::vec3f(1.0f, 3.0f, 0.75f)),
			normalise(dls::math::vec3f(2.0f, 4.0f, 0.75f)),
			normalise(dls::math::vec3f(3.0f, 5.0f, 0.75f)),
			normalise(dls::math::vec3f(4.0f, 6.0f, 0.75f)),
			normalise(dls::math::vec3f(5.0f, 7.0f, 0.75f)),
			normalise(dls::math::vec3f(6.0f, 8.0f, 0.75f)),
			normalise(dls::math::vec3f(7.0f, 9.0f, 0.75f)),
			normalise(dls::math::vec3f(8.0f, 10.0f, 0.75f)),
		};

		auto ctx_exec = lcc::ctx_exec{};
		auto ctx_local = lcc::ctx_local{};

		for (auto entree : entrees) {
			ctx_local.reinitialise();

			idx = gest_props.pointeur_donnees("P");
			compileuse.donnees().stocke(idx, entree);

			idx = gest_props.pointeur_donnees("index");
			compileuse.donnees().stocke(idx, 1024);

			lcc::execute_pile(
						ctx_exec,
						ctx_local,
						compileuse.donnees(),
						compileuse.instructions(),
						0);

			auto idx_sortie = gest_props.pointeur_donnees("P");

			auto sortie = compileuse.donnees().charge_vec3(idx_sortie);

//			for (auto f = compileuse.debut_donnees(); f != compileuse.fin_donnees(); ++f) {
//				os << *f << '\n';
//			}

			os << "Graphe exécuté :\n"
			   << " - entrée : " << entree << '\n'
			   << " - sortie : " << sortie << '\n';

//			auto &tableau = ctx_local.tableaux.tableau(0);

//			os << "taille tableau : " << tableau.taille() << '\n';

//			for (auto v : tableau) {
//				os << v << ',';
//			}
//			os << '\n';
		}
	}
	catch (erreur::frappe const &e) {
		os << e.message() << '\n';
	}

	return 0;
}

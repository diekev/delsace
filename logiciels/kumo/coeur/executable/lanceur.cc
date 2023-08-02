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

#include <cstring>
#include <filesystem>
#include <iostream>

#include "biblinternes/flux/outils.h"
#include "biblinternes/langage/tampon_source.hh"

#include "decoupage/analyseuse_grammaire.hh"
#include "decoupage/decoupeuse.hh"
#include "decoupage/erreur.hh"

static lng::tampon_source charge_fichier(const char *chemin_fichier)
{
	std::ifstream fichier;
	fichier.open(chemin_fichier);

	fichier.seekg(0, fichier.end);
	auto const taille_fichier = fichier.tellg();
	fichier.seekg(0, fichier.beg);

	dls::chaine res;
	res.reserve(taille_fichier);

	dls::flux::pour_chaque_ligne(fichier, [&](dls::chaine const &ligne)
	{
		res += ligne;
		res.ajoute('\n');
	});

	return lng::tampon_source(std::move(res));
}

struct OptionsCompilation {
	const char *chemin_fichier = nullptr;
};

static OptionsCompilation genere_options_compilation(int argc, char **argv)
{
	OptionsCompilation opts;

	for (int i = 1; i < argc; ++i) {
		opts.chemin_fichier = argv[i];
	}

	return opts;
}

static void imprime_type(std::ostream &os, const Colonne &colonne, bool est_ref)
{
	switch (colonne.type) {
		default:
			break;
		case id_morceau::ENTIER:
			if (colonne.octet == 1) {
				os << "TINYINT";
			}
			else if (colonne.octet == 2) {
				os << "SMALLINT";
			}
			else if (colonne.octet == 3) {
				os << "MIDDLEINT";
			}
			else if (colonne.octet == 4) {
				os << "INT";
			}
			else if (colonne.octet == 8) {
				os << "BIGINT";
			}

			if (colonne.taille != 0) {
				os << '(' << colonne.taille << ')';
			}

			if (colonne.signee == false) {
				os << " UNSIGNED";
			}
			break;
		case id_morceau::BIT:
			os << "BIT" << '(' << colonne.taille << ')';
			break;
		case id_morceau::CHAINE:
			if (colonne.variable) {
				os << "VAR";
			}

			os << "CHAR(" << colonne.taille << ')';
			break;
		case id_morceau::BINAIRE:
			if (colonne.variable) {
				os << "VAR";
			}

			os << "BINARY";
			break;
		case id_morceau::TEMPS:
			os << "TIMESTAMP";
			break;
		case id_morceau::TEMPS_DATE:
			os << "DATETIME";
			break;
		case id_morceau::TEXTE:
			os << "TEXT";
			break;
	}

	if (colonne.a_valeur_defaut) {
		os << " DEFAULT";

		switch (colonne.id_valeur_defaut) {
			default:
				break;
			case id_morceau::TEMPS_COURANT:
				os << " CURRENT_TIMESTAMP";
				break;
			case id_morceau::NUL:
				os << " NULL";
				break;
		}
	}

	if (!est_ref && colonne.cle_primaire) {
		os << " PRIMARY KEY";
	}

	if (!est_ref && colonne.auto_inc) {
		os << " AUTO_INCREMENT";
	}

	if (est_ref || !colonne.peut_etre_nulle) {
		os << " NOT NULL";
	}
}

const Table &trouve_table(const Schema &schema, const dls::chaine &nom)
{
	for (const auto &table : schema.tables) {
		if (table.nom == nom) {
			return table;
		}
	}

	throw "table inconnue";
}

const Colonne &trouve_colonne(const Table &table, const dls::chaine &nom)
{
	for (const auto &colonne : table.colonnes) {
		if (colonne.nom == nom) {
			return colonne;
		}
	}

	throw "colonne inconnue";
}

static void imprime_cles_etrangeres(std::ostream &os, const Table &table)
{
	for (const auto &colonne : table.colonnes) {
		if (colonne.type != id_morceau::CLE) {
			continue;
		}

		os << ",\n";
		os << "FOREIGN KEY (" << colonne.nom
		   << ") REFERENCES " << colonne.table << '(' << colonne.ref << ')'
		   << " ON DELETE CASCADE";
	}
}

static void imprime_creation_schema(std::ostream &os, const Schema *schema)
{
	for (const auto &table : schema->tables) {
		os << "CREATE TABLE IF NOT EXISTS " << table.nom << " (";

		auto debut = true;

		for (const auto &colonne : table.colonnes) {
			if (!debut) {
				os << ",\n";
			}

			os << colonne.nom << ' ';

			if (colonne.type == id_morceau::CLE) {
				const auto &tab_ref = trouve_table(*schema, colonne.table);
				const auto &col_ref = trouve_colonne(tab_ref, colonne.ref);

				/* À FAIRE : TESTS. */
				if (!col_ref.cle_primaire) {
					std::cerr << "\nErreur lors de l'ajout d'une clé étrangère pour colonne '"<< colonne.nom  << "' de la table '" << table.nom << "' !\n";
					std::cerr << "La colonne '" << col_ref.nom << "' de la table '" << tab_ref.nom << "' n'est pas une clé primaire !\n";
					return;
				}

				imprime_type(os, col_ref, true);
			}
			else {
				imprime_type(os, colonne, false);
			}

			debut = false;
		}

		imprime_cles_etrangeres(os, table);

		os << ");\n";
	}
}

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER [options...]\n";
		return 1;
	}

	const auto ops = genere_options_compilation(argc, argv);

	const auto chemin_fichier = ops.chemin_fichier;

	if (chemin_fichier == nullptr) {
		std::cerr << "Aucun fichier spécifié !\n";
		return 1;
	}

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	auto tampon = charge_fichier(chemin_fichier);

	try {
		auto decoupeuse = decoupeuse_texte(tampon);
		decoupeuse.genere_morceaux();

		auto analyseuse = analyseuse_grammaire(decoupeuse.morceaux(), tampon);
		analyseuse.lance_analyse(std::cerr);

		auto schema = analyseuse.schema();
		imprime_creation_schema(std::cout, schema);
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}

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

#include "tests_arachne.hh"

#include <list>

#include "../biblinternes/systeme_fichier/utilitaires.h"

#include "../logiciels/arachne/sources/analyseuse.h"
#include "../logiciels/arachne/sources/gestionnaire.h"
#include "../logiciels/arachne/sources/graphe.h"

namespace arachne {

// NOEUD (A_ETIQUETTE '') (A_PROPRIETE '') A_RELATION_ENTRANTE (A_PROPRIETE '') NOEUD (A_ETIQUETTE '') (A_PROPRIETE '')
// NOEUD A_ETIQUETTE "utilisateur" A_PROPRIETE "valeur"

enum {
	NOEUD,
	RELATION,
	VALEUR,

	A_PROPRIETE,
	A_RELATION_ENTRANTE,
	A_RELATION_SORTANTE,
	A_RELATION,
	A_ETIQUETTE,
};

class resultat_analyseuse {
	std::list<noeud *> m_noeuds{};

public:
	void ajoute_noeud(noeud *n)
	{
		m_noeuds.push_back(n);
	}

	/**
	 * Nombre de résultats.
	 */
	size_t nombre() const
	{
		return m_noeuds.size();
	}
};

class analyseuse_graphe final : public lng::analyseuse<arachne::DonneesMorceaux> {
	const graphe &m_graphe;
	resultat_analyseuse m_resultat{};

public:
	explicit analyseuse_graphe(const graphe &g, std::vector<arachne::DonneesMorceaux> &donnees)
		: lng::analyseuse<arachne::DonneesMorceaux>(donnees)
		, m_graphe(g)
	{}

	void lance_analyse(std::ostream &os) override
	{
		m_position = 0;

		if (est_identifiant(NOEUD)) {
			analyse_noeud();
		}
//		else if (est_identifiant(RELATION)) {
//			analyse_relation();
//		}
	}

	void analyse_noeud()
	{
		if (!requiers_identifiant(NOEUD)) {
			return;
		}

#if 0
		for (noeud *n : m_graphe->noeuds()) {
			if (est_identifiant(A_ETIQUETTE)) {
				avance();

				if (!analyse_etiquette(n)) {
					continue;
				}
			}
			else if (est_identifiant(A_RELATION)) {
				avance();

				if (!analyse_relation(n)) {
					continue;
				}
			}
			else {
				continue;
			}

			m_resultat.ajoute_noeud(n);
		}
#else
		if (est_identifiant(A_ETIQUETTE)) {
			avance();

			for (noeud *n : m_graphe.noeuds()) {
				if (!analyse_etiquette(n)) {
					continue;
				}

				m_resultat.ajoute_noeud(n);
			}
		}
#endif
	}

	bool analyse_etiquette(noeud *n)
	{
		auto contenu = donnees().valeur;

		for (etiquette *e : n->etiquettes) {
			if (e->id_nom == contenu) {
				return true;
			}
		}

		return false;
	}

	resultat_analyseuse resultat() const
	{
		return m_resultat;
	}
};

#if 0
void verifie_condition(std::vector<int> conditions)
{
	for (auto condition : conditions) {
		if (condition == NOEUD) {
			noeud *n;

			if (condition == A_PROPRIETE) {
				for (propriete *p : n->proprietes) {
					if (p->id_nom == condition) {
						return /*true*/;
					}
				}

				return /* false*/;
			}

			if (condition == A_ETIQUETTE) {
				for (etiquette *e : n->proprietes) {
					if (e->id_nom == condition) {
						return /*true*/;
					}
				}

				return /* false*/;
			}

			if (condition == A_RELATION_ENTRANTE) {
				for (relation *r : n->relations) {
					if (r->fin == n) {
						return /* true */;
					}
				}
			}
		}

		if (condition == RELATION) {
			relation *r;

			if (condition == A_PROPRIETE) {
				for (propriete *p : r->proprietes) {
					if (p->id_nom == condition) {
						return /*true*/;
					}
				}

				return /* false*/;
			}
		}
	}
}
#endif
}  /* namespace arachne */

void test_analyse_graphe(const arachne::graphe &graphe, dls::test_unitaire::Controleuse &controlleur)
{
	{
		std::vector<arachne::DonneesMorceaux> donnees({
			{ arachne::NOEUD, 0 },
			{ arachne::A_ETIQUETTE, graphe.id_etiquette("utilisateur") },
		});

		arachne::analyseuse_graphe analyseuse(graphe, donnees);
		analyseuse.lance_analyse(std::cerr);

		auto resultat = analyseuse.resultat();

		CU_VERIFIE_CONDITION(controlleur, resultat.nombre() == 2);
	}

	{
		std::vector<arachne::DonneesMorceaux> donnees({
			{ arachne::NOEUD, 0u },
			{ arachne::A_ETIQUETTE, graphe.id_etiquette("utilisatrice") },
		});

		arachne::analyseuse_graphe analyseuse(graphe, donnees);
		analyseuse.lance_analyse(std::cerr);

		auto resultat = analyseuse.resultat();

		CU_VERIFIE_CONDITION(controlleur, resultat.nombre() == 0);
	}

	{
		std::vector<arachne::DonneesMorceaux> donnees({
			{ arachne::NOEUD, 0u },
			{ arachne::A_ETIQUETTE, graphe.id_etiquette("utilisateur") },
			{ arachne::A_RELATION,  graphe.id_etiquette("relation") },
			{ arachne::NOEUD, 0u },
			{ arachne::A_ETIQUETTE, graphe.id_etiquette("utilisateur") },
		});

		arachne::analyseuse_graphe analyseuse(graphe, donnees);
		analyseuse.lance_analyse(std::cerr);

		auto resultat = analyseuse.resultat();

		CU_VERIFIE_CONDITION(controlleur, resultat.nombre() == 2);
	}
}

void test_graphe(dls::test_unitaire::Controleuse &controlleur)
{
	/* Création des dossiers. */

	arachne::gestionnaire gestionnaire;
	gestionnaire.cree_base_donnees("test");

	const auto dossier_bd = gestionnaire.chemin_courant();

	arachne::infos_fichiers infos;
	infos.chemin_fichier_noeud = dossier_bd / arachne::FICHIER_NOEUD;
	infos.chemin_fichier_propriete = dossier_bd / arachne::FICHIER_PROPRIETE;
	infos.chemin_fichier_nom_proprietes = dossier_bd / arachne::FICHIER_NOM_PROPRIETE;
	infos.chemin_fichier_relation = dossier_bd / arachne::FICHIER_RELATION;
	infos.chemin_fichier_type_relation = dossier_bd / arachne::FICHIER_TYPE_RELATION;
	infos.chemin_fichier_etiquette = dossier_bd / arachne::FICHIER_ETIQUETTE;
	infos.chemin_fichier_nom_etiquette = dossier_bd / arachne::FICHIER_NOM_ETIQUETTE;

	arachne::graphe graphe(infos);

	auto noeud1 = graphe.ajoute_noeud("valeur", 5);
	graphe.ajoute_propriete(noeud1, "contrevaleur", 10);
	graphe.ajoute_etiquette(noeud1, "utilisateur");

	auto noeud2 = graphe.ajoute_noeud("valeur", 9);
	graphe.ajoute_etiquette(noeud2, "utilisateur");

	graphe.ajoute_relation(noeud1, noeud2, "relation");
	graphe.ajoute_relation(noeud2, noeud1, "relation");

	graphe.ecris_noeud();
	graphe.ecris_proprietes();
	graphe.ecris_relations();
	graphe.ecris_noms_proprietes();
	graphe.ecris_types_relations();
	graphe.ecris_noms_etiquettes();

	graphe.lis_noeud();
	graphe.lis_proprietes();
	graphe.lis_relations();
	graphe.lis_noms_proprietes();
	graphe.lis_types_relations();
	graphe.lis_noms_etiquettes();

	test_analyse_graphe(graphe, controlleur);
}

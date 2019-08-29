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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrice_graphe_detail.hh"

#include "lcc/arbre_syntactic.h"
#include "lcc/code_inst.hh"
#include "lcc/lcc.hh"

#include "contexte_evaluation.hh"
#include "donnees_aval.hh"
#include "objet.h"
#include "mikisa.h"
#include "usine_operatrice.h"

/* ************************************************************************** */

OperatriceGrapheDetail::OperatriceGrapheDetail(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);
}

const char *OperatriceGrapheDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceGrapheDetail::texte_aide() const
{
	return AIDE;
}

const char *OperatriceGrapheDetail::chemin_entreface() const
{
	return "";
}

Graphe *OperatriceGrapheDetail::graphe()
{
	return &m_graphe;
}

int OperatriceGrapheDetail::type() const
{
	return OPERATRICE_GRAPHE_DETAIL;
}

int OperatriceGrapheDetail::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (!this->entree(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return EXECUTION_ECHOUEE;
	}

	m_corps.reinitialise();
	entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

	if (!compile_graphe(contexte)) {
		ajoute_avertissement("Ne peut pas compiler le graphe, voir si les noeuds n'ont pas d'erreurs.");
		return EXECUTION_ECHOUEE;
	}

	auto points = m_corps.points_pour_ecriture();

	/* fais une copie locale pour éviter les problèmes de concurrence critique */
	auto donnees = m_compileuse.donnees();

	auto ctx_exec = lcc::ctx_exec{};
	auto ctx_local = lcc::ctx_local{};

	for (auto i = 0; i < points->taille(); ++i) {
		auto pos = points->point(i);

		remplis_donnees(donnees, m_gest_props, "P", pos);

		lcc::execute_pile(
					ctx_exec,
					ctx_local,
					donnees,
					m_compileuse.instructions(),
					i);

		auto idx_sortie = m_gest_props.pointeur_donnees("P");
		pos = donnees.charge_vec3(idx_sortie);

		points->point(i, pos);
	}

	return EXECUTION_REUSSIE;
}

bool OperatriceGrapheDetail::compile_graphe(const ContexteEvaluation &contexte)
{
	m_compileuse = compileuse_lng();
	m_gest_props = gestionnaire_propriete();

	if (m_graphe.besoin_ajournement) {
		tri_topologique(m_graphe);
		m_graphe.besoin_ajournement = false;
	}

	auto donnees_aval = DonneesAval{};
	donnees_aval.table.insere({"compileuse", &m_compileuse});
	donnees_aval.table.insere({"gest_props", &m_gest_props});

	auto idx = m_compileuse.donnees().loge_donnees(lcc::taille_type(lcc::type_var::VEC3));
	m_gest_props.ajoute_propriete("P", lcc::type_var::VEC3, idx);

	for (auto &noeud : m_graphe.noeuds()) {
		for (auto &sortie : noeud->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = extrait_opimage(noeud->donnees());
		operatrice->reinitialise_avertisements();

		auto resultat = operatrice->execute(contexte, &donnees_aval);

		if (resultat == EXECUTION_ECHOUEE) {
			return false;
		}
	}

	return true;
}

/* ************************************************************************** */

static auto converti_type_prise(lcc::type_var type)
{
	switch (type) {
		case lcc::type_var::DEC:
			return type_prise::DECIMAL;
		case lcc::type_var::ENT32:
			return type_prise::ENTIER;
		case lcc::type_var::VEC2:
			return type_prise::VEC2;
		case lcc::type_var::VEC3:
			return type_prise::VEC3;
		case lcc::type_var::VEC4:
			return type_prise::VEC4;
		case lcc::type_var::COULEUR:
			return type_prise::COULEUR;
		case lcc::type_var::MAT3:
			return type_prise::MAT3;
		case lcc::type_var::MAT4:
			return type_prise::MAT4;
		case lcc::type_var::CHAINE:
			return type_prise::CHAINE;
		case lcc::type_var::INVALIDE:
			return type_prise::INVALIDE;
		case lcc::type_var::TABLEAU:
			return type_prise::TABLEAU;
		case lcc::type_var::POLYMORPHIQUE:
			return type_prise::POLYMORPHIQUE;
	}

	return type_prise::INVALIDE;
}

static auto converti_type_prise(type_prise type)
{
	switch (type) {
		case type_prise::DECIMAL:
			return lcc::type_var::DEC;
		case type_prise::ENTIER:
			return lcc::type_var::ENT32;
		case type_prise::VEC2:
			return lcc::type_var::VEC2;
		case type_prise::VEC3:
			return lcc::type_var::VEC3;
		case type_prise::VEC4:
			return lcc::type_var::VEC4;
		case type_prise::COULEUR:
			return lcc::type_var::COULEUR;
		case type_prise::MAT3:
			return lcc::type_var::MAT3;
		case type_prise::MAT4:
			return lcc::type_var::MAT4;
		case type_prise::CHAINE:
			return lcc::type_var::CHAINE;
		case type_prise::INVALIDE:
			return lcc::type_var::INVALIDE;
		case type_prise::TABLEAU:
			return lcc::type_var::TABLEAU;
		case type_prise::POLYMORPHIQUE:
			return lcc::type_var::POLYMORPHIQUE;
		case type_prise::CORPS:
		case type_prise::IMAGE:
		case type_prise::OBJET:
			return lcc::type_var::INVALIDE;
	}

	return lcc::type_var::INVALIDE;
}

OperatriceFonctionDetail::OperatriceFonctionDetail(
		Graphe &graphe_parent,
		Noeud *noeud,
		const dls::chaine &nom_fonc,
		const lcc::donnees_fonction *df)
	: OperatriceImage(graphe_parent, noeud)
	, donnees_fonction(df)
	, nom_fonction(nom_fonc)
{
	if (donnees_fonction == nullptr) {
		entrees(0);
		sorties(0);
		return;
	}

	entrees(donnees_fonction->seing.entrees.types.taille());
	sorties(donnees_fonction->seing.sorties.types.taille());
}

const char *OperatriceFonctionDetail::nom_classe() const
{
	return NOM;
}

const char *OperatriceFonctionDetail::texte_aide() const
{
	return AIDE;
}

int OperatriceFonctionDetail::type() const
{
	return OPERATRICE_DETAIL;
}

type_prise OperatriceFonctionDetail::type_entree(int i) const
{
	return converti_type_prise(donnees_fonction->seing.entrees.types[i]);
}

type_prise OperatriceFonctionDetail::type_sortie(int i) const
{
	return converti_type_prise(donnees_fonction->seing.sorties.types[i]);
}

inline auto corrige_type_specialise(lcc::type_var type_specialise, lcc::type_var type)
{
	if (type_specialise == lcc::type_var::INVALIDE) {
		return type;
	}

	if (taille_type(type) > taille_type(type_specialise)) {
		return type;
	}

	return type_specialise;
}

#undef DEBOGUE_SPECIALISATION

int OperatriceFonctionDetail::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	INUTILISE(contexte);
	/* réimplémentation du code de génération d'instruction pour les appels de
	 * fonctions de LCC */

	auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);

	/* défini le type spécialisé pour les connexions d'entrées */

	auto type_params = lcc::types_entrees();
	auto type_specialise = lcc::type_var::INVALIDE;
	auto est_polymorphique = false;

	/* première boucle : défini le type spécialisé pour les connexions d'entrées */
	for (auto i = 0; i < entrees(); ++i) {
		auto type = donnees_fonction->seing.entrees.types[i];
		est_polymorphique = (type == lcc::type_var::POLYMORPHIQUE);

		if (entree(i)->connectee()) {
			auto ptr = entree(i)->pointeur();
			auto prise_sortie = ptr->liens[0];

			if (type == lcc::type_var::POLYMORPHIQUE) {
				type = converti_type_prise(prise_sortie->type_infere);
				type_specialise = corrige_type_specialise(type_specialise, type);
			}
		}

		type_params.ajoute(type);
	}

	/* deuxième boucle : cherche les pointeurs des entrées et converti les
	 * valeurs selon le type spécialisé */

	if (est_polymorphique && type_specialise == lcc::type_var::INVALIDE) {
		this->ajoute_avertissement("Ne peut pas instancier la fonction car les entrées polymorphiques n'ont pas de connexion.");
		return EXECUTION_ECHOUEE;
	}

#ifdef DEBOGUE_SPECIALISATION
	if (type_specialise != lcc::type_var::INVALIDE) {
		std::cerr << "Spécialisation pour type : " << lcc::chaine_type_var(type_specialise) << '\n';
	}
#endif

	auto pointeurs = dls::tableau<int>();

	for (auto i = 0; i < entrees(); ++i) {
		auto type = donnees_fonction->seing.entrees.types[i];

		if (entree(i)->connectee()) {
			auto ptr = entree(i)->pointeur();
			auto prise_sortie = ptr->liens[0];
			auto decalage = static_cast<int>(prise_sortie->decalage_pile);

			/* converti au besoin */
			if (type == lcc::type_var::POLYMORPHIQUE) {
				auto type_prise = converti_type_prise(prise_sortie->type_infere);

				if (type_prise != type_specialise) {
#ifdef DEBOGUE_SPECIALISATION
					std::cerr << "Conversion de "
							  << lcc::chaine_type_var(type_prise)
							  << " vers "
							  << lcc::chaine_type_var(type_specialise) << '\n';
#endif
					decalage = lcc::ajoute_conversion(*compileuse, type_prise, type_specialise, decalage);
				}
			}

			pointeurs.pousse(decalage);
		}
		else {
			/* alloue une valeur par défaut et prend le pointeurs */
			auto ptr = 0;

			if (type == lcc::type_var::POLYMORPHIQUE) {
				ptr = compileuse->donnees().loge_donnees(lcc::taille_type(type_specialise));
				auto ptr_loc = ptr;

				auto valeur = evalue_vecteur("entrée" + dls::vers_chaine(i), contexte.temps_courant);

				switch (type_specialise) {
					case lcc::type_var::ENT32:
					{
						compileuse->donnees().stocke(ptr_loc, static_cast<int>(valeur[0]));
						break;
					}
					case lcc::type_var::DEC:
					{
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						break;
					}
					case lcc::type_var::VEC2:
					{
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						compileuse->donnees().stocke(ptr_loc, valeur[1]);
						break;
					}
					default:
					{
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
				}
			}
			else {
				ptr = compileuse->donnees().loge_donnees(lcc::taille_type(type));
				auto ptr_loc = ptr;

				auto nom_prop = "entrée" + dls::vers_chaine(i);

				switch (type) {
					case lcc::type_var::ENT32:
					{
						auto valeur = evalue_entier(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::DEC:
					{
						auto valeur = evalue_decimal(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::VEC2:
					{
						auto valeur = evalue_vecteur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur[0]);
						compileuse->donnees().stocke(ptr_loc, valeur[1]);
						break;
					}
					case lcc::type_var::VEC3:
					case lcc::type_var::VEC4:
					{
						auto valeur = evalue_vecteur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					case lcc::type_var::COULEUR:
					{
						auto valeur = evalue_couleur(nom_prop, contexte.temps_courant);
						compileuse->donnees().stocke(ptr_loc, valeur);
						break;
					}
					default:
					{
						break;
					}
				}
			}

			pointeurs.pousse(ptr);
		}
	}

	/* ****************** crée les données pour les appels ****************** */

	/* ajoute le code_inst de la fonction */
	compileuse->ajoute_instructions(donnees_fonction->type);

	/* ajoute le type de la fonction pour choisir la bonne surcharge */
	if (type_specialise != lcc::type_var::INVALIDE) {
		compileuse->ajoute_instructions(type_specialise);
	}

	/* ajoute le pointeur de chaque paramètre */
	for (auto ptr : pointeurs) {
		compileuse->ajoute_instructions(ptr);
	}

	/* pour chaque sortie, nous réservons de la place sur la pile de données */
	auto pointeur_donnees = 0;
	for (auto i = 0; i < sorties(); ++i) {
		auto type = donnees_fonction->seing.sorties.types[i];

		if (type == lcc::type_var::POLYMORPHIQUE) {
			type = type_specialise;
		}

		auto pointeur = compileuse->donnees().loge_donnees(lcc::taille_type(type));
		auto prise_sortie = sortie(i)->pointeur();

		prise_sortie->decalage_pile = pointeur;
		prise_sortie->type_infere = converti_type_prise(type);

		if (i == 0) {
			pointeur_donnees = pointeur;
		}
	}

	/* ajoute le pointeur du premier paramètre aux instructions pour savoir
	 * où écrire */
	compileuse->ajoute_instructions(pointeur_donnees);

	return EXECUTION_REUSSIE;
}

void OperatriceFonctionDetail::cree_proprietes()
{
	for (auto i = 0; i < entrees(); ++i) {
		auto nom_propriete = "entrée" + dls::vers_chaine(i);

		auto prop = danjo::Propriete();

		switch (donnees_fonction->seing.entrees.types[i]) {
			case lcc::type_var::DEC:
			{
				prop.type = danjo::TypePropriete::DECIMAL;
				prop.valeur = 0.0f;
				ajoute_propriete(nom_propriete, danjo::TypePropriete::DECIMAL, 0.0f);
				break;
			}
			case lcc::type_var::ENT32:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = 0;
				break;
			}
			case lcc::type_var::VEC2:
			case lcc::type_var::VEC3:
			case lcc::type_var::VEC4:
			case lcc::type_var::POLYMORPHIQUE:
			{
				prop.type = danjo::TypePropriete::VECTEUR;
				prop.valeur = dls::math::vec3f(0.0f);
				break;
			}
			case lcc::type_var::COULEUR:
			{
				prop.type = danjo::TypePropriete::COULEUR;
				prop.valeur = dls::phys::couleur32(1.0f);
				break;
			}
			case lcc::type_var::CHAINE:
			{
				prop.type = danjo::TypePropriete::CHAINE_CARACTERE;
				prop.valeur = dls::chaine();
				break;
			}
			case lcc::type_var::MAT3:
			case lcc::type_var::MAT4:
			case lcc::type_var::INVALIDE:
			case lcc::type_var::TABLEAU:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = 0;
				break;
			}
		}

		ajoute_propriete_extra(nom_propriete, prop);
	}
}

/* ************************************************************************** */

class OperatriceEntreeDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Entrée Détail";
	static constexpr auto AIDE = "Entrée Détail";

	OperatriceEntreeDetail(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	type_prise type_sortie(int i) const override
	{
		INUTILISE(i);
		return type_prise::VEC3;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto gest_props = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_props"]);

		auto prise_sortie = sortie(0)->pointeur();
		prise_sortie->decalage_pile = gest_props->pointeur_donnees("P");
		prise_sortie->type_infere = type_prise::VEC3;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSortieDetail final : public OperatriceImage {
public:
	static constexpr auto NOM = "Sortie Détail";
	static constexpr auto AIDE = "Sortie Détail";

	OperatriceSortieDetail(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
		sorties(0);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	type_prise type_entree(int i) const override
	{
		INUTILISE(i);
		return type_prise::VEC3;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);

		auto compileuse = std::any_cast<compileuse_lng *>(donnees_aval->table["compileuse"]);
		auto gest_props = std::any_cast<gestionnaire_propriete *>(donnees_aval->table["gest_props"]);

		auto ptr_entree = 0;

		if (entree(0)->connectee()) {
			auto ptr = entree(0)->pointeur();
			ptr_entree = static_cast<int>(ptr->liens[0]->decalage_pile);
		}
		else {
			ptr_entree = compileuse->donnees().loge_donnees(taille_type(lcc::type_var::VEC3));
		}

		auto ptr = gest_props->pointeur_donnees("P");

		compileuse->ajoute_instructions(lcc::code_inst::ASSIGNATION);
		compileuse->ajoute_instructions(lcc::type_var::VEC3);
		compileuse->ajoute_instructions(ptr_entree);
		compileuse->ajoute_instructions(ptr);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void graphe_detail_notifie_parent_suranne(Mikisa &mikisa)
{
	auto noeud_objet = mikisa.bdd.graphe_objets()->noeud_actif;
	auto objet = extrait_objet(noeud_objet->donnees());
	auto graphe = &objet->graphe;
	auto noeud_actif = graphe->noeud_actif;

	/* Marque le noeud courant et ceux en son aval surannées. */
	marque_surannee(noeud_actif, [](Noeud *n, PriseEntree *prise)
	{
		auto op = extrait_opimage(n->donnees());
		op->amont_change(prise);
	});
}

/* ************************************************************************** */

void enregistre_operatrices_detail(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGrapheDetail>());
	usine.enregistre_type(cree_desc<OperatriceEntreeDetail>());
	usine.enregistre_type(cree_desc<OperatriceSortieDetail>());
}

OperatriceFonctionDetail *cree_op_detail(
		Mikisa &mikisa,
		Graphe &graphe,
		Noeud *noeud,
		const dls::chaine &nom_fonction)
{
	/* À FAIRE : gestion des fonctions avec surcharges. */
	auto const &table_df = mikisa.lcc->fonctions.table[nom_fonction];
	auto df = &table_df.front();

	return memoire::loge<OperatriceFonctionDetail>("Fonction Détail", graphe, noeud, nom_fonction, df);
}

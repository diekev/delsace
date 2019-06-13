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

#include "graphe.h"

#include <cassert>

namespace arachne {

graphe::graphe(const infos_fichiers &infos)
	: m_infos_fichiers(infos)
{}

graphe::~graphe()
{
	for (noeud *n : m_noeuds) {
		delete n;
	}

	for (relation *r : m_relations) {
		delete r;
	}

	for (propriete *p : m_proprietes) {
		delete p;
	}

	for (etiquette *e : m_etiquettes) {
		delete e;
	}
}

noeud *graphe::ajoute_noeud(const std::string &nom, int valeur)
{
	std::fprintf(stderr, "Création d'un noeud...\n");

	if (nom.empty()) {
		/* À FAIRE : lance erreur, un noeud doit avoir au moins une
			 * propriété pour l'identifier. */
		return nullptr;
	}

	noeud *n = new noeud;
	n->id = ++m_nombre_noeuds;
	assert(n->id != 0);

	ajoute_propriete(n, nom, valeur);

	m_noeuds.push_back(n);

	return n;
}

void graphe::ajoute_propriete(noeud *n, const std::string &nom, int valeur)
{
	std::fprintf(stderr, "Création d'une propriété <%s, %d> pour un noeud...\n", nom.c_str(), valeur);

	if (n == nullptr || n->id == 0) {
		/* À FAIRE : lance erreur. */
		return;
	}

	if (nom.empty()) {
		/* À FAIRE : lance erreur, une propriété doit avoir un nom pour
			 * pouvoir l'identifier. */
		return;
	}

	propriete *p = new propriete;
	p->suivante = nullptr;
	p->id_nom = m_nom_proprietes.ajoute_chaine(nom);
	p->valeur = valeur;
	p->id = ++m_nombre_proprietes;
	assert(p->id != 0);

	m_proprietes.push_back(p);

	if (!n->proprietes.empty()) {
		n->proprietes.back()->suivante = p;
	}

	n->proprietes.push_back(p);
}

void graphe::ajoute_etiquette(noeud *n, const std::string &nom)
{
	if (n == nullptr || n->id == 0) {
		/* À FAIRE : lance erreur. */
		return;
	}

	if (nom.empty()) {
		/* À FAIRE : lance erreur, une propriété doit avoir un nom pour
			 * pouvoir l'identifier. */
		return;
	}

	std::fprintf(stderr, "Création d'une étiquette <%s> pour un noeud...\n", nom.c_str());

	auto e = new etiquette();
	e->suivante = nullptr;
	e->id = ++m_nombre_etiquettes;
	e->id_nom = m_nom_etiquettes.ajoute_chaine(nom);
	assert(e->id != 0);

	m_etiquettes.push_back(e);

	if (!n->etiquettes.empty()) {
		n->etiquettes.back()->suivante = e;
	}

	n->etiquettes.push_back(e);
}

void graphe::ajoute_propriete(relation *r, const std::string &nom, int valeur)
{
	std::fprintf(stderr, "Création d'une propriété pour une relation...\n");

	if (r == nullptr || r->id == 0) {
		/* À FAIRE : lance erreur. */
		return;
	}

	if (nom.empty()) {
		/* À FAIRE : lance erreur, une propriété doit avoir un nom pour
			 * pouvoir l'identifier. */
		return;
	}

	propriete *p = new propriete;
	p->suivante = nullptr;
	p->id_nom = m_nom_proprietes.ajoute_chaine(nom);
	p->valeur = valeur;
	p->id = ++m_nombre_proprietes;
	assert(p->id != 0);

	m_proprietes.push_back(p);

	if (!r->proprietes.empty()) {
		r->proprietes.back()->suivante = p;
	}

	r->proprietes.push_back(p);
}

void graphe::ajoute_relation(noeud *debut, noeud *fin, const std::string &nom)
{
	if (debut == nullptr || fin == nullptr) {
		/* À FAIRE : lance erreur. */
		return;
	}

	if (debut == fin || debut->id == fin->id) {
		/* À FAIRE : lance erreur. */
		return;
	}

	std::fprintf(stderr, "Création d'une relation entre les noeuds %u et %u...\n", debut->id, fin->id);

	relation *r = new relation;
	r->debut = debut;
	r->fin = fin;
	r->id = ++m_nombre_relations;
	r->type_relation = m_type_relations.ajoute_chaine(nom);
	r->rel_prev_debut = nullptr;
	r->rel_suiv_debut = nullptr;
	r->rel_prev_fin = nullptr;
	r->rel_suiv_fin = nullptr;
	assert(r->id != 0);

	if (!debut->relations.empty()) {
		r->rel_prev_debut = debut->relations.back();
		debut->relations.back()->rel_suiv_debut = r;
	}

	if (!fin->relations.empty()) {
		r->rel_prev_fin = fin->relations.back();
		debut->relations.back()->rel_suiv_fin = r;
	}

	debut->relations.push_back(r);
	fin->relations.push_back(r);

	m_relations.push_back(r);
}

void graphe::ecris_noeud()
{
	std::fprintf(stderr,
				 "Écriture des noeuds dans le fichier %s...\n",
				 m_infos_fichiers.chemin_fichier_noeud.c_str());

	m_autrice_noeuds.ouvre(m_infos_fichiers.chemin_fichier_noeud);

	std::vector<char> tampon;
	tampon.resize(m_noeuds.size() * TAILLE_REGISTRE_NOEUD, '\0');
	auto decalage = 0ul;

	for (noeud *n : m_noeuds) {
		tampon[decalage + 0] = 1; //n->utilise;

		if (!n->relations.empty()) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 1]) = n->relations.front()->id;
		}

		if (!n->proprietes.empty()) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 5]) = n->proprietes.front()->id;
		}

		if (!n->etiquettes.empty()) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 9]) = n->etiquettes.front()->id;
		}

		tampon[decalage + 13] = 0; // drapeaux

		decalage += TAILLE_REGISTRE_NOEUD;
	}

	m_autrice_noeuds.ecrit_tampon(tampon.data(), tampon.size());

	m_autrice_noeuds.ferme();
}

void graphe::lis_noeud()
{
	m_lectrice_noeuds.ouvre(m_infos_fichiers.chemin_fichier_noeud);

	if (!m_lectrice_noeuds.est_ouverte()) {
		return;
	}

	std::fprintf(stderr,
				 "Lecture du fichier %s...\n",
				 m_infos_fichiers.chemin_fichier_noeud.c_str());

	const auto taille_fichier = m_lectrice_noeuds.taille();

	const auto nombre_entrees = taille_fichier / TAILLE_REGISTRE_NOEUD;

	std::fprintf(stderr, "Taille fichier : %lu\n", taille_fichier);

	std::fprintf(stderr,
				 "Il y a %lu entrées dans le fichier.\n",
				 nombre_entrees);

	char tampon[TAILLE_REGISTRE_NOEUD];

	for (size_t i = 0; i < nombre_entrees; ++i) {
		m_lectrice_noeuds.lis_tampon(tampon, TAILLE_REGISTRE_NOEUD);

		auto utilise = tampon[0];
		auto id_relation = *reinterpret_cast<unsigned int *>(&tampon[1]);
		auto id_propriete = *reinterpret_cast<unsigned int *>(&tampon[5]);
		auto id_etiquette = *reinterpret_cast<unsigned int *>(&tampon[9]);
		auto drapeaux = tampon[13];

		std::fprintf(stderr, "-------------------------------\n");
		std::fprintf(stderr, "Registre noeud\n");
		std::fprintf(stderr, "Utilisé : %d\n", utilise);
		std::fprintf(stderr, "id_relation : %u\n", id_relation);
		std::fprintf(stderr, "id_propriete : %u\n", id_propriete);
		std::fprintf(stderr, "id_etiquette : %u\n", id_etiquette);
		std::fprintf(stderr, "drapeaux : %d\n", drapeaux);
	}

	m_lectrice_noeuds.ferme();
}

void graphe::ecris_proprietes()
{
	m_autrice_proprietes.ouvre(m_infos_fichiers.chemin_fichier_propriete);

	std::fprintf(stderr,
				 "Écriture des propriétés dans le fichier %s...\n",
				 m_infos_fichiers.chemin_fichier_propriete.c_str());

	std::vector<char> tampon;
	tampon.resize(m_proprietes.size() * TAILLE_REGISTRE_PROPRIETE, '\0');
	auto decalage = 0ul;

	for (propriete *p : m_proprietes) {
		tampon[decalage + 0] = 1; //p->utilise;
		tampon[decalage + 1] = 0; //p->type;

		/* id de la propriété suivante */
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 2]) = (p->suivante != nullptr) ? p->suivante->id : 0;
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 6]) = p->id_nom; // id_nom_propriete
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 10]) = static_cast<unsigned>(p->valeur); // valeur ou id_valeur

		decalage += TAILLE_REGISTRE_PROPRIETE;
	}

	m_autrice_proprietes.ecrit_tampon(tampon.data(), tampon.size());

	m_autrice_proprietes.ferme();
}

void graphe::lis_proprietes()
{
	m_lectrice_proprietes.ouvre(m_infos_fichiers.chemin_fichier_propriete);

	if (!m_lectrice_proprietes.est_ouverte()) {
		return;
	}

	std::fprintf(stderr, "Lecture du fichier %s...\n", m_infos_fichiers.chemin_fichier_propriete.c_str());

	const auto taille_fichier = m_lectrice_proprietes.taille();

	std::fprintf(stderr, "Taille fichier : %lu\n", taille_fichier);

	const auto nombre_entrees = taille_fichier / TAILLE_REGISTRE_PROPRIETE;

	char tampon[TAILLE_REGISTRE_PROPRIETE];

	for (size_t i = 0; i < nombre_entrees; ++i) {
		m_lectrice_proprietes.lis_tampon(tampon, TAILLE_REGISTRE_PROPRIETE);

		auto utilise = tampon[0];
		auto type = tampon[1];
		auto id_propriete_suivante = *reinterpret_cast<unsigned int *>(&tampon[2]);
		auto id_nom_propriete = *reinterpret_cast<unsigned int *>(&tampon[6]);
		auto valeur = *reinterpret_cast<unsigned int *>(&tampon[10]);

		std::fprintf(stderr, "-------------------------------\n");
		std::fprintf(stderr, "Registre propriété\n");
		std::fprintf(stderr, "Utilisé : %d\n", utilise);
		std::fprintf(stderr, "type : %d\n", type);
		std::fprintf(stderr, "id_propriete_suivante : %u\n", id_propriete_suivante);
		std::fprintf(stderr, "id_nom_propriete : %u\n", id_nom_propriete);
		std::fprintf(stderr, "valeur : %u\n", valeur);
	}

	std::fprintf(stderr, "Il y a %lu entrées dans le fichier.\n", nombre_entrees);

	m_lectrice_proprietes.ferme();
}

void graphe::ecris_noms_proprietes()
{
	ecris_magasin_chaine(m_nom_proprietes, m_infos_fichiers.chemin_fichier_nom_proprietes);
}

void graphe::lis_noms_proprietes()
{
	lis_magasin_chaine(m_infos_fichiers.chemin_fichier_nom_proprietes);
}

void graphe::ecris_relations()
{
	m_autrice_relations.ouvre(m_infos_fichiers.chemin_fichier_relation);

	std::fprintf(stderr,
				 "Écriture de %lu relations dans le fichier %s...\n",
				 m_relations.size(),
				 m_infos_fichiers.chemin_fichier_relation.c_str());

	std::vector<char> tampon;
	tampon.resize(m_relations.size() * TAILLE_REGISTRE_RELATION, '\0');
	auto decalage = 0ul;

	for (relation *r : m_relations) {
		tampon[decalage + 0] = 1;  /* À FAIRE : utilisé */
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 1]) = r->debut->id;
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 5]) = r->fin->id;
		*reinterpret_cast<unsigned int *>(&tampon[decalage + 9]) = r->type_relation;

		if (r->rel_prev_debut != nullptr) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 13]) = r->rel_prev_debut->id;
		}

		if (r->rel_suiv_debut != nullptr) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 17]) = r->rel_suiv_debut->id;
		}

		if (r->rel_prev_fin != nullptr) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 21]) = r->rel_prev_fin->id;
		}

		if (r->rel_suiv_fin != nullptr) {
			*reinterpret_cast<unsigned int *>(&tampon[decalage + 25]) = r->rel_suiv_fin->id;
		}

		*reinterpret_cast<unsigned int *>(&tampon[decalage + 29]) = (r->proprietes.empty()) ? 0 : r->proprietes.front()->id;
		tampon[decalage + 33] = 0; /* À FAIRE : drapeaux */

		decalage += TAILLE_REGISTRE_RELATION;
	}

	m_autrice_relations.ecrit_tampon(tampon.data(), tampon.size());

	m_autrice_relations.ferme();
}

void graphe::lis_relations()
{
	m_lectrice_relations.ouvre(m_infos_fichiers.chemin_fichier_relation);

	if (!m_lectrice_relations.est_ouverte()) {
		return;
	}

	const auto taille_fichier = m_lectrice_relations.taille();
	const auto nombre_entrees = taille_fichier / TAILLE_REGISTRE_RELATION;

	std::fprintf(stderr, "Lecture du fichier %s...\n", m_infos_fichiers.chemin_fichier_relation.c_str());
	std::fprintf(stderr, "Taille fichier : %lu\n", taille_fichier);
	std::fprintf(stderr, "Il y a %lu entrées dans le fichier.\n", nombre_entrees);

	char tampon[TAILLE_REGISTRE_RELATION];

	for (size_t i = 0; i < nombre_entrees; ++i) {
		m_lectrice_relations.lis_tampon(tampon, TAILLE_REGISTRE_RELATION);

		auto utilise = tampon[0];
		auto id_noeud_debut = *reinterpret_cast<unsigned int *>(&tampon[1]);
		auto id_noeud_fin = *reinterpret_cast<unsigned int *>(&tampon[5]);
		auto type_relation = *reinterpret_cast<unsigned int *>(&tampon[9]);
		auto relation_prec_debut = *reinterpret_cast<unsigned int *>(&tampon[13]);
		auto relation_suiv_debut = *reinterpret_cast<unsigned int *>(&tampon[17]);
		auto relation_prec_fin = *reinterpret_cast<unsigned int *>(&tampon[21]);
		auto relation_suiv_fin = *reinterpret_cast<unsigned int *>(&tampon[25]);
		auto id_propriete = *reinterpret_cast<unsigned int *>(&tampon[29]);
		auto drapeaux = tampon[33];

		std::fprintf(stderr, "-------------------------------\n");
		std::fprintf(stderr, "Entrée relation\n");
		std::fprintf(stderr, "utilise             : %d\n", utilise);
		std::fprintf(stderr, "id_noeud_debut      : %u\n", id_noeud_debut);
		std::fprintf(stderr, "id_noeud_fin        : %u\n", id_noeud_fin);
		std::fprintf(stderr, "type_relation       : %u\n", type_relation);
		std::fprintf(stderr, "relation_prec_debut : %u\n", relation_prec_debut);
		std::fprintf(stderr, "relation_suiv_debut : %u\n", relation_suiv_debut);
		std::fprintf(stderr, "relation_prec_fin   : %u\n", relation_prec_fin);
		std::fprintf(stderr, "relation_suiv_fin   : %u\n", relation_suiv_fin);
		std::fprintf(stderr, "id_propriete        : %u\n", id_propriete);
		std::fprintf(stderr, "drapeaux            : %d\n", drapeaux);
	}

	m_lectrice_relations.ferme();
}

void graphe::ecris_types_relations()
{
	ecris_magasin_chaine(m_type_relations, m_infos_fichiers.chemin_fichier_type_relation);
}

void graphe::lis_types_relations()
{
	lis_magasin_chaine(m_infos_fichiers.chemin_fichier_type_relation);
}

void graphe::ecris_etiquettes()
{
}

void graphe::lis_etiquettes()
{
}

void graphe::ecris_noms_etiquettes()
{
	ecris_magasin_chaine(m_nom_etiquettes, m_infos_fichiers.chemin_fichier_nom_etiquette);
}

void graphe::lis_noms_etiquettes()
{
	lis_magasin_chaine(m_infos_fichiers.chemin_fichier_nom_etiquette);
}

unsigned int graphe::id_etiquette(const std::string &etiq) const
{
	return m_nom_etiquettes.index_chaine(etiq);
}

std::list<noeud *> graphe::noeuds() const
{
	return m_noeuds;
}

}  /* namespace arachne */

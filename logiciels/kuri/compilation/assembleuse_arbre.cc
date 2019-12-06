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

#include "assembleuse_arbre.h"

#include "contexte_generation_code.h"
#include "coulisse_c.hh"

#ifdef AVEC_LLVM
#include "coulisse_llvm.hh"
#endif

#include "modules.hh"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
{
	contexte.assembleuse = this;
	this->empile_noeud(type_noeud::RACINE, contexte, {});
}

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		memoire::deloge("noeud_base", noeud);
	}
}

noeud::base *assembleuse_arbre::empile_noeud(type_noeud type, ContexteGenerationCode &contexte, DonneesMorceau const &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, contexte, morceau);

	if (!m_pile.est_vide() && ajoute) {
		this->ajoute_noeud(noeud);
	}

	m_pile.empile(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(noeud::base *noeud)
{
	m_pile.haut()->ajoute_noeud(noeud);
}

noeud::base *assembleuse_arbre::cree_noeud(
		type_noeud type,
		ContexteGenerationCode &contexte,
		DonneesMorceau const &morceau)
{
	auto noeud = memoire::loge<noeud::base>("noeud_base", contexte, morceau);
	m_memoire_utilisee += sizeof(noeud::base);

	/* À FAIRE : réutilise la mémoire des noeuds libérés. */

	if (noeud != nullptr) {
		noeud->type = type;

		if (type == type_noeud::APPEL_FONCTION) {
			/* requis pour pouvoir renseigner le noms de arguments depuis
			 * l'analyse. */
			noeud->valeur_calculee = dls::liste<dls::vue_chaine_compacte>{};

			/* requis pour déterminer le module dans le noeud d'accès point
			 * À FAIRE : trouver mieux pour accéder à cette information */
			noeud->module_appel = noeud->morceau.module;
		}

		m_noeuds.pousse(noeud);
	}

	return noeud;
}

void assembleuse_arbre::depile_noeud(type_noeud type)
{
	assert(m_pile.haut()->type == type);
	m_pile.depile();
	static_cast<void>(type);
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	m_pile.haut()->imprime_code(os, 0);
	os << "------------------------------------------------------------------\n";
}

#ifdef AVEC_LLVM
void assembleuse_arbre::genere_code_llvm(ContexteGenerationCode &contexte_generation)
{
	if (m_pile.est_vide()) {
		return;
	}

	noeud::genere_code_llvm(m_pile.haut(), contexte_generation, false);
}
#endif

void assembleuse_arbre::genere_code_C(
		ContexteGenerationCode &contexte_generation,
		std::ostream &os,
		dls::chaine const &racine_kuri)
{
	if (m_pile.est_vide()) {
		return;
	}

	/* Définition de errno, puisque requis dans la plupart des erreurs des
	 * bibliothèques C. */
	auto donnees_var = DonneesVariable{};
	donnees_var.est_dynamique = true;
	donnees_var.index_type = contexte_generation.magasin_types[TYPE_Z32];

	contexte_generation.pousse_globale("errno", donnees_var);

	/* Pour fprintf dans les messages d'erreurs, nous incluons toujours "stdio.h". */
	os << "#include <stdio.h>\n";
	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	os << "#include <stdlib.h>\n";
	/* Pour strlen, nous incluons toujours "string.h". */
	os << "#include <string.h>\n";

	for (auto const &inc : this->inclusions) {
		os << "#include <" << inc << ">\n";
	}

	os << "\n";

	os << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";
	os << "static long __VG_memoire_utilisee__ = 0;";
	os << "static long ";
	auto &df = contexte_generation.module(0)->donnees_fonction("mémoire_utilisée").front();
	os << df.nom_broye;
	os << "() { return __VG_memoire_utilisee__; }";

	auto depassement_limites =
R"(
void KR__depassement_limites(
	const char *fichier,
	long ligne,
	const char *type,
	long taille,
	long index)
{
	fprintf(stderr, "%s:%ld\n", fichier, ligne);
	fprintf(stderr, "Dépassement des limites %s !\n", type);
	fprintf(stderr, "La taille est de %ld mais l'index est de %ld !\n", taille, index);
	abort();
}
)";

	os << depassement_limites;

	auto &magasin = contexte_generation.magasin_types;

	auto ds_contexte_global = DonneesStructure();
	ds_contexte_global.est_enum = false;
	ds_contexte_global.noeud_decl = nullptr;

	auto dm = DonneesMembre();
	dm.index_membre = 0;
	ds_contexte_global.donnees_membres.insere({ "compteur", dm });

	ds_contexte_global.index_types.pousse(magasin[TYPE_Z32]);

	contexte_generation.ajoute_donnees_structure("__contexte_global", ds_contexte_global);
	//contexte_generation.index_type_ctx = ds_contexte_global.index_type;

	auto dt = DonneesTypeFinal{};
	dt.pousse(id_morceau::POINTEUR);
	dt.pousse(id_morceau::CHAINE_CARACTERE | static_cast<int>(ds_contexte_global.id << 8));
	contexte_generation.index_type_ctx = magasin.ajoute_type(dt);

	/* NOTE : les initialiseurs des infos types doivent être valides pour toute
	 * la durée du programme, donc nous les mettons dans la fonction principale.
	 */
	dls::flux_chaine ss_infos_types;
	dls::flux_chaine fc_code;

	noeud::genere_code_C(m_pile.haut(), contexte_generation, fc_code, ss_infos_types);

	auto debut_main =
R"(
int main(int argc, char **argv)
{
	Tableau_char_ptr_ tabl_args;
	tabl_args.pointeur = argv;
	tabl_args.taille = argc;

	__contexte_global ctx;
	ctx.compteur = 0;
)";

	auto fin_main =
R"(
	return principale(&ctx, tabl_args);
}
)";

	os << fc_code.chn();
	os << debut_main;
	os << ss_infos_types.chn();
	os << fin_main;
}

void assembleuse_arbre::supprime_noeud(noeud::base *noeud)
{
	this->noeuds_libres.pousse(noeud);
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	return m_memoire_utilisee + nombre_noeuds() * sizeof(noeud::base *);
}

size_t assembleuse_arbre::nombre_noeuds() const
{
	return static_cast<size_t>(m_noeuds.taille());
}

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "noeud::base              : " << sizeof(noeud::base) << '\n';
	os << "DonneesTypeFinal         : " << sizeof(DonneesTypeFinal) << '\n';
	os << "DonneesMorceau          : " << sizeof(DonneesMorceau) << '\n';
	os << "dls::liste<noeud::base *> : " << sizeof(dls::liste<noeud::base *>) << '\n';
	os << "std::any                 : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}

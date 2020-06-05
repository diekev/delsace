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

#include "constructrice_ri.hh"

#include <fstream>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/numerique.hh"

#include "arbre_syntactic.h"
#include "compilatrice.hh"
#include "erreur.h"
#include "outils_lexemes.hh"
#include "optimisations.hh"

/* À FAIRE : (représentation intermédiaire, non-urgent)
 * - copie les tableaux fixes quand nous les assignations (a = b -> copie_mem(a, b))
 * - crée une branche lors de l'insertion d'un label si la dernière instruction
 *   n'en est pas une (pour mimiquer le comportement des blocs de LLVM)
 */

/* ************************************************************************** */

static auto trouve_index_membre(TypeCompose *type_compose, dls::vue_chaine_compacte const &nom_membre)
{
	auto idx_membre = 0u;

	POUR (type_compose->membres) {
		if (it.nom == nom_membre) {
			break;
		}

		idx_membre += 1;
	}

	return idx_membre;
}

static auto trouve_index_membre(Type *type, dls::vue_chaine_compacte const &nom_membre)
{
	auto type_compose = static_cast<TypeCompose *>(type);
	return trouve_index_membre(type_compose, nom_membre);
}

static auto trouve_index_membre(NoeudStruct *noeud_struct, dls::vue_chaine_compacte const &nom_membre)
{
	auto type_compose = static_cast<TypeCompose *>(noeud_struct->type);
	return trouve_index_membre(type_compose, nom_membre);
}

static auto est_expression_logique(NoeudExpression *noeud)
{
	if (noeud->genre == GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE) {
		return true;
	}

	if (noeud->lexeme->genre == GenreLexeme::BARRE_BARRE || noeud->lexeme->genre == GenreLexeme::ESP_ESP) {
		return true;
	}

	if (noeud->genre == GenreNoeud::EXPRESSION_PARENTHESE) {
		auto expr_paren = static_cast<NoeudExpressionUnaire *>(noeud);
		return est_expression_logique(expr_paren->expr);
	}

	return false;
}

/* ************************************************************************** */

#define IDENT_CODE(x) m_compilatrice.table_identifiants.identifiant_pour_chaine((x))

ConstructriceRI::ConstructriceRI(Compilatrice &compilatrice)
	: m_compilatrice(compilatrice)
{
	ident_contexte = IDENT_CODE("contexte");
}

ConstructriceRI::~ConstructriceRI()
{
#define DELOGE_ATOMES(Type, Tableau) \
	for (auto ptr : Tableau) {\
	memoire::deloge(#Type, ptr); \
}

	DELOGE_ATOMES(AtomeFonction, atomes_fonction);
	DELOGE_ATOMES(AtomeValeurConstante, atomes_constante);
	DELOGE_ATOMES(AtomeGlobale, atomes_globale);
	DELOGE_ATOMES(InstructionAllocation, insts_allocation);
	DELOGE_ATOMES(InstructionAppel, insts_appel);
	DELOGE_ATOMES(InstructionBranche, insts_branche);
	DELOGE_ATOMES(InstructionBrancheCondition, insts_branche_condition);
	DELOGE_ATOMES(InstructionChargeMem, insts_charge_memoire);
	DELOGE_ATOMES(InstructionLabel, insts_label);
	DELOGE_ATOMES(InstructionOpBinaire, insts_opbinaire);
	DELOGE_ATOMES(InstructionOpUnaire, insts_opunaire);
	DELOGE_ATOMES(InstructionStockeMem, insts_stocke_memoire);
	DELOGE_ATOMES(InstructionRetour, insts_retour);
	DELOGE_ATOMES(InstructionAccedeIndex, insts_accede_index);
	DELOGE_ATOMES(InstructionAccedeMembre, insts_accede_membre);
	DELOGE_ATOMES(InstructionTranstype, insts_transtype);
	DELOGE_ATOMES(TranstypeConstant, transtypes_constants);
	DELOGE_ATOMES(OpBinaireConstant, op_binaires_constants);
	DELOGE_ATOMES(OpUnaireConstant, op_unaires_constants);
	DELOGE_ATOMES(AccedeIndexConstant, accede_index_constants);

#undef DELOGE_ATOMES
}

static void ajoute_dependances_implicites(
		Compilatrice &compilatrice,
		NoeudDependance *noeud_fonction_principale,
		bool pour_meta_programme)
{
	auto &graphe_dependance = compilatrice.graphe_dependance;

	if (pour_meta_programme) {
		auto fonc_init = cherche_fonction_dans_module(compilatrice, "Kuri", "initialise_RC");
		auto noeud_init = graphe_dependance.cree_noeud_fonction(fonc_init);
		graphe_dependance.connecte_fonction_fonction(*noeud_fonction_principale, *noeud_init);
	}
}

void ConstructriceRI::genere_ri()
{
	auto &graphe_dependance = m_compilatrice.graphe_dependance;
	auto noeud_fonction_principale = graphe_dependance.cherche_noeud_fonction("principale");

	if (noeud_fonction_principale == nullptr) {
		erreur::fonction_principale_manquante();
	}

	ajoute_dependances_implicites(m_compilatrice, noeud_fonction_principale, false);

	reduction_transitive(graphe_dependance);

	auto debut_generation = dls::chrono::compte_seconde();

#ifdef DEBOGUE_PROGRESSION_RI
	m_noeuds_a_traiter = 0;

	traverse_graphe(noeud_fonction_principale, [this](NoeudDependance *)
	{
		m_noeuds_a_traiter += 1;
	});

	POUR (graphe_dependance.noeuds) {
		it->fut_visite = false;
	}

	m_noeuds_traites = 0;
#endif

	// table type
	auto index_type = 0u;
	m_compilatrice.typeuse.type_type_de_donnees_->index_dans_table_types = index_type++;
	m_compilatrice.typeuse.type_chaine->index_dans_table_types = index_type++;
	m_compilatrice.typeuse.type_eini->index_dans_table_types = index_type++;
	POUR (m_compilatrice.typeuse.types_simples) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_pointeurs) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_references) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_structures) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_enums) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_tableaux_fixes) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_tableaux_dynamiques) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_fonctions) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_variadiques) { it->index_dans_table_types = index_type++; }
	POUR (m_compilatrice.typeuse.types_unions) { it->index_dans_table_types = index_type++; }

	traverse_graphe(noeud_fonction_principale, [this](NoeudDependance *racine)
	{
#ifdef DEBOGUE_PROGRESSION_RI
		m_noeuds_traites += 1;
		std::cerr << "-----------------------------------------\n";
		std::cerr << "Génére code pour le noeud " << m_noeuds_traites << " / " << m_noeuds_a_traiter << '\n';
#endif

		if (racine->type == TypeNoeudDependance::TYPE) {
			if (racine->noeud_syntactique != nullptr) {
				genere_ri_pour_noeud(racine->noeud_syntactique);
			}
		}
		else {
			//imprime_arbre(racine->noeud_syntactique, std::cerr, 0);
			genere_ri_pour_noeud(racine->noeud_syntactique);
		}
	});

	genere_ri_pour_fonction_main();

	temps_generation = debut_generation.temps();
}

void ConstructriceRI::imprime_programme() const
{
	std::ofstream os;
	os.open("/tmp/ri_programme.kr");

	POUR (fonctions) {
		switch (it->genre_atome) {
			case Atome::Genre::CONSTANTE:
			{
				break;
			}
			case Atome::Genre::FONCTION:
			{
				auto atome_fonc = static_cast<AtomeFonction const *>(it);
				os << "fonction " << atome_fonc->nom;

				auto virgule = "(";

				for (auto param : atome_fonc->params_entrees) {
					os << virgule;
					os << param->ident->nom << ' ';

					auto type_pointeur = static_cast<TypePointeur *>(param->type);
					os << chaine_type(type_pointeur->type_pointe);

					virgule = ", ";
				}

				if (atome_fonc->params_entrees.taille == 0) {
					os << virgule;
				}

				os << ") -> ";
				os << chaine_type(atome_fonc->type);
				os << '\n';

				for (auto atome : atome_fonc->instructions) {
					auto inst = static_cast<Instruction const *>(atome);
					auto nombre_zero_avant_numero = dls::num::nombre_de_chiffres(atome_fonc->instructions.taille) - dls::num::nombre_de_chiffres(inst->numero);

					for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
						os << ' ';
					}

					os << "%" << inst->numero << ' ';

					imprime_instruction(inst, os);
				}

				break;
			}
			case Atome::Genre::INSTRUCTION:
			{
				auto inst = static_cast<Instruction const *>(it);
				imprime_instruction(inst, os);
				break;
			}
			case Atome::Genre::GLOBALE:
			{
				break;
			}
		}

		os << '\n';
	}
}

static void imprime_atome(Atome const *atome, std::ostream &os)
{
	if (atome->genre_atome == Atome::Genre::CONSTANTE) {
		auto atome_const = static_cast<AtomeConstante const *>(atome);

		switch (atome_const->genre) {
			case AtomeConstante::Genre::GLOBALE:
			{
				break;
			}
			case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
			{
				auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
				os << "  transtype ";
				imprime_atome(transtype_const->valeur, os);
				os << " vers " << chaine_type(transtype_const->type) << '\n';
				break;
			}
			case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
			{
				break;
			}
			case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
			{
				break;
			}
			case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
			{
				break;
			}
			case AtomeConstante::Genre::VALEUR:
			{
				auto valeur_constante = static_cast<AtomeValeurConstante const *>(atome);

				switch (valeur_constante->valeur.genre) {
					case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
					{
						os << valeur_constante->valeur.valeur_booleenne;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::ENTIERE:
					{
						os << valeur_constante->valeur.valeur_entiere;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::REELLE:
					{
						os << valeur_constante->valeur.valeur_reelle;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::NULLE:
					{
						os << "nul";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::CARACTERE:
					{
						os << valeur_constante->valeur.valeur_reelle;
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
					{
						os << "indéfinie";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
					{
						os << "À FAIRE(ri) : structure";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
					{
						os << "À FAIRE(ri) : tableau fixe";
						break;
					}
					case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
					{
						os << "À FAIRE(ri) : tableau données constantes";
						break;
					}
				}
			}
		}
	}
	else if (atome->genre_atome == Atome::Genre::FONCTION) {
		auto atome_fonction = static_cast<AtomeFonction const *>(atome);
		os << atome_fonction->nom;
	}
	else {
		auto inst_valeur = static_cast<Instruction const *>(atome);
		os << "%" << inst_valeur->numero;
	}
}

void ConstructriceRI::imprime_instruction(Instruction const *inst, std::ostream &os) const
{
	switch (inst->genre) {
		case Instruction::Genre::INVALIDE:
		{
			os << "  invalide\n";
			break;
		}
		case Instruction::Genre::ALLOCATION:
		{
			auto type_pointeur = static_cast<TypePointeur *>(inst->type);
			os << "  alloue " << chaine_type(type_pointeur->type_pointe) << ' ';

			if (inst->ident != nullptr) {
				os << inst->ident->nom << '\n';
			}
			else {
				os << "val" << inst->numero << '\n';
			}

			break;
		}
		case Instruction::Genre::APPEL:
		{
			auto inst_appel = static_cast<InstructionAppel const *>(inst);
			os << "  appel " << chaine_type(inst_appel->type) << ' ';
			imprime_atome(inst_appel->appele, os);

			auto virgule = "(";

			POUR (inst_appel->args) {
				os << virgule;
				imprime_atome(it, os);
				virgule = ", ";
			}

			if (inst_appel->args.taille == 0) {
				os << virgule;
			}

			os << ")\n";

			break;
		}
		case Instruction::Genre::BRANCHE:
		{
			auto inst_branche = static_cast<InstructionBranche const *>(inst);
			os << "  branche %" << inst_branche->label->numero << "\n";
			break;
		}
		case Instruction::Genre::BRANCHE_CONDITION:
		{
			auto inst_branche = static_cast<InstructionBrancheCondition const *>(inst);
			os << "  si ";
			imprime_atome(inst_branche->condition, os);
			os << " alors %" << inst_branche->label_si_vrai->numero
			   << " sinon %" << inst_branche->label_si_faux->numero
			   << '\n';
			break;
		}
		case Instruction::Genre::CHARGE_MEMOIRE:
		{
			auto inst_charge = static_cast<InstructionChargeMem const *>(inst);
			auto charge = inst_charge->chargee;

			os << "  charge " << chaine_type(inst->type);

			if (charge->genre_atome == Atome::Genre::GLOBALE) {
				os << " @globale" << charge;
			}
			else {
				auto inst_chargee = static_cast<Instruction const *>(charge);
				os << " %" << inst_chargee->numero;
			}

			os << '\n';
			break;
		}
		case Instruction::Genre::STOCKE_MEMOIRE:
		{
			auto inst_stocke = static_cast<InstructionStockeMem const *>(inst);
			auto ou = inst_stocke->ou;

			os << "  stocke " << chaine_type(ou->type);

			if (ou->genre_atome == Atome::Genre::GLOBALE) {
				os << " @globale" << ou;
			}
			else {
				auto inst_chargee = static_cast<Instruction const *>(ou);
				os << " %" << inst_chargee->numero;
			}

			os << ", " << chaine_type(inst_stocke->valeur->type) << ' ';
			imprime_atome(inst_stocke->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::LABEL:
		{
			auto inst_label = static_cast<InstructionLabel const *>(inst);
			os << "label " << inst_label->id << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_UNAIRE:
		{
			auto inst_un = static_cast<InstructionOpUnaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_un->op)
			   << ' ' << chaine_type(inst_un->type);
			imprime_atome(inst_un->valeur, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::OPERATION_BINAIRE:
		{
			auto inst_bin = static_cast<InstructionOpBinaire const *>(inst);
			os << "  " << chaine_pour_genre_op(inst_bin->op)
			   << ' ' << chaine_type(inst_bin->type) << ' ';
			imprime_atome(inst_bin->valeur_gauche, os);
			os << ", ";
			imprime_atome(inst_bin->valeur_droite, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::RETOUR:
		{
			auto inst_retour = static_cast<InstructionRetour const *>(inst);
			os << "  retourne ";
			if (inst_retour->valeur != nullptr) {
				auto atome = inst_retour->valeur;
				os << chaine_type(atome->type);
				os << ' ';

				imprime_atome(atome, os);
			}
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_INDEX:
		{
			auto inst_acces = static_cast<InstructionAccedeIndex const *>(inst);
			os << "  index " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << ", ";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::ACCEDE_MEMBRE:
		{
			auto inst_acces = static_cast<InstructionAccedeMembre const *>(inst);
			os << "  membre " << chaine_type(inst_acces->type) << ' ';
			imprime_atome(inst_acces->accede, os);
			os << ", ";
			imprime_atome(inst_acces->index, os);
			os << '\n';
			break;
		}
		case Instruction::Genre::TRANSTYPE:
		{
			auto inst_transtype = static_cast<InstructionTranstype const *>(inst);
			os << "  transtype ";
			imprime_atome(inst_transtype->valeur, os);
			os << " vers " << chaine_type(inst_transtype->type) << '\n';
			break;
		}
	}
}

size_t ConstructriceRI::memoire_utilisee() const
{
	auto memoire = 0ul;

#define COMPTE_MEMOIRE(Type, Tableau) \
	memoire += static_cast<size_t>(Tableau.taille) * (sizeof(Type *) + sizeof(Type))

	COMPTE_MEMOIRE(AtomeFonction, atomes_fonction);
	COMPTE_MEMOIRE(AtomeValeurConstante, atomes_constante);
	COMPTE_MEMOIRE(AtomeGlobale, atomes_globale);
	COMPTE_MEMOIRE(InstructionAllocation, insts_allocation);
	COMPTE_MEMOIRE(InstructionAppel, insts_appel);
	COMPTE_MEMOIRE(InstructionBranche, insts_branche);
	COMPTE_MEMOIRE(InstructionBrancheCondition, insts_branche_condition);
	COMPTE_MEMOIRE(InstructionChargeMem, insts_charge_memoire);
	COMPTE_MEMOIRE(InstructionLabel, insts_label);
	COMPTE_MEMOIRE(InstructionOpBinaire, insts_opbinaire);
	COMPTE_MEMOIRE(InstructionOpUnaire, insts_opunaire);
	COMPTE_MEMOIRE(InstructionStockeMem, insts_stocke_memoire);
	COMPTE_MEMOIRE(InstructionRetour, insts_retour);
	COMPTE_MEMOIRE(InstructionAccedeIndex, insts_accede_index);
	COMPTE_MEMOIRE(InstructionAccedeMembre, insts_accede_membre);
	COMPTE_MEMOIRE(InstructionTranstype, insts_transtype);
	COMPTE_MEMOIRE(TranstypeConstant, transtypes_constants);
	COMPTE_MEMOIRE(OpBinaireConstant, op_binaires_constants);
	COMPTE_MEMOIRE(OpUnaireConstant, op_unaires_constants);
	COMPTE_MEMOIRE(AccedeIndexConstant, accede_index_constants);

	POUR (atomes_fonction) {
		memoire += static_cast<size_t>(it->params_entrees.taille) * sizeof(Atome *);
		memoire += static_cast<size_t>(it->params_sorties.taille) * sizeof(Atome *);
	}

	POUR (insts_appel) {
		memoire += static_cast<size_t>(it->args.taille) * sizeof(Atome *);
	}

#undef COMPTE_MEMOIRE

#if 0
#define NOMBRE_INSTRUCTIONS(Tableau) \
	std::cerr << #Tableau << " : " << Tableau.taille << '\n';

	NOMBRE_INSTRUCTIONS(atomes_fonction);
	NOMBRE_INSTRUCTIONS(atomes_constante);
	NOMBRE_INSTRUCTIONS(atomes_globale);
	NOMBRE_INSTRUCTIONS(insts_allocation);
	NOMBRE_INSTRUCTIONS(insts_appel);
	NOMBRE_INSTRUCTIONS(insts_branche);
	NOMBRE_INSTRUCTIONS(insts_branche_condition);
	NOMBRE_INSTRUCTIONS(insts_charge_memoire);
	NOMBRE_INSTRUCTIONS(insts_label);
	NOMBRE_INSTRUCTIONS(insts_opbinaire);
	NOMBRE_INSTRUCTIONS(insts_opunaire);
	NOMBRE_INSTRUCTIONS(insts_stocke_memoire);
	NOMBRE_INSTRUCTIONS(insts_retour);
	NOMBRE_INSTRUCTIONS(insts_accede_index);
	NOMBRE_INSTRUCTIONS(insts_accede_membre);
	NOMBRE_INSTRUCTIONS(insts_transtype);
	NOMBRE_INSTRUCTIONS(transtypes_constants);
	NOMBRE_INSTRUCTIONS(op_binaires_constants);
	NOMBRE_INSTRUCTIONS(op_unaires_constants);
	NOMBRE_INSTRUCTIONS(accede_index_constants);

#undef NOMBRE_INSTRUCTIONS
#endif

	return memoire;
}

AtomeFonction *ConstructriceRI::cree_fonction(Lexeme const *lexeme, dls::chaine const &nom)
{
	auto atome = AtomeFonction::cree(lexeme, nom);
	atomes_fonction.pousse(atome);
	fonctions.pousse(atome);
	return atome;
}

AtomeFonction *ConstructriceRI::cree_fonction(Lexeme const *lexeme, dls::chaine const &nom, kuri::tableau<Atome *> &&params)
{
	auto atome = AtomeFonction::cree(lexeme, nom, std::move(params));
	atomes_fonction.pousse(atome);
	fonctions.pousse(atome);
	return atome;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *ConstructriceRI::trouve_ou_insere_fonction(NoeudDeclarationFonction const *decl)
{
	auto iter_fonc = table_fonctions.trouve(decl->nom_broye);

	if (iter_fonc != table_fonctions.fin()) {
		return iter_fonc->second;
	}

	auto params = kuri::tableau<Atome *>();
	params.reserve(decl->params.taille);

	if (!decl->est_externe && !dls::outils::possede_drapeau(decl->drapeaux, FORCE_NULCTX)) {
		auto atome = cree_allocation(m_compilatrice.type_contexte, ident_contexte);
		params.pousse(atome);
	}

	POUR (decl->params) {
		auto atome = cree_allocation(it->type, it->ident);
		params.pousse(atome);
	}

	// À FAIRE : retours multiples

	auto atome_fonc = cree_fonction(decl->lexeme, decl->nom_broye, std::move(params));
	atome_fonc->type = decl->type;

	if (dls::outils::possede_drapeau(decl->drapeaux, FORCE_SANSTRACE)) {
		atome_fonc->sanstrace = true;
	}

	table_fonctions.insere({ decl->nom_broye, atome_fonc });

	return atome_fonc;
}

AtomeConstante *ConstructriceRI::cree_constante_entiere(Type *type, unsigned long long valeur)
{
	auto atome = AtomeValeurConstante::cree(type, valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_z32(unsigned long long valeur)
{
	return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z32], valeur);
}

AtomeConstante *ConstructriceRI::cree_z64(unsigned long long valeur)
{
	return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z64], valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_reelle(Type *type, double valeur)
{
	auto atome = AtomeValeurConstante::cree(type, valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_structure(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto atome = AtomeValeurConstante::cree(type, std::move(valeurs));
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_fixe(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto atome = AtomeValeurConstante::cree_tableau_fixe(type, std::move(valeurs));
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(Type *type, kuri::tableau<char> &&donnees_constantes)
{
	auto atome = AtomeValeurConstante::cree(type, std::move(donnees_constantes));
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(Type *type, char *pointeur, long taille)
{
	auto atome = AtomeValeurConstante::cree(type, pointeur, taille);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeGlobale *ConstructriceRI::cree_globale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante)
{
	auto atome = AtomeGlobale::cree(m_compilatrice.typeuse.type_pointeur_pour(type), initialisateur, est_externe, est_constante);
	atomes_globale.pousse(atome);
	globales.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_tableau_global(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto taille_tableau = valeurs.taille;
	auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(type, taille_tableau);
	auto tableau_fixe = cree_constante_tableau_fixe(type_tableau, std::move(valeurs));

	return cree_tableau_global(tableau_fixe);
}

AtomeConstante *ConstructriceRI::cree_tableau_global(AtomeConstante *tableau_fixe)
{
	auto type_tableau_fixe = static_cast<TypeTableauFixe *>(tableau_fixe->type);
	auto globale_tableau_fixe = cree_globale(type_tableau_fixe, tableau_fixe, false, true);
	auto ptr_premier_element = cree_acces_index_constant(globale_tableau_fixe, cree_z64(0));
	auto valeur_taille = cree_z64(static_cast<unsigned long>(type_tableau_fixe->taille));
	auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);

	auto membres = kuri::tableau<AtomeConstante *>(3);
	membres[0] = ptr_premier_element;
	membres[1] = valeur_taille;
	membres[2] = valeur_taille;

	return cree_constante_structure(type_tableau_dyn, std::move(membres));
}

AtomeConstante *ConstructriceRI::cree_constante_booleenne(bool valeur)
{
	auto atome = AtomeValeurConstante::cree(m_compilatrice.typeuse[TypeBase::BOOL], valeur);
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_caractere(Type *type, unsigned long long valeur)
{
	auto atome = AtomeValeurConstante::cree(type, valeur);
	atome->valeur.genre = AtomeValeurConstante::Valeur::Genre::CARACTERE;
	atomes_constante.pousse(atome);
	return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_nulle(Type *type)
{
	auto atome = AtomeValeurConstante::cree(type);
	atomes_constante.pousse(atome);
	return atome;
}

InstructionBranche *ConstructriceRI::cree_branche(InstructionLabel *label)
{
	auto inst = InstructionBranche::cree(label);
	inst->numero = nombre_instructions++;
	insts_branche.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionBrancheCondition *ConstructriceRI::cree_branche_condition(Atome *valeur, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
{
	auto inst = InstructionBrancheCondition::cree(valeur, label_si_vrai, label_si_faux);
	inst->numero = nombre_instructions++;
	insts_branche_condition.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionLabel *ConstructriceRI::cree_label()
{
	auto inst = InstructionLabel::cree(nombre_labels++);
	inst->numero = nombre_instructions++;
	insts_label.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionLabel *ConstructriceRI::reserve_label()
{
	auto inst = InstructionLabel::cree(nombre_labels++);
	insts_label.pousse(inst);
	return inst;
}

void ConstructriceRI::insere_label(InstructionLabel *label)
{
	label->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(label);
}

InstructionRetour *ConstructriceRI::cree_retour(Atome *valeur)
{
	auto inst = InstructionRetour::cree(valeur);
	inst->numero = nombre_instructions++;
	insts_retour.pousse(inst);
	fonction_courante->instructions.pousse(inst);
	return inst;
}

InstructionAllocation *ConstructriceRI::cree_allocation(Type *type, IdentifiantCode *ident)
{
	/* le résultat d'une instruction d'allocation est l'adresse de la variable. */
	auto type_pointeur = m_compilatrice.typeuse.type_pointeur_pour(type);
	auto inst = InstructionAllocation::cree(type_pointeur, ident);
	inst->numero = nombre_instructions++;

	/* Nous utilisons pour l'instant cree_allocation pour les paramètres des
	 * fonctions, et la fonction_courante est nulle lors de cette opération.
	 */
	if (fonction_courante) {
		fonction_courante->instructions.pousse(inst);
	}

	insts_allocation.pousse(inst);

	return inst;
}

InstructionStockeMem *ConstructriceRI::cree_stocke_mem(Atome *ou, Atome *valeur)
{
	assert(ou->type->genre == GenreType::POINTEUR);
	auto type_pointeur = static_cast<TypePointeur *>(ou->type);
//	std::cerr << __func__ << ", type_pointeur->type_pointe : " << chaine_type(type_pointeur->type_pointe)
//			  << ", valeur->type : " << chaine_type(valeur->type) << '\n';
	assert(type_pointeur->type_pointe == valeur->type || (type_pointeur->type_pointe->genre == GenreType::TYPE_DE_DONNEES && type_pointeur->type_pointe->genre == valeur->type->genre));

	auto inst = InstructionStockeMem::cree(valeur->type, ou, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_stocke_memoire.pousse(inst);
	return inst;
}

InstructionChargeMem *ConstructriceRI::cree_charge_mem(Atome *ou)
{
	/* nous chargeons depuis une adresse en mémoire, donc nous devons avoir un pointeur */
	//std::cerr << __func__ << ", type atome : " << chaine_type(ou->type) << '\n';
	assert(ou->type->genre == GenreType::POINTEUR || ou->type->genre == GenreType::REFERENCE);
	auto type_pointeur = static_cast<TypePointeur *>(ou->type);

	POUR (charge_mems) {
		if (it->chargee == ou) {
			return it;
		}
	}

	assert(ou->genre_atome == Atome::Genre::INSTRUCTION || ou->genre_atome == Atome::Genre::GLOBALE);
	//std::cerr << __func__ << ", type instruction : " << static_cast<int>(inst_chargee->genre) << '\n';
	//assert(dls::outils::est_element(inst_chargee->genre, Instruction::Genre::ALLOCATION, Instruction::Genre::ACCEDE_MEMBRE, Instruction::Genre::ACCEDE_INDEX));

	auto inst = InstructionChargeMem::cree(type_pointeur->type_pointe, ou);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_charge_memoire.pousse(inst);
	charge_mems.pousse(inst);
	return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(Lexeme const *lexeme, Atome *appele)
{
	auto inst = InstructionAppel::cree(lexeme, appele);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_appel.pousse(inst);
	return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(Lexeme const *lexeme, Atome *appele, kuri::tableau<Atome *> &&args)
{
	auto inst = InstructionAppel::cree(lexeme, appele, std::move(args));	
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_appel.pousse(inst);
	return inst;
}

InstructionOpUnaire *ConstructriceRI::cree_op_unaire(Type *type, OperateurUnaire::Genre op, Atome *valeur)
{
	auto inst = InstructionOpUnaire::cree(type, op, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_opunaire.pousse(inst);
	return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_binaire(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite)
{
	auto inst = InstructionOpBinaire::cree(type, op, valeur_gauche, valeur_droite);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_opbinaire.pousse(inst);
	return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_comparaison(OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite)
{
	return cree_op_binaire(m_compilatrice.typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
}

InstructionAccedeIndex *ConstructriceRI::cree_acces_index(Atome *accede, Atome *index)
{
	auto type_pointe = static_cast<Type *>(nullptr);
	if (accede->genre_atome == Atome::Genre::CONSTANTE) {
		type_pointe = accede->type;
	}
	else {
		//std::cerr << __func__ << ", type accede : " << chaine_type(accede->type) << '\n';
		assert(accede->type->genre == GenreType::POINTEUR);
		auto type_pointeur = static_cast<TypePointeur *>(accede->type);
		type_pointe = type_pointeur->type_pointe;
	}

	//std::cerr << __func__ << ", accede->type : " << chaine_type(type_pointe) << '\n';
	assert(dls::outils::est_element(type_pointe->genre, GenreType::POINTEUR, GenreType::TABLEAU_FIXE));

	auto type = m_compilatrice.typeuse.type_pointeur_pour(type_dereference_pour(type_pointe));

	auto inst = InstructionAccedeIndex::cree(type, accede, index);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_accede_index.pousse(inst);
	return inst;
}

InstructionAccedeMembre *ConstructriceRI::cree_acces_membre(Atome *accede, long index)
{
	//std::cerr << __func__ << ", accede->type : " << chaine_type(accede->type) << '\n';
	assert(accede->type->genre == GenreType::POINTEUR || accede->type->genre == GenreType::REFERENCE);
	auto type_pointeur = static_cast<TypePointeur *>(accede->type);
	assert(est_type_compose(type_pointeur->type_pointe));

	POUR (acces_membres) {
		if (it->accede == accede && static_cast<AtomeValeurConstante *>(it->index)->valeur.valeur_entiere == static_cast<unsigned>(index)) {
			return it;
		}
	}

	auto type_compose = static_cast<TypeCompose *>(type_pointeur->type_pointe);
	auto type = static_cast<Type *>(nullptr);

	if (type_compose->genre == GenreType::UNION) {
		if (index < type_compose->membres.taille) {
			type = type_compose->membres[index].type;
		}
		else {
			type = m_compilatrice.typeuse[TypeBase::Z32];
		}
	}
	else {
		type = type_compose->membres[index].type;
	}

	/* nous retournons un pointeur vers le membre */
	type = m_compilatrice.typeuse.type_pointeur_pour(type);

	auto inst = InstructionAccedeMembre::cree(type, accede, cree_z64(static_cast<unsigned>(index)));
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_accede_membre.pousse(inst);
	acces_membres.pousse(inst);
	return inst;
}

Instruction *ConstructriceRI::cree_acces_membre_et_charge(Atome *accede, long index)
{
	auto inst = cree_acces_membre(accede, index);
	return cree_charge_mem(inst);
}

InstructionTranstype *ConstructriceRI::cree_transtype(Type *type, Atome *valeur)
{
	//std::cerr << __func__ << ", type : " << chaine_type(type) << ", valeur " << chaine_type(valeur->type) << '\n';
	auto inst = InstructionTranstype::cree(type, valeur);
	inst->numero = nombre_instructions++;
	fonction_courante->instructions.pousse(inst);
	insts_transtype.pousse(inst);
	return inst;
}

TranstypeConstant *ConstructriceRI::cree_transtype_constant(Type *type, AtomeConstante *valeur)
{
	auto inst = TranstypeConstant::cree(type, valeur);
	transtypes_constants.pousse(inst);
	return inst;
}

OpUnaireConstant *ConstructriceRI::cree_op_unaire_constant(Type *type, OperateurUnaire::Genre op, AtomeConstante *valeur)
{
	auto inst = OpUnaireConstant::cree(type, op, valeur);
	op_unaires_constants.pousse(inst);
	return inst;
}

OpBinaireConstant *ConstructriceRI::cree_op_binaire_constant(Type *type, OperateurBinaire::Genre op, AtomeConstante *valeur_gauche, AtomeConstante *valeur_droite)
{
	auto inst = OpBinaireConstant::cree(type, op, valeur_gauche, valeur_droite);
	op_binaires_constants.pousse(inst);
	return inst;
}

OpBinaireConstant *ConstructriceRI::cree_op_comparaison_constant(OperateurBinaire::Genre op, AtomeConstante *valeur_gauche, AtomeConstante *valeur_droite)
{
	return cree_op_binaire_constant(m_compilatrice.typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
}

AccedeIndexConstant *ConstructriceRI::cree_acces_index_constant(AtomeConstante *accede, AtomeConstante *index)
{
	//std::cerr << __func__ << ", type accede : " << chaine_type(accede->type) << '\n';
	assert(accede->type->genre == GenreType::POINTEUR);
	auto type_pointeur = static_cast<TypePointeur *>(accede->type);
	//std::cerr << __func__ << ", accede->type : " << chaine_type(type_pointeur->type_pointe) << '\n';
	assert(dls::outils::est_element(type_pointeur->type_pointe->genre, GenreType::POINTEUR, GenreType::TABLEAU_FIXE));

	auto type = m_compilatrice.typeuse.type_pointeur_pour(type_dereference_pour(type_pointeur->type_pointe));

	auto inst = AccedeIndexConstant::cree(type, accede, index);

	if (fonction_courante) {
		fonction_courante->instructions.pousse(inst);
	}

	accede_index_constants.pousse(inst);
	return inst;
}

void ConstructriceRI::empile_controle_boucle(IdentifiantCode *ident, InstructionLabel *label_continue, InstructionLabel *label_arrete)
{
	insts_continue_arrete.pousse({ ident, label_continue, label_arrete });
}

void ConstructriceRI::depile_controle_boucle()
{
	insts_continue_arrete.pop_back();
}

Atome *ConstructriceRI::genere_ri_pour_noeud(NoeudExpression *noeud)
{
	switch (noeud->genre) {
		case GenreNoeud::DECLARATION_ENUM:
		case GenreNoeud::DIRECTIVE_EXECUTION:
		case GenreNoeud::EXPRESSION_PLAGE:
		case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
		case GenreNoeud::INSTRUCTION_SINON:
		{
			return nullptr;
		}
		case GenreNoeud::DECLARATION_OPERATEUR:
		case GenreNoeud::DECLARATION_FONCTION:
		{
			auto decl = static_cast<NoeudDeclarationFonction *>(noeud);

			if (decl->est_declaration_type) {
				return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, decl->type->index_dans_table_types);
			}

			nombre_instructions = 0;
			nombre_labels = 0;

			auto atome_fonc = trouve_ou_insere_fonction(decl);

			if (decl->est_externe) {
				return atome_fonc;
			}

			fonction_courante = atome_fonc;
			table_locales.efface();
			acces_membres.taille = 0;
			charge_mems.taille = 0;

			POUR (atome_fonc->params_entrees) {
				table_locales.insere({ it->ident, it });
			}

			cree_label();

			genere_ri_pour_noeud(decl->bloc);

			if (decl->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
				cree_retour(nullptr);
			}

			corrige_labels(atome_fonc);

			fonction_courante = nullptr;

			return atome_fonc;
		}
		case GenreNoeud::DECLARATION_COROUTINE:
		{
			genere_ri_pour_coroutine(static_cast<NoeudDeclarationFonction *>(noeud));
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_COMPOSEE:
		{
			auto noeud_bloc = static_cast<NoeudBloc *>(noeud);

			if (noeud_bloc->est_differe) {
				noeud_bloc->bloc_parent->noeuds_differes.pousse(noeud_bloc);
				return nullptr;
			}

			POUR (noeud_bloc->expressions) {
				genere_ri_pour_noeud(it);
			}

			auto derniere_instruction = *(fonction_courante->instructions.end() - 1);

			if (derniere_instruction->genre_atome == Atome::Genre::INSTRUCTION && static_cast<Instruction *>(derniere_instruction)->genre != Instruction::Genre::RETOUR) {
				/* génère le code pour tous les noeuds différés de ce bloc */
				for (auto i = noeud_bloc->noeuds_differes.taille - 1; i >= 0; --i) {
					auto bloc_differe = noeud_bloc->noeuds_differes[i];
					bloc_differe->est_differe = false;
					genere_ri_pour_noeud(bloc_differe);
					bloc_differe->est_differe = true;
				}
			}

			return nullptr;
		}
		case GenreNoeud::EXPRESSION_APPEL_FONCTION:
		{
			auto expr_appel = static_cast<NoeudExpressionAppel *>(noeud);

			auto args = kuri::tableau<Atome *>();
			args.reserve(expr_appel->exprs.taille);

			if (!dls::outils::possede_drapeau(expr_appel->drapeaux, FORCE_NULCTX)) {
				args.pousse(cree_charge_mem(table_locales[ident_contexte]));
			}

			auto ancien_pour_appel = m_noeud_pour_appel;
			m_noeud_pour_appel = expr_appel;

			POUR (expr_appel->exprs) {
				if (est_expression_logique(it)) {
					auto bloc_si_vrai = reserve_label();
					auto bloc_si_faux = reserve_label();
					auto bloc_apres_faux = reserve_label();

					auto atome = cree_allocation(m_compilatrice.typeuse[TypeBase::BOOL], nullptr);
					genere_ri_pour_condition(it, bloc_si_vrai, bloc_si_faux);
					insere_label(bloc_si_vrai);
					cree_stocke_mem(atome, cree_constante_booleenne(true));
					cree_branche(bloc_apres_faux);
					insere_label(bloc_si_faux);
					cree_stocke_mem(atome, cree_constante_booleenne(false));
					insere_label(bloc_apres_faux);

					args.pousse(cree_charge_mem(atome));
				}
				else {
					auto atome = genere_ri_transformee_pour_noeud(it, nullptr);
					args.pousse(atome);
				}
			}

			m_noeud_pour_appel = ancien_pour_appel;

			auto atome_fonc = static_cast<Atome *>(nullptr);

			if (expr_appel->aide_generation_code == APPEL_POINTEUR_FONCTION) {
				atome_fonc = genere_ri_pour_expression_droite(expr_appel->appelee);
			}
			else if (expr_appel->appelee->genre == GenreNoeud::EXPRESSION_INIT_DE) {
				atome_fonc = genere_ri_pour_noeud(expr_appel->appelee);
			}
			else {
				auto decl = static_cast<NoeudDeclarationFonction const *>(expr_appel->noeud_fonction_appelee);
				atome_fonc = trouve_ou_insere_fonction(decl);
			}

			return cree_appel(noeud->lexeme, atome_fonc, std::move(args));
		}
		case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
		{
			auto expr_ref = static_cast<NoeudExpressionReference *>(noeud);
			auto decl_ref = expr_ref->decl;

			// À FAIRE : decl peut être nulle pour les appels de pointeurs de fonctions

			if (decl_ref != nullptr && decl_ref->drapeaux & EST_CONSTANTE) {
				auto decl_const = static_cast<NoeudDeclarationVariable *>(decl_ref);
				return cree_constante_entiere(decl_ref->type, static_cast<unsigned long>(decl_const->valeur_expression.entier));
			}

			if (decl_ref != nullptr && decl_ref->genre == GenreNoeud::DECLARATION_FONCTION) {
				return trouve_ou_insere_fonction(static_cast<NoeudDeclarationFonction *>(decl_ref));
			}

			auto iter_globale = table_globales.trouve(noeud->ident);

			if (iter_globale != table_globales.fin()) {
				return iter_globale->second;
			}

			return table_locales[noeud->ident];
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
		{
			auto noeud_bin = static_cast<NoeudExpressionMembre *>(noeud);
			return genere_ri_pour_acces_membre(noeud_bin);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
		{
			auto noeud_bin = static_cast<NoeudExpressionMembre *>(noeud);
			return genere_ri_pour_acces_membre_union(noeud_bin);
		}
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(noeud->type);
			return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->type_connu->index_dans_table_types);
		}
		case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
		{
			auto expr_ass = static_cast<NoeudExpressionBinaire *>(noeud);
			auto expression = expr_ass->expr2;

			auto pointeur = genere_ri_pour_noeud(expr_ass->expr1);

			if (est_expression_logique(expression)) {
				auto label_si_vrai = reserve_label();
				auto label_si_faux = reserve_label();
				auto label_apres_faux = reserve_label();

				genere_ri_pour_condition(expression, label_si_vrai, label_si_faux);

				insere_label(label_si_vrai);
				cree_stocke_mem(pointeur, cree_constante_booleenne(true));
				cree_branche(label_apres_faux);
				insere_label(label_si_faux);
				cree_stocke_mem(pointeur, cree_constante_booleenne(false));
				insere_label(label_apres_faux);
			}
			else {
				genere_ri_transformee_pour_noeud(expr_ass->expr2, pointeur);
			}

			return nullptr;
		}
		case GenreNoeud::DECLARATION_VARIABLE:
		{
			auto decl = static_cast<NoeudDeclarationVariable *>(noeud);

			if ((decl->drapeaux & EST_CONSTANTE) != 0) {
				return nullptr;
			}

			if (fonction_courante == nullptr) {
				auto est_externe = dls::outils::possede_drapeau(decl->drapeaux, EST_EXTERNE);
				auto valeur = static_cast<AtomeConstante *>(nullptr);
				auto atome = cree_globale(noeud->type, valeur, est_externe, false);
				atome->ident = noeud->ident;

				if (decl->expression) {
					if (decl->expression->genre == GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU) {
						valeur = static_cast<AtomeConstante *>(genere_ri_transformee_pour_noeud(decl->expression, nullptr));
					}
					else {
						constructeurs_globaux.pousse({ atome, decl->expression });
					}
				}
				else if (!est_externe) {
					valeur = genere_initialisation_defaut_pour_type(noeud->type);
				}

				atome->initialisateur = valeur;

				table_globales.insere({ noeud->ident, atome });

				return atome;
			}

			auto pointeur = cree_allocation(noeud->type, noeud->ident);

			if (decl->expression) {
				if (est_expression_logique(decl->expression)) {
					auto label_si_vrai = reserve_label();
					auto label_si_faux = reserve_label();
					auto label_apres_faux = reserve_label();

					genere_ri_pour_condition(decl->expression, label_si_vrai, label_si_faux);

					insere_label(label_si_vrai);
					cree_stocke_mem(pointeur, cree_constante_booleenne(true));
					cree_branche(label_apres_faux);
					insere_label(label_si_faux);
					cree_stocke_mem(pointeur, cree_constante_booleenne(false));
					insere_label(label_apres_faux);
				}
				else if (decl->expression->genre != GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
					genere_ri_transformee_pour_noeud(decl->expression, pointeur);
				}
			}
			else {
				if (noeud->type->genre == GenreType::TABLEAU_FIXE) {
					// À FAIRE(ri) : valeur défaut pour tableau fixe
				}
				else if (noeud->type->genre == GenreType::STRUCTURE) {
					auto nom_fonction = "initialise_" + dls::vers_chaine(noeud->type);
					auto atome_fonction = table_fonctions[nom_fonction];

					auto params_init = kuri::tableau<Atome *>(2);
					params_init[0] = cree_charge_mem(table_locales[ident_contexte]);
					params_init[1] = pointeur;

					cree_appel(noeud->lexeme, atome_fonction, std::move(params_init));
				}
				else {
					auto valeur = genere_initialisation_defaut_pour_type(noeud->type);
					cree_stocke_mem(pointeur, valeur);
				}
			}

			table_locales[noeud->ident] = pointeur;

			return pointeur;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		{
			return cree_constante_reelle(noeud->type, noeud->lexeme->valeur_reelle);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		{
			return cree_constante_entiere(noeud->type, noeud->lexeme->valeur_entiere);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		{
			auto chaine = kuri::chaine();
			chaine.pointeur = noeud->lexeme->pointeur;
			chaine.taille = noeud->lexeme->taille;
			return cree_chaine(dls::vue_chaine_compacte(chaine.pointeur, chaine.taille));
		}
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		{
			return cree_constante_booleenne(noeud->lexeme->chaine == "vrai");
		}
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		{
			// À FAIRE : caractères Unicode
			auto caractere = static_cast<unsigned char>(noeud->lexeme->valeur_entiere);
			return cree_constante_caractere(noeud->type, caractere);
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		{
			return cree_constante_nulle(noeud->type);
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto expr_bin = static_cast<NoeudExpressionBinaire *>(noeud);

			if (expr_bin->type->genre == GenreType::TYPE_DE_DONNEES) {
				auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(expr_bin->type);

				if (type_de_donnees->type_connu) {
					return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->type_connu->index_dans_table_types);
				}

				return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->index_dans_table_types);
			}

			auto traduit_operation_binaire = [&](OperateurBinaire const *op, Atome *valeur_gauche, Atome *valeur_droite) -> Atome*
			{
				// À FAIRE(ri) : arithmétique de pointeur, opérateurs logiques
				if (op->est_basique) {
					return cree_op_binaire(noeud->type, op->genre, valeur_gauche, valeur_droite);
				}

				auto decl = op->decl;
				auto requiers_contexte = !decl->est_externe && !dls::outils::possede_drapeau(decl->drapeaux, FORCE_NULCTX);
				auto atome_fonction = trouve_ou_insere_fonction(decl);
				auto args = kuri::tableau<Atome *>(2 + requiers_contexte);

				if (requiers_contexte) {
					args[0] = cree_charge_mem(table_locales[ident_contexte]);
				}

				args[0 + requiers_contexte] = valeur_gauche;
				args[1 + requiers_contexte] = valeur_droite;

				return cree_appel(noeud->lexeme, atome_fonction, std::move(args));
			};

			if ((expr_bin->drapeaux & EST_ASSIGNATION_COMPOSEE) != 0) {
				auto pointeur = genere_ri_pour_noeud(expr_bin->expr1);
				auto valeur_gauche = cree_charge_mem(pointeur);
				auto valeur_droite = genere_ri_transformee_pour_noeud(expr_bin->expr2, nullptr);

				auto valeur = traduit_operation_binaire(expr_bin->op, valeur_gauche, valeur_droite);
				return cree_stocke_mem(pointeur, valeur);
			}

			auto valeur_gauche = genere_ri_transformee_pour_noeud(expr_bin->expr1, nullptr);
			auto valeur_droite = genere_ri_transformee_pour_noeud(expr_bin->expr2, nullptr);
			return traduit_operation_binaire(expr_bin->op, valeur_gauche, valeur_droite);
		}
		case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
		{
			// Ce noeud devrait être géré au niveau du parent, soit lors d'une
			// condition pour une boucle, soit lors d'une assignation booléenne.
			imprime_fichier_ligne(m_compilatrice, *noeud->lexeme);
			assert(false);
			return nullptr;
		}
		case GenreNoeud::EXPRESSION_INDEXAGE:
		{
			auto expr_bin = static_cast<NoeudExpressionBinaire *>(noeud);
			auto type_gauche = expr_bin->expr1->type;
			auto pointeur = genere_ri_pour_noeud(expr_bin->expr1);
			auto valeur = genere_ri_transformee_pour_noeud(expr_bin->expr2, nullptr);

			if (type_gauche->genre == GenreType::POINTEUR) {
				return cree_acces_index(pointeur, valeur);
			}

			/* À CONSIDÉRER :
			 * - directive pour ne pas générer le code de vérification,
			 *   car les branches nuisent à la vitesse d'exécution des
			 *   programmes
			 * - tests redondants ou inutiles, par exemple :
			 *    - ceci génère deux fois la même instruction
			 *      x[i] = 0;
			 *      y = x[i];
			 *    - ceci génère une instruction inutile
			 *	    dyn x : [6]z32;
			 *      x[0] = 8;
			 */

			auto genere_protection_limites = [this, noeud](Atome *acces_taille, Atome *valeur_, AtomeFonction *fonction)
			{
				auto label1 = reserve_label();
				auto label2 = reserve_label();
				auto label3 = reserve_label();
				auto label4 = reserve_label();

				auto condition = cree_op_comparaison(OperateurBinaire::Genre::Comp_Inf, valeur_, cree_z64(0));
				cree_branche_condition(condition, label1, label2);

				insere_label(label1);

				auto params = kuri::tableau<Atome *>(3);
				params[0] = cree_charge_mem(table_locales[ident_contexte]);
				params[1] = acces_taille;
				params[2] = valeur_;
				cree_appel(noeud->lexeme, fonction, std::move(params));

				insere_label(label2);

				condition = cree_op_comparaison(OperateurBinaire::Genre::Comp_Sup_Egal, valeur_, acces_taille);
				cree_branche_condition(condition, label3, label4);

				insere_label(label3);

				params = kuri::tableau<Atome *>(3);
				params[0] = cree_charge_mem(table_locales[ident_contexte]);
				params[1] = acces_taille;
				params[2] = valeur_;
				cree_appel(noeud->lexeme, fonction, std::move(params));

				insere_label(label4);
			};

			// À FAIRE : les fonctions sans contexte ne peuvent pas avoir des vérifications de limites

			if (type_gauche->genre == GenreType::TABLEAU_FIXE) {
				if (table_locales[ident_contexte] != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
					auto type_tableau_fixe = static_cast<TypeTableauFixe *>(type_gauche);
					auto acces_taille = cree_z64(static_cast<unsigned long>(type_tableau_fixe->taille));
					genere_protection_limites(acces_taille, valeur, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_tableau));
				}
				return cree_acces_index(pointeur, valeur);
			}

			if (type_gauche->genre == GenreType::TABLEAU_DYNAMIQUE || type_gauche->genre == GenreType::VARIADIQUE) {
				if (table_locales[ident_contexte] != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
					auto acces_taille = cree_acces_membre_et_charge(pointeur, 1);
					genere_protection_limites(acces_taille, valeur, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_tableau));
				}
				pointeur = cree_acces_membre(pointeur, 0);
				return cree_acces_index(pointeur, valeur);
			}

			if (type_gauche->genre == GenreType::CHAINE) {
				if (table_locales[ident_contexte] != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
					auto acces_taille = cree_acces_membre_et_charge(pointeur, 1);
					genere_protection_limites(acces_taille, valeur, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_chaine));
				}
				pointeur = cree_acces_membre(pointeur, 0);
				return cree_acces_index(pointeur, valeur);
			}

			return nullptr;
		}
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto expr_un = static_cast<NoeudExpressionUnaire *>(noeud);

			if (expr_un->type->genre == GenreType::TYPE_DE_DONNEES) {
				auto type_de_donnes = static_cast<TypeTypeDeDonnees *>(expr_un->type);
				return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnes->type_connu->index_dans_table_types);
			}

			if (noeud->lexeme->genre == GenreLexeme::AROBASE) {
				auto valeur = genere_ri_pour_noeud(expr_un->expr);
				if (expr_un->expr->type->genre == GenreType::REFERENCE) {
					valeur = cree_charge_mem(valeur);
				}
				return valeur;
			}

			if (noeud->lexeme->genre == GenreLexeme::EXCLAMATION) {
				auto condition = expr_un->expr;
				auto type_condition = condition->type;
				auto valeur = static_cast<Atome *>(nullptr);

				switch (type_condition->genre) {
					case GenreType::ENTIER_NATUREL:
					case GenreType::ENTIER_RELATIF:
					case GenreType::ENTIER_CONSTANT:
					{
						auto valeur1 = genere_ri_transformee_pour_noeud(condition, nullptr);
						auto valeur2 = cree_constante_entiere(type_condition, 0);
						valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
						break;
					}
					case GenreType::BOOL:
					{
						auto valeur1 = genere_ri_transformee_pour_noeud(condition, nullptr);
						auto valeur2 = cree_constante_booleenne(false);
						valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
						break;
					}
					case GenreType::FONCTION:
					case GenreType::POINTEUR:
					{
						auto valeur1 = genere_ri_transformee_pour_noeud(condition, nullptr);
						auto valeur2 = cree_constante_nulle(type_condition);
						valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
						break;
					}
					case GenreType::CHAINE:
					case GenreType::TABLEAU_DYNAMIQUE:
					{
						auto pointeur = genere_ri_pour_noeud(condition);
						auto pointeur_taille = cree_acces_membre(pointeur, 1);
						auto valeur1 = cree_charge_mem(pointeur_taille);
						auto valeur2 = cree_z64(0);
						valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
						break;
					}
					default:
					{
						break;
					}
				}

				return valeur;
			}

			auto valeur = genere_ri_transformee_pour_noeud(expr_un->expr, nullptr);

			if (expr_un->op->est_basique) {
				return cree_op_unaire(expr_un->type, expr_un->op->genre, valeur);
			}

			auto decl = expr_un->op->decl;
			auto requiers_contexte = !decl->est_externe && !dls::outils::possede_drapeau(decl->drapeaux, FORCE_NULCTX);
			auto atome_fonction = table_fonctions[decl->nom_broye];
			auto args = kuri::tableau<Atome *>(1 + requiers_contexte);

			if (requiers_contexte) {
				args[0] = cree_charge_mem(table_locales[ident_contexte]);
			}

			args[0 + requiers_contexte] = valeur;

			return cree_appel(noeud->lexeme, atome_fonction, std::move(args));
		}
		case GenreNoeud::INSTRUCTION_RETIENS:
		{
			genere_ri_pour_retiens(noeud);
			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_RETOUR:
		{
			genere_ri_blocs_differes(noeud->bloc_parent);
			return cree_retour(nullptr);
		}
		case GenreNoeud::INSTRUCTION_RETOUR_SIMPLE:
		{
			auto inst_retour = static_cast<NoeudExpressionUnaire *>(noeud);
			auto expr = inst_retour->expr;

			if (est_expression_logique(expr)) {
				auto label_si_vrai = reserve_label();
				auto label_si_faux = reserve_label();

				genere_ri_pour_condition(expr, label_si_vrai, label_si_faux);
				insere_label(label_si_vrai);
				cree_retour(cree_constante_booleenne(true));
				insere_label(label_si_faux);
				cree_retour(cree_constante_booleenne(false));
				return nullptr;
			}

			auto valeur = genere_ri_transformee_pour_noeud(inst_retour->expr, nullptr);
			genere_ri_blocs_differes(noeud->bloc_parent);
			return cree_retour(valeur);
		}
		case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
		{
			genere_ri_blocs_differes(noeud->bloc_parent);
			return cree_retour(nullptr);
		}
		case GenreNoeud::INSTRUCTION_SAUFSI:
		case GenreNoeud::INSTRUCTION_SI:
		{
			// À FAIRE: si comme expression (a := si b { c } sinon { d }
			auto inst_si = static_cast<NoeudSi *>(noeud);

			auto label_si_vrai = reserve_label();
			auto label_si_faux = reserve_label();

			if (noeud->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
				genere_ri_pour_condition(inst_si->condition, label_si_faux, label_si_vrai);
			}
			else {
				genere_ri_pour_condition(inst_si->condition, label_si_vrai, label_si_faux);
			}

			if (inst_si->bloc_si_faux) {
				auto label_apres_instruction = reserve_label();

				insere_label(label_si_vrai);
				genere_ri_pour_noeud(inst_si->bloc_si_vrai);
				cree_branche(label_apres_instruction);
				insere_label(label_si_faux);
				genere_ri_pour_noeud(inst_si->bloc_si_faux);
				insere_label(label_apres_instruction);
			}
			else {
				insere_label(label_si_vrai);
				genere_ri_pour_noeud(inst_si->bloc_si_vrai);
				insere_label(label_si_faux);
			}

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_POUR:
		{
			auto noeud_pour = static_cast<NoeudPour *>(noeud);
			return genere_ri_pour_boucle_pour(noeud_pour);
		}
		case GenreNoeud::INSTRUCTION_BOUCLE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_boucle = reserve_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_boucle, label_apres_boucle);

			insere_label(label_boucle);
			genere_ri_pour_noeud(inst_boucle->bloc);
			cree_branche(label_boucle);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_REPETE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_boucle = reserve_label();
			auto label_condition = reserve_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_condition, label_apres_boucle);

			insere_label(label_boucle);
			genere_ri_pour_noeud(inst_boucle->bloc);
			insere_label(label_condition);
			genere_ri_pour_condition(inst_boucle->condition, label_boucle, label_apres_boucle);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_TANTQUE:
		{
			auto inst_boucle = static_cast<NoeudBoucle *>(noeud);
			auto label_condition = reserve_label();
			auto label_boucle = reserve_label();
			auto label_apres_boucle = reserve_label();

			empile_controle_boucle(nullptr, label_condition, label_apres_boucle);

			insere_label(label_condition);
			genere_ri_pour_condition(inst_boucle->condition, label_boucle, label_apres_boucle);

			insere_label(label_boucle);
			genere_ri_pour_noeud(inst_boucle->bloc);
			cree_branche(label_condition);
			insere_label(label_apres_boucle);

			depile_controle_boucle();

			return nullptr;
		}
		case GenreNoeud::INSTRUCTION_CONTINUE_ARRETE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
			auto label = static_cast<InstructionLabel *>(nullptr);

			if (inst->expr == nullptr) {
				if (inst->lexeme->genre == GenreLexeme::CONTINUE) {
					label = insts_continue_arrete.back().t1;
				}
				else {
					label = insts_continue_arrete.back().t2;
				}
			}
			else {
				auto ident = inst->expr->ident;

				POUR (insts_continue_arrete) {
					if (it.t0 != ident) {
						continue;
					}

					if (inst->lexeme->genre == GenreLexeme::CONTINUE) {
						label = it.t1;
					}
					else {
						label = it.t2;
					}

					break;
				}
			}

			return cree_branche(label);
		}
		case GenreNoeud::EXPRESSION_COMME:
		{
			auto inst = static_cast<NoeudExpressionBinaire *>(noeud);
			auto enfant = inst->expr1;
			auto const &type_de = enfant->type;

			auto valeur_enfant = genere_ri_pour_noeud(enfant);

			if (type_de == inst->type) {
				return valeur_enfant;
			}

			if (valeur_enfant->est_chargeable && enfant->lexeme->genre != GenreLexeme::AROBASE && enfant->genre_valeur != GenreValeur::DROITE) {
				valeur_enfant = cree_charge_mem(valeur_enfant);
			}

			return cree_transtype(inst->type, valeur_enfant);
		}
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			auto type = expr->expr->type;
			return cree_constante_entiere(noeud->type, type->taille_octet);
		}
		case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
		{
			auto noeud_tableau = static_cast<NoeudTableauArgsVariadiques *>(noeud);
			auto taille_tableau = noeud_tableau->exprs.taille;
			auto type_tableau_fixe = m_compilatrice.typeuse.type_tableau_fixe(noeud->type, taille_tableau);
			auto pointeur_tableau = cree_allocation(type_tableau_fixe, nullptr);

			auto index = 0ul;
			POUR (noeud_tableau->exprs) {
				auto index_tableau = cree_acces_index(pointeur_tableau, cree_z64(index++));
				genere_ri_transformee_pour_noeud(it, index_tableau);
			}

			return converti_vers_tableau_dyn(pointeur_tableau, type_tableau_fixe, nullptr);
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		{
			auto expr = static_cast<NoeudExpressionAppel *>(noeud);
			auto alloc = static_cast<Atome *>(nullptr);

			if (expr->type->genre == GenreType::UNION) {
				auto index_membre = 0u;
				auto valeur = static_cast<Atome *>(nullptr);
				auto type_union = static_cast<TypeUnion *>(expr->type);

				POUR (expr->exprs) {
					if (it != nullptr) {
						valeur = genere_ri_transformee_pour_noeud(it, nullptr);

						if (it->type != type_union->type_le_plus_grand) {
							auto ptr = static_cast<InstructionChargeMem *>(valeur)->chargee;
							valeur = cree_transtype(m_compilatrice.typeuse.type_pointeur_pour(type_union->type_le_plus_grand), ptr);
							valeur = cree_charge_mem(valeur);
						}

						break;
					}

					index_membre += 1;
				}

				alloc = cree_allocation(type_union, nullptr);
				auto ptr_valeur = cree_acces_membre(alloc, 0);
				cree_stocke_mem(ptr_valeur, valeur);

				if (type_union->est_nonsure) {
					auto ptr_index = cree_acces_membre(alloc, 0);
					cree_stocke_mem(ptr_index, cree_z32(index_membre + 1));
				}
			}
			else {
				if (noeud->lexeme->chaine == "PositionCodeSource") {
					return genere_ri_pour_position_code_source(noeud);
				}

				auto type_struct = static_cast<TypeStructure *>(noeud->type);
				auto index_membre = 0u;

				alloc = cree_allocation(type_struct, nullptr);

				POUR (expr->exprs) {
					auto valeur = static_cast<Atome *>(nullptr);

					if (it != nullptr) {
						valeur = genere_ri_transformee_pour_noeud(it, nullptr);
					}
					else {
						valeur = genere_initialisation_defaut_pour_type(type_struct->membres[index_membre].type);
					}

					auto ptr = cree_acces_membre(alloc, index_membre);
					cree_stocke_mem(ptr, valeur);

					index_membre += 1;
				}
			}

			return alloc;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);

			dls::tablet<NoeudExpression *, 10> feuilles;
			rassemble_feuilles(expr->expr, feuilles);

			if (fonction_courante == nullptr) {
				auto type_tableau_fixe = static_cast<TypeTableauFixe *>(expr->type);
				kuri::tableau<AtomeConstante *> valeurs;
				valeurs.reserve(feuilles.taille());

				POUR (feuilles) {
					auto valeur = genere_ri_pour_noeud(it);
					valeurs.pousse(static_cast<AtomeConstante *>(valeur));
				}

				auto tableau_constant = cree_constante_tableau_fixe(type_tableau_fixe, std::move(valeurs));
				return tableau_constant;
			}

			auto pointeur_tableau = cree_allocation(expr->type, nullptr);

			auto index = 0ul;
			POUR (feuilles) {
				auto valeur = genere_ri_transformee_pour_noeud(it, nullptr);
				auto index_tableau = cree_acces_index(pointeur_tableau, cree_z64(index++));
				cree_stocke_mem(index_tableau, valeur);
			}

			return pointeur_tableau;
		}
		case GenreNoeud::EXPRESSION_INFO_DE:
		{
			auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
			auto enfant = inst->expr;
			auto valeur = cree_info_type(enfant->type);
			valeur->est_chargeable = false;
			return valeur;
		}
		case GenreNoeud::EXPRESSION_INIT_DE:
		{
			auto type_fonction = static_cast<TypeFonction *>(noeud->type);
			auto type_pointeur = type_fonction->types_entrees[1];
			auto type_arg = static_cast<TypePointeur *>(type_pointeur)->type_pointe;
			auto nom_fonction = "initialise_" + dls::vers_chaine(type_arg);
			return table_fonctions[nom_fonction];
		}
		case GenreNoeud::EXPRESSION_TYPE_DE:
		{
			auto expr = static_cast<NoeudExpressionUnaire *>(noeud);
			auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(expr->type);

			if (type_de_donnees->type_connu == nullptr) {
				return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->index_dans_table_types);
			}

			return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->type_connu->index_dans_table_types);
		}
		case GenreNoeud::EXPRESSION_MEMOIRE:
		{
			auto inst_mem = static_cast<NoeudExpressionUnaire *>(noeud);
			auto valeur = genere_ri_pour_noeud(inst_mem->expr);

			if (valeur->genre_atome == Atome::Genre::INSTRUCTION
					&& dls::outils::est_element(static_cast<Instruction *>(valeur)->genre, Instruction::Genre::ALLOCATION, Instruction::Genre::ACCEDE_MEMBRE, Instruction::Genre::ACCEDE_INDEX)
					&& !expression_gauche)
			{
				valeur = cree_charge_mem(valeur);
			}

			valeur = cree_charge_mem(valeur);
			valeur->est_chargeable = false;
			return valeur;
		}
		case GenreNoeud::EXPRESSION_LOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->type, 0, expr, expr->expr, expr->expr_taille, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_DELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->expr->type, 2, expr->expr, expr->expr, expr->expr_taille, expr->bloc);
		}
		case GenreNoeud::EXPRESSION_RELOGE:
		{
			auto expr = static_cast<NoeudExpressionLogement *>(noeud);
			return genere_ri_pour_logement(expr->expr->type, 1, expr, expr->expr, expr->expr_taille, expr->bloc);
		}
		case GenreNoeud::DECLARATION_STRUCTURE:
		{
			auto noeud_struct = static_cast<NoeudStruct *>(noeud);
			return genere_ri_pour_declaration_structure(noeud_struct);
		}
		case GenreNoeud::INSTRUCTION_DISCR:
		case GenreNoeud::INSTRUCTION_DISCR_ENUM:
		case GenreNoeud::INSTRUCTION_DISCR_UNION:
		{
			auto noeud_discr = static_cast<NoeudDiscr *>(noeud);
			return genere_ri_pour_discr(noeud_discr);
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		{
			return genere_ri_transformee_pour_noeud(static_cast<NoeudExpressionParenthese *>(noeud)->expr, nullptr);
		}
		case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
		{
			auto noeud_pc = static_cast<NoeudPousseContexte *>(noeud);
			auto atome_nouveau_contexte = genere_ri_pour_noeud(noeud_pc->expr);
			auto atome_ancien_contexte = table_locales[ident_contexte];

			table_locales[ident_contexte] = atome_nouveau_contexte;
			genere_ri_pour_noeud(noeud_pc->bloc);
			table_locales[ident_contexte] = atome_ancien_contexte;

			return nullptr;
		}
		case GenreNoeud::EXPANSION_VARIADIQUE:
		{
			return genere_ri_pour_expression_droite(static_cast<NoeudExpressionUnaire *>(noeud)->expr);
		}
		case GenreNoeud::INSTRUCTION_TENTE:
		{
			auto noeud_tente = static_cast<NoeudTente *>(noeud);
			return genere_ri_pour_tente(noeud_tente);
		}
	}

	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_expression_droite(NoeudExpression *noeud)
{
	expression_gauche = false;
	auto atome = genere_ri_pour_noeud(noeud);
	expression_gauche = true;

	if (!atome->est_chargeable) {
		return atome;
	}

	return cree_charge_mem(atome);
}

Atome *ConstructriceRI::genere_ri_transformee_pour_noeud(NoeudExpression *noeud, Atome *place)
{
	auto &transformation = noeud->transformation;
	expression_gauche = false;
	auto valeur = genere_ri_pour_noeud(noeud);
	expression_gauche = true;

	auto place_fut_utilisee = false;

	if (valeur == nullptr) {
		std::cerr << __func__ << ", valeur est nulle pour " << chaine_genre_noeud(noeud->genre) << '\n';
		imprime_fichier_ligne(m_compilatrice, *noeud->lexeme);
	}

	switch (transformation.type) {
		case TypeTransformation::IMPOSSIBLE:
		{
			break;
		}
		case TypeTransformation::INUTILE:
		{
			if (valeur->est_chargeable && noeud->lexeme->genre != GenreLexeme::AROBASE) {
				valeur = cree_charge_mem(valeur);
			}

			break;
		}
		case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
		{
			// valeur est déjà une constante, change simplement le type
			valeur->type = transformation.type_cible;
			assert(valeur->type->genre != GenreType::ENTIER_CONSTANT);
			break;
		}
		case TypeTransformation::CONSTRUIT_UNION:
		{
			auto type_union = static_cast<TypeUnion *>(transformation.type_cible);

			auto alloc = cree_allocation(type_union, nullptr);

		/*	if (type_union->type_le_plus_grand != noeud->type) {
				valeur = cree_transtype(m_compilatrice.typeuse.type_pointeur_pour(type_union->type_le_plus_grand), valeur);
				valeur = cree_charge_mem(valeur);
			}
			else*/ if (valeur->est_chargeable) {
				valeur = cree_charge_mem(valeur);
			}

			auto acces_membre = cree_acces_membre(alloc, transformation.index_membre);
			cree_stocke_mem(acces_membre, valeur);

			if (!type_union->est_nonsure) {
				acces_membre = cree_acces_membre(alloc, type_union->membres.taille);
				auto index = cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z32], static_cast<unsigned long>(transformation.index_membre + 1));
				cree_stocke_mem(acces_membre, index);
			}

			valeur = cree_charge_mem(alloc);
			break;
		}
		case TypeTransformation::EXTRAIT_UNION:
		{
			auto type_union = static_cast<TypeUnion *>(noeud->type);

			if (!valeur->est_chargeable) {
				auto alloc = cree_allocation(valeur->type, nullptr);
				cree_stocke_mem(alloc, valeur);
				valeur = alloc;
			}

			if (!type_union->est_nonsure) {
				auto membre_actif = cree_acces_membre_et_charge(valeur, type_union->membres.taille);

				auto label_si_vrai = reserve_label();
				auto label_si_faux = reserve_label();

				auto condition = cree_op_comparaison(
							OperateurBinaire::Genre::Comp_Inegal,
							membre_actif,
							cree_z32(static_cast<unsigned>(transformation.index_membre + 1)));

				cree_branche_condition(condition, label_si_vrai, label_si_faux);
				insere_label(label_si_vrai);
				// À FAIRE : nous pourrions avoir une erreur différente ici.
				auto params = kuri::tableau<Atome *>(1);
				params[0] = cree_charge_mem(table_locales[ident_contexte]);
				cree_appel(noeud->lexeme, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_membre_union), std::move(params));
				insere_label(label_si_faux);
			}

			auto membre = cree_acces_membre(valeur, 0);
			valeur = cree_transtype(m_compilatrice.typeuse.type_pointeur_pour(transformation.type_cible), membre);
			valeur = cree_charge_mem(valeur);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
		{
			if (valeur->est_chargeable && noeud->lexeme->genre != GenreLexeme::AROBASE && noeud->genre != GenreNoeud::EXPRESSION_COMME) {
				valeur = cree_charge_mem(valeur);
			}

			if (noeud->genre != GenreNoeud::EXPRESSION_LITTERALE_NUL) {
				valeur = cree_transtype(m_compilatrice.typeuse[TypeBase::PTR_RIEN], valeur);
			}

			break;
		}
		case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
		{
			if (valeur->est_chargeable) {
				valeur = cree_charge_mem(valeur);
			}

			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
		case TypeTransformation::AUGMENTE_TAILLE_TYPE:
		{
			if (valeur->est_chargeable) {
				valeur = cree_charge_mem(valeur);
			}

			// À FAIRE(ri) : granularise les expressions de transtypage

//			if (llvm::isa<llvm::Constant>(valeur)) {
//				auto type_donnees_llvm = converti_type_llvm(compilatrice, b->type);
//				auto alloc_const = builder.CreateAlloca(type_donnees_llvm, 0u);
//				builder.CreateStore(valeur, alloc_const);
//				valeur = builder.CreateLoad(alloc_const, "");
//			}
//			else if (llvm::isa<llvm::AllocaInst>(valeur) || llvm::isa<llvm::GetElementPtrInst>(valeur)) {
//				valeur = builder.CreateLoad(valeur, "");
//			}

			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
		case TypeTransformation::CONSTRUIT_EINI:
		{
			auto alloc_eini = place;

			if (alloc_eini == nullptr) {
				auto type_eini = m_compilatrice.typeuse[TypeBase::EINI];
				alloc_eini = cree_allocation(type_eini, nullptr);
			}

			/* copie le pointeur de la valeur vers le type eini */
			auto ptr_eini = cree_acces_membre(alloc_eini, 0);

			if (!valeur->est_chargeable) {
				auto alloc_tmp = cree_allocation(valeur->type, nullptr);
				cree_stocke_mem(alloc_tmp, valeur);
				valeur = alloc_tmp;
			}

			auto transtype = cree_transtype(m_compilatrice.typeuse[TypeBase::PTR_RIEN], valeur);
			cree_stocke_mem(ptr_eini, transtype);

			/* copie le pointeur vers les infos du type du eini */
			auto tpe_eini = cree_acces_membre(alloc_eini, 1);
			auto info_type = cree_info_type(noeud->type);
			auto ptr_info_type = cree_transtype_constant(
						m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
						info_type);

			cree_stocke_mem(tpe_eini, ptr_info_type);

			if (place == nullptr) {
				valeur = cree_charge_mem(alloc_eini);
			}
			else {
				place_fut_utilisee = true;
			}

			break;
		}
		case TypeTransformation::EXTRAIT_EINI:
		{
			valeur = cree_acces_membre(valeur, 0);
			auto type_cible = m_compilatrice.typeuse.type_pointeur_pour(transformation.type_cible);
			valeur = cree_transtype(type_cible, valeur);
			valeur = cree_charge_mem(valeur);
			break;
		}
		case TypeTransformation::CONSTRUIT_TABL_OCTET:
		{
			auto valeur_pointeur = static_cast<Atome *>(nullptr);
			auto valeur_taille = static_cast<Atome *>(nullptr);

			auto type_cible = m_compilatrice.typeuse[TypeBase::PTR_OCTET];

			switch (noeud->type->genre) {
				default:
				{
					// À FAIRE(ri)
//					if (llvm::isa<llvm::Constant>(valeur)) {
//						auto type_donnees_llvm = converti_type_llvm(compilatrice, noeud->type);
//						auto alloc_const = builder.CreateAlloca(type_donnees_llvm, 0u);
//						builder.CreateStore(valeur, alloc_const);
//						valeur = alloc_const;
//					}

					valeur = cree_transtype(type_cible, valeur);
					valeur_pointeur = valeur;

					if (noeud->type->genre == GenreType::ENTIER_CONSTANT) {
						valeur_taille = cree_z64(4);
					}
					else {
						valeur_taille = cree_z64(noeud->type->taille_octet);
					}

					break;
				}
				case GenreType::POINTEUR:
				{
					auto type_pointe = static_cast<TypePointeur *>(noeud->type)->type_pointe;
					valeur = cree_transtype(type_cible, valeur);
					valeur_pointeur = valeur;
					auto taille_type = type_pointe->taille_octet;
					valeur_taille = cree_z64(taille_type);
					break;
				}
				case GenreType::CHAINE:
				{
					// À FAIRE(ri)
					/*if (llvm::isa<llvm::Constant>(valeur_chaine)) {
						auto const_struct = static_cast<llvm::ConstantStruct *>(valeur_chaine);
						valeur_pointeur = const_struct->getAggregateElement(0u);
						valeur_taille = const_struct->getAggregateElement(1);
					}
					else*/ {
						valeur_pointeur = cree_acces_membre_et_charge(valeur, 0);
						valeur_pointeur = cree_transtype(type_cible, valeur_pointeur);
						valeur_taille = cree_acces_membre_et_charge(valeur, 1);
					}

					break;
				}
				case GenreType::TABLEAU_DYNAMIQUE:
				{
					auto type_pointer = static_cast<TypeTableauDynamique *>(noeud->type)->type_pointe;

					valeur_pointeur = cree_acces_membre_et_charge(valeur, 0);
					valeur_pointeur = cree_transtype(type_cible, valeur_pointeur);
					valeur_taille = cree_acces_membre_et_charge(valeur, 1);

					auto taille_type = type_pointer->taille_octet;

					valeur_taille = cree_op_binaire(
								m_compilatrice.typeuse[TypeBase::Z64],
								OperateurBinaire::Genre::Multiplication,
								valeur_taille,
								cree_z64(taille_type));

					break;
				}
				case GenreType::TABLEAU_FIXE:
				{
					auto type_tabl = static_cast<TypeTableauFixe *>(noeud->type);
					auto type_pointe = type_tabl->type_pointe;
					auto taille_type = type_pointe->taille_octet;

					valeur_pointeur = cree_acces_index(valeur, cree_z64(0ul));
					valeur_pointeur = cree_transtype(type_cible, valeur_pointeur);
					valeur_taille = cree_z64(static_cast<unsigned>(type_tabl->taille) * taille_type);

					break;
				}
			}

			/* alloue de l'espace pour ce type */
			auto tabl_octet = cree_allocation(m_compilatrice.typeuse[TypeBase::TABL_OCTET], nullptr);

			auto pointeur_tabl_octet = cree_acces_membre(tabl_octet, 0);
			cree_stocke_mem(pointeur_tabl_octet, valeur_pointeur);

			auto taille_tabl_octet = cree_acces_membre(tabl_octet, 1);
			cree_stocke_mem(taille_tabl_octet, valeur_taille);

			valeur = cree_charge_mem(tabl_octet);
			break;
		}
		case TypeTransformation::CONVERTI_TABLEAU:
		{
			if (fonction_courante == nullptr) {
				auto valeur_tableau_fixe = static_cast<AtomeConstante *>(valeur);
				return cree_tableau_global(valeur_tableau_fixe);
			}

			valeur = converti_vers_tableau_dyn(valeur, static_cast<TypeTableauFixe *>(noeud->type), place);

			if (place == nullptr) {
				valeur = cree_charge_mem(valeur);
			}
			else {
				place_fut_utilisee = true;
			}

			break;
		}
		case TypeTransformation::FONCTION:
		{
			auto atome_fonction = trouve_ou_insere_fonction(transformation.fonction);

			if (valeur->est_chargeable) {
				valeur = cree_charge_mem(valeur);
			}

			auto args = kuri::tableau<Atome *>();
			args.pousse(valeur);

			valeur = cree_appel(noeud->lexeme, atome_fonction, std::move(args));
			break;
		}
		case TypeTransformation::PREND_REFERENCE:
		{
			// RÀF : valeur doit déjà être un pointeur
			break;
		}
		case TypeTransformation::DEREFERENCE:
		{
			valeur = cree_charge_mem(valeur);
			valeur = cree_charge_mem(valeur);
			break;
		}
		case TypeTransformation::CONVERTI_VERS_BASE:
		{
			// À FAIRE : décalage dans la structure
			valeur = cree_charge_mem(valeur);
			valeur = cree_transtype(transformation.type_cible, valeur);
			break;
		}
	}

	if (place && !place_fut_utilisee) {
		cree_stocke_mem(place, valeur);
	}

	return valeur;
}

Atome *ConstructriceRI::genere_ri_pour_discr(NoeudDiscr *noeud)
{
	auto expression = noeud->expr;
	auto op = noeud->op;
	auto decl_struct = static_cast<NoeudStruct *>(nullptr);

	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
		auto type_struct = static_cast<TypeStructure *>(expression->type);
		decl_struct = type_struct->decl;
	}

	struct DonneesPaireDiscr {
		NoeudExpression *expr = nullptr;
		NoeudBloc *bloc = nullptr;
		InstructionLabel *bloc_de_la_condition = nullptr;
		InstructionLabel *bloc_si_vrai = nullptr;
		InstructionLabel *bloc_si_faux = nullptr;
	};

	auto bloc_post_discr = reserve_label();

	dls::tableau<DonneesPaireDiscr> donnees_paires;
	donnees_paires.reserve(noeud->paires_discr.taille + noeud->bloc_sinon != nullptr);

	POUR (noeud->paires_discr) {
		auto donnees = DonneesPaireDiscr();
		donnees.expr = it.first;
		donnees.bloc = it.second;
		donnees.bloc_de_la_condition = reserve_label();
		donnees.bloc_si_vrai = reserve_label();

		if (!donnees_paires.est_vide()) {
			donnees_paires.back().bloc_si_faux = donnees.bloc_de_la_condition;
		}

		donnees_paires.pousse(donnees);
	}

	if (noeud->bloc_sinon) {
		auto donnees = DonneesPaireDiscr();
		donnees.bloc = noeud->bloc_sinon;
		donnees.bloc_si_vrai = reserve_label();

		if (!donnees_paires.est_vide()) {
			donnees_paires.back().bloc_si_faux = donnees.bloc_si_vrai;
		}

		donnees_paires.pousse(donnees);
	}

	donnees_paires.back().bloc_si_faux = bloc_post_discr;

	auto valeur_expression = static_cast<Atome *>(nullptr);
	auto ptr_structure = static_cast<Atome *>(nullptr);

	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
		// prend l'index de discrimination
		valeur_expression = genere_ri_pour_noeud(expression);
		ptr_structure = valeur_expression;
		valeur_expression = cree_acces_membre(valeur_expression, 1);
		valeur_expression = cree_charge_mem(valeur_expression);
	}
	else {
		valeur_expression = genere_ri_transformee_pour_noeud(expression, nullptr);
		ptr_structure = valeur_expression;
	}

	cree_branche(donnees_paires.front().bloc_de_la_condition);

	for (auto &donnees : donnees_paires) {
		auto enf0 = donnees.expr;
		auto enf1 = donnees.bloc;

		if (enf0 != nullptr) {
			insere_label(donnees.bloc_de_la_condition);

			auto feuilles = dls::tablet<NoeudExpression *, 10>();
			rassemble_feuilles(enf0, feuilles);

			// les différentes feuilles sont évaluées dans des blocs
			// séparés afin de pouvoir éviter de tester trop de conditions
			// dès qu'une condition est vraie, nous allons dans le bloc_si_vrai
			// sinon nous allons dans le bloc pour la feuille suivante
			for (auto f : feuilles) {
				auto bloc_si_faux = donnees.bloc_si_faux;

				if (f != feuilles.back()) {
					bloc_si_faux = reserve_label();
				}

				auto valeur_f = static_cast<Atome *>(nullptr);

				if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
					valeur_f = valeur_enum(static_cast<TypeEnum *>(expression->type), f->ident);
				}
				else if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
					auto idx_membre = trouve_index_membre(decl_struct, f->ident->nom);
					valeur_f = cree_z32(idx_membre + 1);

					// À FAIRE(ri) : revoir les accès à des unions
					auto valeur = cree_acces_membre(ptr_structure, 0);

					table_locales[f->ident] = valeur;
				}
				else {
					valeur_f = genere_ri_pour_expression_droite(f);
				}

				// op est nul pour les énums
				if (!op || op->est_basique) {
					auto condition = cree_op_comparaison(
								OperateurBinaire::Genre::Comp_Egal,
								valeur_expression,
								valeur_f);

					cree_branche_condition(condition, donnees.bloc_si_vrai, bloc_si_faux);
				}
				else {
					auto decl = op->decl;
					auto requiers_contexte = !decl->est_externe && !dls::outils::possede_drapeau(decl->drapeaux, FORCE_NULCTX);
					auto atome_fonction = table_fonctions[decl->nom_broye];
					auto args = kuri::tableau<Atome *>(2 + requiers_contexte);

					if (requiers_contexte) {
						args[0] = cree_charge_mem(table_locales[ident_contexte]);
					}

					args[0 + requiers_contexte] = valeur_expression;
					args[1 + requiers_contexte] = valeur_f;

					auto condition = cree_appel(noeud->lexeme, atome_fonction, std::move(args));
					cree_branche_condition(condition, donnees.bloc_si_vrai, bloc_si_faux);
				}

				if (f != feuilles.back()) {
					insere_label(bloc_si_faux);
				}
			}
		}

		insere_label(donnees.bloc_si_vrai);

		genere_ri_pour_noeud(enf1);
		cree_branche(bloc_post_discr);
	}

	insere_label(bloc_post_discr);

	return nullptr;
}

Atome *ConstructriceRI::genere_ri_pour_tente(NoeudTente *noeud)
{
	// À FAIRE(retours multiples)
	auto valeur_expression = genere_ri_pour_expression_droite(noeud->expr_appel);

	struct DonneesGenerationCodeTente {
		Atome *acces_variable{};
		Atome *acces_erreur{};
		Atome *acces_erreur_pour_test{};

		Type *type_piege = nullptr;
		Type *type_variable = nullptr;
	};

	DonneesGenerationCodeTente gen_tente;

	if (noeud->expr_appel->type->genre == GenreType::ERREUR) {
		gen_tente.type_piege = noeud->expr_appel->type;
		gen_tente.type_variable = gen_tente.type_piege;
		gen_tente.acces_erreur = valeur_expression;
		gen_tente.acces_variable = gen_tente.acces_erreur;
		gen_tente.acces_erreur_pour_test = gen_tente.acces_erreur;

		auto label_si_vrai = reserve_label();
		auto label_si_faux = reserve_label();

		auto condition = cree_op_comparaison(
					OperateurBinaire::Genre::Comp_Inegal,
					gen_tente.acces_erreur_pour_test,
					cree_z32(0));

		cree_branche_condition(condition, label_si_vrai, label_si_faux);

		insere_label(label_si_vrai);
		if (noeud->expr_piege == nullptr) {
			auto params = kuri::tableau<Atome *>(1);
			params[0] = cree_charge_mem(table_locales[ident_contexte]);
			cree_appel(noeud->lexeme, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_erreur), std::move(params));
		}
		else {
			auto var_expr_piege = cree_allocation(gen_tente.type_piege, noeud->expr_piege->ident);
			table_locales[noeud->expr_piege->ident] = var_expr_piege;
			genere_ri_pour_noeud(noeud->bloc);
		}

		insere_label(label_si_faux);

		return valeur_expression;
	}
	else if (noeud->expr_appel->type->genre == GenreType::UNION) {
		auto type_union = static_cast<TypeUnion *>(noeud->expr_appel->type);
		auto index_membre_variable = 0;
		auto index_membre_erreur = 0;

		if (type_union->membres.taille == 2) {
			if (type_union->membres[0].type->genre == GenreType::ERREUR) {
				gen_tente.type_piege = type_union->membres[0].type;
				gen_tente.type_variable = type_union->membres[1].type;
				index_membre_variable = 1;
			}
			else {
				gen_tente.type_piege = type_union->membres[1].type;
				gen_tente.type_variable = type_union->membres[0].type;
				index_membre_erreur = 1;
			}
		}
		else {
			// À FAIRE(tente) : extraction des valeurs de l'union
		}

		// test si membre actif est erreur
		auto label_si_vrai = reserve_label();
		auto label_si_faux = reserve_label();

		auto valeur_union = cree_allocation(noeud->expr_appel->type, nullptr);
		cree_stocke_mem(valeur_union, valeur_expression);

		auto acces_membre_actif = cree_acces_membre_et_charge(valeur_union, type_union->membres.taille);

		auto condition_membre_actif = cree_op_comparaison(
					OperateurBinaire::Genre::Comp_Egal,
					acces_membre_actif,
					cree_z32(static_cast<unsigned>(index_membre_erreur + 1)));

		cree_branche_condition(condition_membre_actif, label_si_vrai, label_si_faux);

		insere_label(label_si_vrai);
		if (noeud->expr_piege == nullptr) {
			auto params = kuri::tableau<Atome *>(1);
			params[0] = cree_charge_mem(table_locales[ident_contexte]);
			cree_appel(noeud->lexeme, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_erreur), std::move(params));
		}
		else {
			table_locales[noeud->expr_piege->ident] = cree_acces_membre(valeur_union, index_membre_erreur);
			genere_ri_pour_noeud(noeud->bloc);
		}

		insere_label(label_si_faux);
		valeur_expression = cree_acces_membre(valeur_union, index_membre_variable);
	}

	return valeur_expression;
}

Atome *ConstructriceRI::genere_ri_pour_boucle_pour(NoeudPour *inst)
{
	/* on génère d'abord le type de la variable */
	auto enfant1 = inst->variable;
	auto enfant2 = inst->expression;
	auto enfant3 = inst->bloc;
	auto enfant_sans_arret = inst->bloc_sansarret;
	auto enfant_sinon = inst->bloc_sinon;

	auto type = enfant2->type;
	enfant1->type = type;

	/* création des blocs */
	auto bloc_boucle = reserve_label();
	auto bloc_corps = reserve_label();
	auto bloc_inc = reserve_label();

	auto bloc_sansarret = static_cast<InstructionLabel *>(nullptr);
	auto bloc_sinon = static_cast<InstructionLabel *>(nullptr);

	if (enfant_sans_arret) {
		bloc_sansarret = reserve_label();
	}

	if (enfant_sinon) {
		bloc_sinon = reserve_label();
	}

	auto bloc_apres = reserve_label();

	auto var = enfant1;
	auto idx = static_cast<NoeudExpression *>(nullptr);

	if (enfant1->lexeme->genre == GenreLexeme::VIRGULE) {
		auto expr_bin = static_cast<NoeudExpressionBinaire *>(var);
		var = expr_bin->expr1;
		idx = expr_bin->expr2;
	}

	empile_controle_boucle(var->ident, bloc_inc, (bloc_sinon != nullptr) ? bloc_sinon : bloc_apres);

	auto type_de_la_variable = static_cast<Type *>(nullptr);
	auto type_de_l_index = static_cast<Type *>(nullptr);

	if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE || inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
		type_de_la_variable = enfant2->type;
		type_de_l_index = enfant2->type;
	}
	else {
		type_de_la_variable = m_compilatrice.typeuse[TypeBase::Z64];
		type_de_l_index = m_compilatrice.typeuse[TypeBase::Z64];
	}

	auto valeur_debut = cree_allocation(type_de_la_variable, var->ident);
	auto valeur_index = static_cast<Atome *>(nullptr);

	if (idx != nullptr) {
		valeur_index = cree_allocation(type_de_l_index, idx->ident);
		cree_stocke_mem(valeur_index, cree_constante_entiere(type_de_l_index, 0));
	}

	if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE || inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
		auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
		auto init_debut = genere_ri_transformee_pour_noeud(expr_plage->expr1, nullptr);
		cree_stocke_mem(valeur_debut, init_debut);
	}
	else {
		cree_stocke_mem(valeur_debut, cree_z64(0));
	}

	/* bloc_boucle */
	/* on crée une branche explicite dans le bloc */
	insere_label(bloc_boucle);

	auto pointeur_tableau = static_cast<Atome *>(nullptr);

	switch (inst->aide_generation_code) {
		case GENERE_BOUCLE_PLAGE:
		case GENERE_BOUCLE_PLAGE_INDEX:
		{
			/* condition */
			auto expr_plage = static_cast<NoeudExpressionBinaire *>(enfant2);
			auto valeur_fin = genere_ri_pour_expression_droite(expr_plage->expr2);

			auto condition = cree_op_comparaison(
					OperateurBinaire::Genre::Comp_Inf_Egal,
					cree_charge_mem(valeur_debut),
					valeur_fin);

			cree_branche_condition(condition, bloc_corps, (bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

			table_locales[var->ident] = valeur_debut;
			if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
				table_locales[idx->ident] = valeur_index;
			}

			/* corps */
			insere_label(bloc_corps);

			genere_ri_pour_noeud(enfant3);

			/* suivant */
			insere_label(bloc_inc);

			cree_incrementation_valeur(type_de_la_variable, valeur_debut);

			if (valeur_index) {
				cree_incrementation_valeur(type_de_l_index, valeur_index);
			}

			cree_branche(bloc_boucle);

			break;
		}
		case GENERE_BOUCLE_TABLEAU:
		case GENERE_BOUCLE_TABLEAU_INDEX:
		{
			/* condition */
			auto taille_tableau = 0l;

			if (type->genre == GenreType::TABLEAU_FIXE) {
				taille_tableau = static_cast<TypeTableauFixe *>(type)->taille;
			}

			auto valeur_fin = static_cast<Atome *>(nullptr);

			if (taille_tableau != 0) {
				valeur_fin = cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z64], static_cast<unsigned long>(taille_tableau));
			}
			else {
				pointeur_tableau = genere_ri_pour_noeud(enfant2);

				// À FAIRE : ici ce serait bien de pouvoir accéder directement
				// aux membres de la constante, mais la génération de code C
				// échoue avec une telle approche, à vérifier avec LLVM.
				if (pointeur_tableau->genre_atome == Atome::Genre::CONSTANTE) {
					auto alloc = cree_allocation(enfant2->type, nullptr);
					cree_stocke_mem(alloc, pointeur_tableau);
					pointeur_tableau = alloc;
				}

				valeur_fin = cree_acces_membre(pointeur_tableau, 1);
				valeur_fin = cree_charge_mem(valeur_fin);
			}

			auto condition = cree_op_comparaison(
					OperateurBinaire::Genre::Comp_Inf,
					cree_charge_mem(valeur_debut),
					valeur_fin);

			cree_branche_condition(
						condition,
						bloc_corps,
						(bloc_sansarret != nullptr) ? bloc_sansarret : bloc_apres);

			insere_label(bloc_corps);

			auto valeur_arg = static_cast<Atome *>(nullptr);

			if (taille_tableau != 0) {
				auto valeur_tableau = genere_ri_pour_noeud(enfant2);
				valeur_arg = cree_acces_index(valeur_tableau, cree_charge_mem(valeur_debut));
			}
			else {
				auto pointeur = cree_acces_membre(pointeur_tableau, 0);
				valeur_arg = cree_acces_index(pointeur, cree_charge_mem(valeur_debut));
			}

			table_locales[var->ident] = valeur_arg;
			if (inst->aide_generation_code == GENERE_BOUCLE_TABLEAU_INDEX) {
				table_locales[idx->ident] = valeur_index;
			}

			/* corps */
			genere_ri_pour_noeud(enfant3);

			/* suivant */
			insere_label(bloc_inc);

			cree_incrementation_valeur(type_de_la_variable, valeur_debut);

			if (valeur_index) {
				cree_incrementation_valeur(type_de_l_index, valeur_index);
			}

			cree_branche(bloc_boucle);

			break;
		}
		case GENERE_BOUCLE_COROUTINE:
		case GENERE_BOUCLE_COROUTINE_INDEX:
		{
			/* À FAIRE(ri) : coroutine */
#if 0
			auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
			auto decl_fonc = static_cast<NoeudDeclarationFonction const *>(expr_appel->noeud_fonction_appelee);
			auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
			auto nom_type_coro = "__etat_coro" + decl_fonc->nom_broye;

			constructrice << nom_type_coro << " " << nom_etat << " = {\n";
			constructrice << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".contexte = contexte,\n";
			constructrice << ".__termine_coro = 0\n";
			constructrice << "};\n";

			/* intialise les arguments de la fonction. */
			POUR (expr_appel->params) {
				genere_code_C(it, constructrice, compilatrice, false);
			}

			auto iter_enf = expr_appel->params.begin();

			POUR (decl_fonc->params) {
				auto nom_broye = broye_nom_simple(it->ident->nom);
				constructrice << nom_etat << '.' << nom_broye << " = ";
				constructrice << (*iter_enf)->chaine_calculee();
				constructrice << ";\n";
				++iter_enf;
			}

			constructrice << "pthread_t fil_coro;\n";
			constructrice << "pthread_create(&fil_coro, NULL, " << decl_fonc->nom_broye << ", &" << nom_etat << ");\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			/* À FAIRE : utilisation du type */
			auto nombre_vars_ret = decl_fonc->type_fonc->types_sorties.taille;

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant1, feuilles);

			auto idx = static_cast<NoeudExpression *>(nullptr);
			auto nom_idx = dls::chaine{};

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				idx = feuilles.back();
				nom_idx = "__idx" + dls::vers_chaine(b);
				constructrice << "int " << nom_idx << " = 0;";
			}

			constructrice << "while (" << nom_etat << ".__termine_coro == 0) {\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

			for (auto i = 0l; i < nombre_vars_ret; ++i) {
				auto f = feuilles[i];
				auto nom_var_broye = broye_chaine(f);
				constructrice.declare_variable(type, nom_var_broye, "");
				constructrice << nom_var_broye << " = "
							   << nom_etat << '.' << decl_fonc->noms_retours[i]
							   << ";\n";
			}

			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			if (idx) {
				constructrice << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
				constructrice << nom_idx << " += 1;";
			}
#endif
			break;
		}
	}

	/* 'continue'/'arrête' dans les blocs 'sinon'/'sansarrêt' n'a aucun sens */
	depile_controle_boucle();

	if (enfant_sans_arret) {
		insere_label(bloc_sansarret);
		genere_ri_pour_noeud(enfant_sans_arret);
		cree_branche(bloc_apres);
	}

	if (enfant_sinon) {
		insere_label(bloc_sinon);
		genere_ri_pour_noeud(enfant_sinon);
	}

	insere_label(bloc_apres);

	return nullptr;
}

void ConstructriceRI::cree_incrementation_valeur(Type *type, Atome *valeur)
{
	auto val_inc = static_cast<Atome *>(nullptr);

	if (type->genre == GenreType::ENTIER_NATUREL) {
		val_inc = cree_constante_entiere(type, 1);
	}
	else if (type->genre == GenreType::ENTIER_RELATIF) {
		val_inc = cree_constante_entiere(type, 1);
	}
	// À FAIRE(ri) : r16
	else if (type->genre == GenreType::REEL) {
		val_inc = cree_constante_reelle(type, 1.0);
	}

	auto inc = cree_op_binaire(type, OperateurBinaire::Genre::Addition, cree_charge_mem(valeur), val_inc);
	cree_stocke_mem(valeur, inc);
}

Atome *ConstructriceRI::genere_ri_pour_logement(Type *type, int mode, NoeudExpression *noeud, NoeudExpression *variable, NoeudExpression *expression, NoeudExpression *bloc_sinon)
{
	auto type_du_pointeur_retour = type;
	auto val_enfant = static_cast<Atome *>(nullptr);
	auto val_acces_pointeur = static_cast<Atome *>(nullptr);
	auto val_acces_capacite = static_cast<Atome *>(nullptr);
	auto val_ancienne_taille_octet = static_cast<Atome *>(nullptr);
	auto val_nouvelle_taille_octet = static_cast<Atome *>(nullptr);
	auto val_ancien_nombre_element = static_cast<Atome *>(nullptr);
	auto val_nouveau_nombre_element = static_cast<Atome *>(nullptr);

	/* variable n'est nul que pour les allocations simples */
	if (variable != nullptr) {
		assert(mode == 1 || mode == 2);
		val_enfant = genere_ri_pour_noeud(variable);
	}
	else {
		assert(mode == 0);
		val_enfant = cree_allocation(type, nullptr);
		auto init = genere_initialisation_defaut_pour_type(type);
		cree_stocke_mem(val_enfant, init);
	}

	switch (type->genre) {
		case GenreType::TABLEAU_DYNAMIQUE:
		{
			auto type_deref = type_dereference_pour(type);
			type_du_pointeur_retour = m_compilatrice.typeuse.type_pointeur_pour(type_deref);

			val_acces_pointeur = cree_acces_membre(val_enfant, 0);
			val_acces_capacite = cree_acces_membre(val_enfant, 2);

			auto taille_type = type_deref->taille_octet;

			val_ancien_nombre_element = cree_charge_mem(val_acces_capacite);
			val_ancienne_taille_octet = cree_op_binaire(
						m_compilatrice.typeuse[TypeBase::Z64],
						OperateurBinaire::Genre::Multiplication,
						val_ancien_nombre_element,
						cree_z64(taille_type));

			/* allocation ou réallocation */
			if (expression != nullptr) {
				auto val_expr = genere_ri_transformee_pour_noeud(expression, nullptr);
				val_nouveau_nombre_element = val_expr;

				val_nouvelle_taille_octet = cree_op_binaire(
							m_compilatrice.typeuse[TypeBase::Z64],
							OperateurBinaire::Genre::Multiplication,
							val_nouveau_nombre_element,
							cree_z64(taille_type));
			}
			/* désallocation */
			else {
				val_nouveau_nombre_element = cree_z64(0);
				val_nouvelle_taille_octet = cree_z64(0);
			}

			break;
		}
		case GenreType::CHAINE:
		{
			type_du_pointeur_retour = m_compilatrice.typeuse[TypeBase::PTR_Z8];
			val_acces_pointeur = cree_acces_membre(val_enfant, 0);
			val_acces_capacite = cree_acces_membre(val_enfant, 1);
			val_ancienne_taille_octet = cree_charge_mem(val_acces_capacite);

			/* allocation ou réallocation */
			if (expression != nullptr) {
				auto val_expr = genere_ri_transformee_pour_noeud(expression, nullptr);
				val_nouveau_nombre_element = val_expr;
				val_nouvelle_taille_octet = val_nouveau_nombre_element;
			}
			/* désallocation */
			else {
				val_nouveau_nombre_element = cree_z64(0);
				val_nouvelle_taille_octet = cree_z64(0);
			}

			break;
		}
		default:
		{
			val_acces_pointeur = val_enfant;

			auto type_deref = type_dereference_pour(type);

			auto taille_octet = type_deref->taille_octet;
			val_ancienne_taille_octet = cree_z64(taille_octet);

			/* allocation ou réallocation */
			val_nouvelle_taille_octet = val_ancienne_taille_octet;
		}
	}

	auto ptr_contexte = table_locales[ident_contexte];

	// int mode = ...;
	// long nouvelle_taille_octet = ...;
	// long ancienne_taille_octet = ...;
	// void *pointeur = ...;
	// void *données = contexte->données_allocatrice;
	// InfoType *info_type = ...;
	// contexte->allocatrice(mode, nouvelle_taille_octet, ancienne_taille_octet, pointeur, données, info_type);

	auto arg_ptr_allocatrice = cree_acces_membre_et_charge(ptr_contexte, 0);
	auto arg_ptr_donnees = cree_acces_membre_et_charge(ptr_contexte, 1);

	auto ptr_info_type = cree_info_type(type);
	auto arg_ptr_info_type = cree_transtype(
				m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
				ptr_info_type);

	auto arg_val_mode = cree_z32(static_cast<unsigned>(mode));

	auto arg_val_ancien_ptr = cree_transtype(
				m_compilatrice.typeuse[TypeBase::PTR_RIEN],
				cree_charge_mem(val_acces_pointeur));

	auto arg_position = genere_ri_pour_position_code_source(noeud);

	kuri::tableau<Atome *> parametres;
	parametres.pousse(cree_charge_mem(ptr_contexte));
	parametres.pousse(arg_val_mode);
	parametres.pousse(val_nouvelle_taille_octet);
	parametres.pousse(val_ancienne_taille_octet);
	parametres.pousse(arg_val_ancien_ptr);
	parametres.pousse(arg_ptr_donnees);
	parametres.pousse(arg_ptr_info_type);
	parametres.pousse(cree_charge_mem(arg_position));

	auto ret_pointeur = cree_appel(noeud->lexeme, arg_ptr_allocatrice, std::move(parametres));
	auto ptr_transtype = cree_transtype(type_du_pointeur_retour, ret_pointeur);

	switch (mode) {
		case 0:
		case 1:
		{
			auto label_si_vrai = reserve_label();
			auto label_si_faux = reserve_label();

			auto condition = cree_op_comparaison(OperateurBinaire::Genre::Comp_Egal, ret_pointeur, cree_constante_nulle(m_compilatrice.typeuse[TypeBase::PTR_RIEN]));
			cree_branche_condition(condition, label_si_vrai, label_si_faux);

			insere_label(label_si_vrai);

			if (bloc_sinon) {
				genere_ri_pour_noeud(bloc_sinon);
			}
			else {
				auto params = kuri::tableau<Atome *>(1);
				params[0] = cree_charge_mem(table_locales[ident_contexte]);
				cree_appel(noeud->lexeme, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_memoire), std::move(params));
			}

			insere_label(label_si_faux);

			if (mode == 0 && type->genre != GenreType::TABLEAU_DYNAMIQUE && type->genre != GenreType::CHAINE) {
				auto type_deref = type_dereference_pour(type);
				auto nom_fonction = "initialise_" + dls::vers_chaine(type_deref);
				auto atome_fonction = table_fonctions[nom_fonction];

				auto params_init = kuri::tableau<Atome *>(2);
				params_init[0] = cree_charge_mem(ptr_contexte);
				params_init[1] = ptr_transtype;

				cree_appel(noeud->lexeme, atome_fonction, std::move(params_init));
			}

			break;
		}
	}

	cree_stocke_mem(val_acces_pointeur, ptr_transtype);

	if (val_acces_capacite != nullptr) {
		cree_stocke_mem(val_acces_capacite, val_nouveau_nombre_element);
	}

	return val_enfant;
}

struct DonneesComparaisonChainee {
	NoeudExpression *operande_gauche = nullptr;
	NoeudExpression *operande_droite = nullptr;
	OperateurBinaire const *op = nullptr;

	COPIE_CONSTRUCT(DonneesComparaisonChainee);
};

static void rassemble_operations_chainees(
		NoeudExpression *racine,
		kuri::tableau<DonneesComparaisonChainee> &comparaisons)
{
	auto expr_bin = static_cast<NoeudExpressionBinaire *>(racine);

	if (est_operateur_comp(expr_bin->expr1->lexeme->genre)) {
		rassemble_operations_chainees(expr_bin->expr1, comparaisons);

		auto expr_operande = static_cast<NoeudExpressionBinaire *>(expr_bin->expr1);

		auto comparaison = DonneesComparaisonChainee{};
		comparaison.operande_gauche = expr_operande->expr2;
		comparaison.operande_droite = expr_bin->expr2;
		comparaison.op = expr_bin->op;

		comparaisons.pousse(comparaison);
	}
	else {
		auto comparaison = DonneesComparaisonChainee{};
		comparaison.operande_gauche = expr_bin->expr1;
		comparaison.operande_droite = expr_bin->expr2;
		comparaison.op = expr_bin->op;

		comparaisons.pousse(comparaison);
	}
}

void ConstructriceRI::genere_ri_pour_comparaison_chainee(NoeudExpression *noeud, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
{
	auto comparaisons = kuri::tableau<DonneesComparaisonChainee>();
	rassemble_operations_chainees(noeud, comparaisons);

	auto nouveau_label = reserve_label();

	for (auto i = 0; i < comparaisons.taille; ++i) {
		auto &it = comparaisons[i];
		auto atome_gauche = genere_ri_transformee_pour_noeud(it.operande_gauche, nullptr);
		auto atome_droite = genere_ri_transformee_pour_noeud(it.operande_droite, nullptr);
		auto atome_op_bin = cree_op_binaire(noeud->type, it.op->genre, atome_gauche, atome_droite);

		cree_branche_condition(atome_op_bin, nouveau_label, label_si_faux);

		if (i < comparaisons.taille - 1) {
			insere_label(nouveau_label);

			if (i < comparaisons.taille - 2) {
				nouveau_label = reserve_label();
			}
			else {
				nouveau_label = label_si_vrai;
			}
		}
	}
}

Atome *ConstructriceRI::genere_ri_pour_declaration_structure(NoeudStruct *noeud)
{
	auto type = noeud->type;
	auto type_union = static_cast<TypeUnion *>(type);
	auto type_struct = static_cast<TypeStructure *>(type);

	if (type->genre == GenreType::UNION && type_union->deja_genere) {
		return nullptr;
	}
	else if (type->genre == GenreType::STRUCTURE && type_struct->deja_genere) {
		return nullptr;
	}

	auto ancienne_table = table_locales;
	table_locales.efface();
	acces_membres.taille = 0;
	charge_mems.taille = 0;
	auto ancienne_fonction = fonction_courante;
	fonction_courante = nullptr;
	auto ancien_nombre_instruction = nombre_instructions;
	nombre_instructions = 0;
	nombre_labels = 0;

	auto types_entrees = kuri::tableau<Type *>(2);
	types_entrees[0] = m_compilatrice.type_contexte;
	types_entrees[1] = m_compilatrice.typeuse.type_pointeur_pour(type);

	auto types_sorties = kuri::tableau<Type *>(1);
	types_sorties[0] = m_compilatrice.typeuse[TypeBase::RIEN];

	auto params = kuri::tableau<Atome *>(2);
	params[0] = cree_allocation(types_entrees[0], ident_contexte);
	params[1] = cree_allocation(types_entrees[1], IDENT_CODE("pointeur"));

	auto ptr_contexte = params[0];

	auto nom_fonction = "initialise_" + dls::vers_chaine(type);

	auto fonction = cree_fonction(nullptr, nom_fonction, std::move(params));
	fonction->type = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));
	table_fonctions.insere({ nom_fonction, fonction });
	fonction_courante = fonction;
	cree_label();

	if (type->genre == GenreType::UNION) {
		auto index_membre = 0u;
		auto pointeur_union = cree_charge_mem(fonction->params_entrees[1]);

		POUR (type_union->membres) {
			if (it.type != type_union->type_le_plus_grand) {
				index_membre += 1;
				continue;
			}

			auto valeur = static_cast<Atome *>(nullptr);

			if (it.expression_valeur_defaut) {
				valeur = genere_ri_transformee_pour_noeud(it.expression_valeur_defaut, nullptr);
			}
			else {
				valeur = genere_initialisation_defaut_pour_type(it.type);
			}

			// valeur peut être nulle pour les initialisations de tableaux fixes
			if (valeur) {
				auto pointeur = cree_acces_membre(pointeur_union, index_membre);
				cree_stocke_mem(pointeur, valeur);
			}
		}

		type_union->deja_genere = true;
	}
	else if (type->genre == GenreType::STRUCTURE) {
		auto index_membre = 0u;
		auto pointeur_struct = cree_charge_mem(fonction->params_entrees[1]);

		POUR (type_struct->membres) {
			if (it.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
				index_membre += 1;
				continue;
			}

			auto valeur = static_cast<Atome *>(nullptr);
			auto pointeur = cree_acces_membre(pointeur_struct, index_membre);

			if (it.expression_valeur_defaut) {
				valeur = genere_ri_transformee_pour_noeud(it.expression_valeur_defaut, nullptr);
			}
			else {
				if (it.type->genre == GenreType::STRUCTURE || it.type->genre == GenreType::UNION) {
					auto nom_fonction_init = "initialise_" + dls::vers_chaine(it.type);
					auto atome_fonction = table_fonctions[nom_fonction_init];

					auto params_init = kuri::tableau<Atome *>(2);
					params_init[0] = cree_charge_mem(ptr_contexte);
					params_init[1] = pointeur;

					cree_appel(noeud->lexeme, atome_fonction, std::move(params_init));
				}
				else {
					valeur = genere_initialisation_defaut_pour_type(it.type);
				}
			}

			// valeur peut être nulle pour les initialisations de tableaux fixes
			if (valeur) {
				cree_stocke_mem(pointeur, valeur);
			}

			index_membre += 1;
		}

		type_struct->deja_genere = true;
	}

	cree_retour(nullptr);

	table_locales = ancienne_table;
	fonction_courante = ancienne_fonction;
	nombre_instructions = ancien_nombre_instruction;

	return nullptr;
}

Atome *ConstructriceRI::valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident)
{
	auto decl_enum = type_enum->decl;

	auto index_membre = 0;

	POUR (decl_enum->bloc->membres) {
		if (it->ident == ident) {
			break;
		}

		index_membre += 1;
	}

	auto valeur = type_enum->membres[index_membre].valeur;
	return cree_constante_entiere(type_enum, static_cast<unsigned>(valeur));
}

Atome *ConstructriceRI::genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud)
{
	// À FAIRE(ri) : ceci ignore les espaces de noms.
	auto accede = noeud->accede;
	auto type_accede = accede->type;

	auto est_pointeur = type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE;

	while (type_accede->genre == GenreType::POINTEUR || type_accede->genre == GenreType::REFERENCE) {
		type_accede = type_dereference_pour(type_accede);
	}

	if (type_accede->genre == GenreType::TABLEAU_FIXE) {
		auto taille = static_cast<TypeTableauFixe *>(type_accede)->taille;
		return cree_constante_entiere(noeud->type, static_cast<unsigned long>(taille));
	}

	if (type_accede->genre == GenreType::ENUM || type_accede->genre == GenreType::ERREUR) {
		auto type_enum = static_cast<TypeEnum *>(type_accede);
		auto valeur_enum = type_enum->membres[noeud->index_membre].valeur;
		return cree_constante_entiere(type_enum, static_cast<unsigned>(valeur_enum));
	}

	if (noeud->type->genre == GenreType::TYPE_DE_DONNEES) {
		auto type_de_donnees = static_cast<TypeTypeDeDonnees *>(noeud->type);

		if (type_de_donnees->type_connu) {
			return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->type_connu->index_dans_table_types);
		}

		return cree_constante_entiere(m_compilatrice.typeuse.type_type_de_donnees_, type_de_donnees->index_dans_table_types);
	}

	auto type_compose = static_cast<TypeCompose *>(type_accede);
	auto &membre = type_compose->membres[noeud->index_membre];

	if (membre.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
		return genere_ri_transformee_pour_noeud(membre.expression_valeur_defaut, nullptr);
	}

	auto pointeur_accede = genere_ri_pour_noeud(accede);

	if (est_pointeur) {
		pointeur_accede = cree_charge_mem(pointeur_accede);
	}

	if (pointeur_accede->genre_atome == Atome::Genre::CONSTANTE) {
		auto initialisateur = static_cast<AtomeValeurConstante *>(pointeur_accede);
		return initialisateur->valeur.valeur_structure.pointeur[noeud->index_membre];
	}

	// dans le cas où nous avons une expression de type foo().membre, la valeur
	// foo() n'est pas un pointeur sur la pile comme pour foo.membre, donc passe
	// par une variable temporaire
	if (accede->genre_valeur == GenreValeur::DROITE) {
		auto alloc = cree_allocation(type_accede, nullptr);
		cree_stocke_mem(alloc, pointeur_accede);
		pointeur_accede = alloc;
	}

	return cree_acces_membre(pointeur_accede, noeud->index_membre);
}

Atome *ConstructriceRI::genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud)
{
	auto ptr_union = genere_ri_pour_noeud(noeud->accede);
	auto type = ptr_union->type;

	while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
		type = type_dereference_pour(type);
	}

	auto type_union = static_cast<TypeUnion *>(type);

	if (type_union->est_nonsure) {
		if (expression_gauche) {
			// ajourne l'index du membre
			auto membre_actif = cree_acces_membre(ptr_union, 1);
			cree_stocke_mem(membre_actif, cree_z32(static_cast<unsigned>(noeud->index_membre + 1)));
		}
		else {
			// vérifie l'index du membre
			auto membre_actif = cree_acces_membre_et_charge(ptr_union, 1);

			auto label_si_vrai = reserve_label();
			auto label_si_faux = reserve_label();

			auto condition = cree_op_comparaison(
						OperateurBinaire::Genre::Comp_Inegal,
						membre_actif,
						cree_z32(static_cast<unsigned>(noeud->index_membre + 1)));

			cree_branche_condition(condition, label_si_vrai, label_si_faux);
			insere_label(label_si_vrai);
			auto params = kuri::tableau<Atome *>(1);
			params[0] = cree_charge_mem(table_locales[ident_contexte]);
			cree_appel(noeud->lexeme, trouve_ou_insere_fonction(m_compilatrice.interface_kuri.decl_panique_membre_union), std::move(params));
			insere_label(label_si_faux);
		}
	}

	return cree_acces_membre(ptr_union, 0);
}

AtomeConstante *ConstructriceRI::genere_initialisation_defaut_pour_type(Type *type)
{
	switch (type->genre) {
		case GenreType::INVALIDE:
		case GenreType::REFERENCE:
		case GenreType::RIEN:
		case GenreType::POLYMORPHIQUE:
		{
			return nullptr;
		}
		case GenreType::BOOL:
		{
			return cree_constante_booleenne(false);
		}
		case GenreType::POINTEUR:
		case GenreType::FONCTION:
		{
			return cree_constante_nulle(type);
		}
		case GenreType::OCTET:
		case GenreType::ENTIER_NATUREL:
		case GenreType::ENTIER_RELATIF:
		case GenreType::TYPE_DE_DONNEES:
		{
			return cree_constante_entiere(type, 0);
		}
		case GenreType::ENTIER_CONSTANT:
		{
			return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z32], 0);
		}
		case GenreType::REEL:
		{
			if (type->taille_octet == 2) {
				return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::N16], 0);
			}

			return cree_constante_reelle(type, 0.0);
		}
		case GenreType::TABLEAU_FIXE:
		{
			// À FAIRE(ri) : initialisation défaut pour tableau fixe
			return nullptr;
		}
		case GenreType::UNION:
		{
			auto type_union = static_cast<TypeUnion *>(type);

			if (type_union->est_nonsure) {
				return genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand);
			}

			auto valeurs = kuri::tableau<AtomeConstante *>();
			valeurs.reserve(2);

			valeurs.pousse(genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand));
			valeurs.pousse(genere_initialisation_defaut_pour_type(m_compilatrice.typeuse[TypeBase::Z32]));

			return cree_constante_structure(type, std::move(valeurs));
		}
		case GenreType::CHAINE:
		case GenreType::EINI:
		case GenreType::STRUCTURE:
		case GenreType::TABLEAU_DYNAMIQUE:
		case GenreType::VARIADIQUE:
		{
			auto type_compose = static_cast<TypeCompose *>(type);
			auto valeurs = kuri::tableau<AtomeConstante *>();
			valeurs.reserve(type_compose->membres.taille);

			POUR (type_compose->membres) {
				auto valeur = genere_initialisation_defaut_pour_type(it.type);
				valeurs.pousse(valeur);
			}

			return cree_constante_structure(type, std::move(valeurs));
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);
			return cree_constante_entiere(type_enum, 0);
		}
	}

	return nullptr;
}

// Logique tirée de « Basics of Compiler Design », Torben Ægidius Mogensen
void ConstructriceRI::genere_ri_pour_condition(NoeudExpression *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
{
	auto genre_lexeme = condition->lexeme->genre;

	if (est_operateur_comp(genre_lexeme)) {
		if (condition->genre == GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE) {
			genere_ri_pour_comparaison_chainee(condition, label_si_vrai, label_si_faux);
		}
		else {
			auto valeur = genere_ri_pour_noeud(condition);
			cree_branche_condition(valeur, label_si_vrai, label_si_faux);
		}
	}
	else if (genre_lexeme == GenreLexeme::ESP_ESP) {
		auto expr_bin = static_cast<NoeudExpressionBinaire *>(condition);
		auto cond1 = expr_bin->expr1;
		auto cond2 = expr_bin->expr2;

		auto nouveau_label = reserve_label();
		genere_ri_pour_condition(cond1, nouveau_label, label_si_faux);
		insere_label(nouveau_label);
		genere_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
	}
	else if (genre_lexeme == GenreLexeme::BARRE_BARRE) {
		auto expr_bin = static_cast<NoeudExpressionBinaire *>(condition);
		auto cond1 = expr_bin->expr1;
		auto cond2 = expr_bin->expr2;

		auto nouveau_label = reserve_label();
		genere_ri_pour_condition(cond1, label_si_vrai, nouveau_label);
		insere_label(nouveau_label);
		genere_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
	}
	else if (genre_lexeme == GenreLexeme::EXCLAMATION) {
		auto expr_unaire = static_cast<NoeudExpressionUnaire *>(condition);
		genere_ri_pour_condition(expr_unaire->expr, label_si_faux, label_si_vrai);
	}
	else if (genre_lexeme == GenreLexeme::VRAI) {
		cree_branche(label_si_vrai);
	}
	else if (genre_lexeme == GenreLexeme::FAUX) {
		cree_branche(label_si_faux);
	}
	else if (condition->genre == GenreNoeud::EXPRESSION_PARENTHESE) {
		auto expr_unaire = static_cast<NoeudExpressionUnaire *>(condition);
		genere_ri_pour_condition(expr_unaire->expr, label_si_vrai, label_si_faux);
	}
	else {
		auto type_condition = condition->type;
		auto valeur = static_cast<Atome *>(nullptr);

		switch (type_condition->genre) {
			case GenreType::ENTIER_NATUREL:
			case GenreType::ENTIER_RELATIF:
			case GenreType::ENTIER_CONSTANT:
			{
				auto valeur1 = genere_ri_transformee_pour_noeud(condition, nullptr);
				auto valeur2 = cree_constante_entiere(type_condition, 0);
				valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
				break;
			}
			case GenreType::BOOL:
			{
				valeur = genere_ri_transformee_pour_noeud(condition, nullptr);
				break;
			}
			case GenreType::FONCTION:
			case GenreType::POINTEUR:
			{
				auto valeur1 = genere_ri_transformee_pour_noeud(condition, nullptr);
				auto valeur2 = cree_constante_nulle(type_condition);
				valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
				break;
			}
			case GenreType::CHAINE:
			case GenreType::TABLEAU_DYNAMIQUE:
			{
				auto pointeur = genere_ri_pour_noeud(condition);
				auto pointeur_taille = cree_acces_membre(pointeur, 1);
				auto valeur1 = cree_charge_mem(pointeur_taille);
				auto valeur2 = cree_z64(0);
				valeur = cree_op_comparaison(OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
				break;
			}
			default:
			{
				break;
			}
		}

		cree_branche_condition(valeur, label_si_vrai, label_si_faux);
	}
}

void ConstructriceRI::genere_ri_blocs_differes(NoeudBloc *bloc)
{
#if 0
	if (compilatrice.donnees_fonction->est_coroutine) {
		constructrice << "__etat->__termine_coro = 1;\n";
		constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}
#endif

	while (bloc != nullptr) {
		for (auto i = bloc->noeuds_differes.taille - 1; i >= 0; --i) {
			auto bloc_differe = bloc->noeuds_differes[i];
			bloc_differe->est_differe = false;
			genere_ri_pour_noeud(bloc_differe);
			bloc_differe->est_differe = true;
		}

		bloc = bloc->parent;
	}
}

/* À tenir synchronisé avec l'énum dans info_type.kuri
 * Nous utilisons ceci lors de la génération du code des infos types car nous ne
 * générons pas de code (ou symboles) pour les énums, mais prenons directements
 * leurs valeurs.
 */
struct IDInfoType {
	static constexpr unsigned ENTIER          = 0;
	static constexpr unsigned REEL            = 1;
	static constexpr unsigned BOOLEEN         = 2;
	static constexpr unsigned CHAINE          = 3;
	static constexpr unsigned POINTEUR        = 4;
	static constexpr unsigned STRUCTURE       = 5;
	static constexpr unsigned FONCTION        = 6;
	static constexpr unsigned TABLEAU         = 7;
	static constexpr unsigned EINI            = 8;
	static constexpr unsigned RIEN            = 9;
	static constexpr unsigned ENUM            = 10;
	static constexpr unsigned OCTET           = 11;
	static constexpr unsigned TYPE_DE_DONNEES = 12;
	static constexpr unsigned UNION           = 13;
};

AtomeConstante *ConstructriceRI::cree_info_type(Type *type)
{
	if (type->info_type != nullptr) {
		return type->info_type;
	}

	switch (type->genre) {
		case GenreType::INVALIDE:
		case GenreType::POLYMORPHIQUE:
		{
			assert(false);
			break;
		}
		case GenreType::BOOL:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::BOOLEEN, type->taille_octet);
			break;
		}
		case GenreType::OCTET:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::OCTET, type->taille_octet);
			break;
		}
		case GenreType::ENTIER_CONSTANT:
		{
			type->info_type = cree_info_type_entier(4, true);
			break;
		}
		case GenreType::ENTIER_NATUREL:
		{
			type->info_type = cree_info_type_entier(type->taille_octet, false);
			break;
		}
		case GenreType::ENTIER_RELATIF:
		{
			type->info_type = cree_info_type_entier(type->taille_octet, true);
			break;
		}
		case GenreType::REEL:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::REEL, type->taille_octet);
			break;
		}
		case GenreType::REFERENCE:
		case GenreType::POINTEUR:
		{
			/* { id, taille_en_octet type_pointé, est_référence } */

			auto type_pointeur = m_compilatrice.typeuse.type_info_type_pointeur;

			auto valeur_id = cree_z32(IDInfoType::POINTEUR);
			auto valeur_taille_octet = cree_z32(type->taille_octet);

			auto type_deref = type_dereference_pour(type);
			auto valeur_type_pointe = cree_info_type(type_deref);
			valeur_type_pointe = cree_transtype_constant(
						m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
						valeur_type_pointe);

			auto est_reference = cree_constante_booleenne(type->genre == GenreType::REFERENCE);

			auto valeurs = kuri::tableau<AtomeConstante *>(4);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = valeur_type_pointe;
			valeurs[3] = est_reference;

			auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));
			type->info_type = cree_globale(type_pointeur, initialisateur, false, true);
			break;
		}
		case GenreType::ENUM:
		case GenreType::ERREUR:
		{
			auto type_enum = static_cast<TypeEnum *>(type);

			auto type_llvm = m_compilatrice.typeuse.type_info_type_enum;

			/* { id: e32, taille_en_octet, nom: chaine, valeurs: [], membres: [], est_drapeau: bool } */

			auto valeur_id = cree_z32(IDInfoType::ENUM);
			auto valeur_taille_octet = cree_z32(type->taille_octet);
			auto est_drapeau = cree_constante_booleenne(type_enum->est_drapeau);

			auto struct_chaine = cree_chaine(type_enum->nom);

			/* création des tableaux de valeurs et de noms */

			kuri::tableau<AtomeConstante *> valeurs_enum;
			valeurs_enum.reserve(type_enum->membres.taille);

			POUR (type_enum->membres) {
				auto valeur = cree_z32(static_cast<unsigned>(it.valeur));
				valeurs_enum.pousse(valeur);
			}

			kuri::tableau<AtomeConstante *> noms_enum;
			noms_enum.reserve(type_enum->membres.taille);

			POUR (type_enum->membres) {
				auto chaine_nom = cree_chaine(it.nom);
				noms_enum.pousse(chaine_nom);
			}

			auto tableau_valeurs = cree_tableau_global(m_compilatrice.typeuse[TypeBase::Z32], std::move(valeurs_enum));
			auto tableau_noms = cree_tableau_global(m_compilatrice.typeuse[TypeBase::CHAINE], std::move(noms_enum));

			/* création de l'info type */

			auto valeurs = kuri::tableau<AtomeConstante *>(6);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = struct_chaine;
			valeurs[3] = tableau_valeurs;
			valeurs[4] = tableau_noms;
			valeurs[5] = est_drapeau;

			auto initialisateur = cree_constante_structure(type_llvm, std::move(valeurs));
			type->info_type = cree_globale(type_llvm, initialisateur, false, true);
			break;
		}
		case GenreType::UNION:
		{
			auto type_union = static_cast<TypeUnion *>(type);
			auto nom_union = dls::vue_chaine_compacte();

			if (type_union->est_anonyme) {
				nom_union = "anonyme";
			}
			else {
				nom_union = type_union->decl->lexeme->chaine;
			}

			// ------------------------------------
			// Commence par assigner une globale non-initialisée comme info type
			// pour éviter de recréer plusieurs fois le même info type.
			auto type_info_union = m_compilatrice.typeuse.type_info_type_union;

			auto globale = cree_globale(type_info_union, nullptr, false, true);
			type->info_type = globale;

			// ------------------------------------
			/* pour chaque membre cree une instance de InfoTypeMembreStructure */
			auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

			kuri::tableau<AtomeConstante *> valeurs_membres;

			POUR (type_union->membres) {
				/* { nom: chaine, info : *InfoType, décalage, drapeaux } */
				auto type_membre = it.type;

				auto info_type = cree_info_type(type_membre);
				info_type = cree_transtype_constant(
							m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
							info_type);

				auto valeur_nom = cree_chaine(it.nom);
				auto valeur_decalage = cree_z64(static_cast<uint64_t>(it.decalage));
				auto valeur_drapeaux = cree_z32(static_cast<unsigned>(it.drapeaux));

				auto valeurs = kuri::tableau<AtomeConstante *>(4);
				valeurs[0] = valeur_nom;
				valeurs[1] = info_type;
				valeurs[2] = valeur_decalage;
				valeurs[3] = valeur_drapeaux;

				auto initialisateur = cree_constante_structure(type_struct_membre, std::move(valeurs));

				/* Création d'un InfoType globale. */
				auto globale_membre = cree_globale(type_struct_membre, initialisateur, false, true);
				valeurs_membres.pousse(globale_membre);
			}

			/* id : n32
			 * taille_en_octet: z32
			 * nom: chaine
			 * membres : []InfoTypeMembreStructure
			 * type_le_plus_grand : *InfoType
			 * décalage_index : z64
			 * est_sûre: bool
			 */
			auto info_type_plus_grand = cree_info_type(type_union->type_le_plus_grand);
			info_type_plus_grand = cree_transtype_constant(
						m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
						info_type_plus_grand);

			auto valeur_id = cree_z32(IDInfoType::UNION);
			auto valeur_taille_octet = cree_z32(type->taille_octet);
			auto valeur_nom = cree_chaine(nom_union);
			auto valeur_est_sure = cree_constante_booleenne(!type_union->est_nonsure);
			auto valeur_type_le_plus_grand = info_type_plus_grand;
			auto valeur_decalage_index = cree_z64(type_union->decalage_index);

			// Pour les références à des globales, nous devons avoir un type pointeur.
			auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre);

			auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

			auto valeurs = kuri::tableau<AtomeConstante *>(7);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = valeur_nom;
			valeurs[3] = tableau_membre;
			valeurs[4] = valeur_type_le_plus_grand;
			valeurs[5] = valeur_decalage_index;
			valeurs[6] = valeur_est_sure;

			globale->initialisateur = cree_constante_structure(type_info_union, std::move(valeurs));

			break;
		}
		case GenreType::STRUCTURE:
		{
			auto type_struct = static_cast<TypeStructure *>(type);
			auto decl_struct = type_struct->decl;

			// ------------------------------------
			// Commence par assigner une globale non-initialisée comme info type
			// pour éviter de recréer plusieurs fois le même info type.
			auto type_info_struct = m_compilatrice.typeuse.type_info_type_structure;

			auto globale = cree_globale(type_info_struct, nullptr, false, true);
			type->info_type = globale;

			// ------------------------------------
			/* pour chaque membre cree une instance de InfoTypeMembreStructure */
			auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

			kuri::tableau<AtomeConstante *> valeurs_membres;

			POUR (type_struct->membres) {
				/* { nom: chaine, info : *InfoType, décalage, drapeaux } */
				auto type_membre = it.type;

				auto info_type = cree_info_type(type_membre);
				info_type = cree_transtype_constant(
							m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
							info_type);

				auto valeur_nom = cree_chaine(it.nom);
				auto valeur_decalage = cree_z64(static_cast<uint64_t>(it.decalage));
				auto valeur_drapeaux = cree_z32(static_cast<unsigned>(it.drapeaux));

				auto valeurs = kuri::tableau<AtomeConstante *>(4);
				valeurs[0] = valeur_nom;
				valeurs[1] = info_type;
				valeurs[2] = valeur_decalage;
				valeurs[3] = valeur_drapeaux;

				auto initialisateur = cree_constante_structure(type_struct_membre, std::move(valeurs));

				/* Création d'un InfoType globale. */
				auto globale_membre = cree_globale(type_struct_membre, initialisateur, false, true);
				valeurs_membres.pousse(globale_membre);
			}

			/* { id : n32, taille_en_octet, nom: chaine, membres : []InfoTypeMembreStructure } */

			auto valeur_id = cree_z32(IDInfoType::STRUCTURE);
			auto valeur_taille_octet = cree_z32(type->taille_octet);
			auto valeur_nom = cree_chaine(decl_struct->lexeme->chaine);

			// Pour les références à des globales, nous devons avoir un type pointeur.
			auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre);

			auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

			auto valeurs = kuri::tableau<AtomeConstante *>(4);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = valeur_nom;
			valeurs[3] = tableau_membre;

			globale->initialisateur = cree_constante_structure(type_info_struct, std::move(valeurs));
			break;
		}
		case GenreType::TABLEAU_DYNAMIQUE:
		case GenreType::VARIADIQUE:
		{
			/* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

			auto type_pointeur = m_compilatrice.typeuse.type_info_type_tableau;

			auto valeur_id = cree_z32(IDInfoType::TABLEAU);
			auto valeur_taille_octet = cree_z32(type->taille_octet);

			auto type_deref = type_dereference_pour(type);

			auto valeur_type_pointe = cree_info_type(type_deref);
			valeur_type_pointe = cree_transtype_constant(
						m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
						valeur_type_pointe);

			auto valeur_est_fixe = cree_constante_booleenne(false);
			auto valeur_taille_fixe = cree_z32(0);

			auto valeurs = kuri::tableau<AtomeConstante *>(5);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = valeur_type_pointe;
			valeurs[3] = valeur_est_fixe;
			valeurs[4] = valeur_taille_fixe;

			auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));

			type->info_type = cree_globale(type_pointeur, initialisateur, false, true);
			break;
		}
		case GenreType::TABLEAU_FIXE:
		{
			auto type_tableau = static_cast<TypeTableauFixe *>(type);
			/* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

			auto type_pointeur = m_compilatrice.typeuse.type_info_type_tableau;

			auto valeur_id = cree_z32(IDInfoType::TABLEAU);
			auto valeur_taille_octet = cree_z32(type->taille_octet);

			auto valeur_type_pointe = cree_info_type(type_tableau->type_pointe);
			valeur_type_pointe = cree_transtype_constant(
						m_compilatrice.typeuse.type_pointeur_pour(m_compilatrice.typeuse.type_info_type_),
						valeur_type_pointe);

			auto valeur_est_fixe = cree_constante_booleenne(true);
			auto valeur_taille_fixe = cree_z32(static_cast<size_t>(type_tableau->taille));

			auto valeurs = kuri::tableau<AtomeConstante *>(5);
			valeurs[0] = valeur_id;
			valeurs[1] = valeur_taille_octet;
			valeurs[2] = valeur_type_pointe;
			valeurs[3] = valeur_est_fixe;
			valeurs[4] = valeur_taille_fixe;

			auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));

			type->info_type = cree_globale(type_pointeur, initialisateur, false, true);
			break;
		}
		case GenreType::FONCTION:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::FONCTION, type->taille_octet);
			break;
		}
		case GenreType::EINI:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::EINI, type->taille_octet);
			break;
		}
		case GenreType::RIEN:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::RIEN, type->taille_octet);
			break;
		}
		case GenreType::CHAINE:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::CHAINE, type->taille_octet);
			break;
		}
		case GenreType::TYPE_DE_DONNEES:
		{
			type->info_type = cree_info_type_defaut(IDInfoType::TYPE_DE_DONNEES, type->taille_octet);
			break;
		}
	}

	return type->info_type;
}

AtomeConstante *ConstructriceRI::cree_info_type_defaut(unsigned index, unsigned taille_octet)
{
	auto valeur_id = cree_z32(index);
	auto valeur_taille_octet = cree_z32(taille_octet);

	auto valeurs = kuri::tableau<AtomeConstante *>(2);
	valeurs[0] = valeur_id;
	valeurs[1] = valeur_taille_octet;

	auto initialisateur = cree_constante_structure(m_compilatrice.typeuse.type_info_type_, std::move(valeurs));

	return cree_globale(m_compilatrice.typeuse.type_info_type_, initialisateur, false, true);
}

AtomeConstante *ConstructriceRI::cree_info_type_entier(unsigned taille_octet, bool est_relatif)
{
	auto valeur_id = cree_z32(IDInfoType::ENTIER);
	auto valeur_taille_octet = cree_z32(taille_octet);

	auto valeurs = kuri::tableau<AtomeConstante *>(3);
	valeurs[0] = valeur_id;
	valeurs[1] = valeur_taille_octet;
	valeurs[2] = cree_constante_booleenne(est_relatif);

	auto type_info_entier = m_compilatrice.typeuse.type_info_type_entier;
	auto initialisateur = cree_constante_structure(type_info_entier, std::move(valeurs));

	return cree_globale(type_info_entier, initialisateur, false, true);
}

Atome *ConstructriceRI::genere_ri_pour_position_code_source(NoeudExpression *noeud)
{
	if (m_noeud_pour_appel) {
		noeud = m_noeud_pour_appel;
	}

	auto type_position = m_compilatrice.typeuse.type_position_code_source;

	auto alloc = cree_allocation(type_position, nullptr);

	// fichier
	auto fichier = m_compilatrice.fichiers[noeud->lexeme->fichier];
	auto chaine_nom_fichier = cree_chaine(fichier->nom);
	auto ptr_fichier = cree_acces_membre(alloc, 0);
	cree_stocke_mem(ptr_fichier, chaine_nom_fichier);

	// fonction
	auto nom_fonction = dls::vue_chaine_compacte("");

	if (fonction_courante != nullptr) {
		nom_fonction = fonction_courante->nom;
	}

	auto chaine_fonction = cree_chaine(nom_fonction);
	auto ptr_fonction = cree_acces_membre(alloc, 1);
	cree_stocke_mem(ptr_fonction, chaine_fonction);

	// ligne
	auto pos = position_lexeme(*noeud->lexeme);
	auto ligne = cree_z32(static_cast<unsigned>(pos.numero_ligne));
	auto ptr_ligne = cree_acces_membre(alloc, 2);
	cree_stocke_mem(ptr_ligne, ligne);

	// colonne
	auto colonne = cree_z32(static_cast<unsigned>(pos.pos));
	auto ptr_colonne = cree_acces_membre(alloc, 3);
	cree_stocke_mem(ptr_colonne, colonne);

	return alloc;
}

void ConstructriceRI::genere_ri_pour_coroutine(NoeudDeclarationFonction *decl)
{
#if 0
	compilatrice.commence_fonction(decl);

	/* Crée fonction */
	auto nom_fonction = decl->nom_broye;
	auto nom_type_coro = "__etat_coro" + nom_fonction;

	/* Déclare la structure d'état de la coroutine. */
	constructrice << "typedef struct " << nom_type_coro << " {\n";
	constructrice << "pthread_mutex_t mutex_boucle;\n";
	constructrice << "pthread_cond_t cond_boucle;\n";
	constructrice << "pthread_mutex_t mutex_coro;\n";
	constructrice << "pthread_cond_t cond_coro;\n";
	constructrice << "bool __termine_coro;\n";
	constructrice << "ContexteProgramme contexte;\n";

	auto idx_ret = 0l;
	POUR (decl->type_fonc->types_sorties) {
		auto &nom_ret = decl->noms_retours[idx_ret++];
		constructrice.declare_variable(it, nom_ret, "");
	}

	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);
		constructrice.declare_variable(it->type, nom_broye, "");
	}

	constructrice << " } " << nom_type_coro << ";\n";

	/* Déclare la fonction. */
	constructrice << "static void *" << nom_fonction << "(\nvoid *data)\n";
	constructrice << "{\n";
	constructrice << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
	constructrice << "ContexteProgramme contexte = __etat->contexte;\n";

	/* déclare les paramètres. */
	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident->nom);
		constructrice.declare_variable(it->type, nom_broye, "__etat->" + nom_broye);
	}

	/* Crée code pour le bloc. */
	genere_code_C(decl->bloc, constructrice, compilatrice, false);

	if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
		genere_code_extra_pre_retour(decl->bloc, compilatrice, constructrice);
	}

	constructrice << "}\n";

	compilatrice.termine_fonction();
#endif
}

void ConstructriceRI::genere_ri_pour_retiens(NoeudExpression */*noeud*/)
{
#if 0
	auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
	auto df = compilatrice.donnees_fonction;
	auto enfant = inst->expr;

	constructrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

	auto feuilles = dls::tablet<NoeudExpression *, 10>{};
	rassemble_feuilles(enfant, feuilles);

	for (auto i = 0l; i < feuilles.taille(); ++i) {
		auto f = feuilles[i];

		genere_code_C(f, constructrice, compilatrice, true);

		constructrice << "__etat->" << df->noms_retours[i] << " = ";
		constructrice << f->chaine_calculee();
		constructrice << ";\n";
	}

	constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";
#endif
}

Atome *ConstructriceRI::converti_vers_tableau_dyn(Atome *pointeur_tableau_fixe, TypeTableauFixe *type_tableau_fixe, Atome *place)
{
	auto alloc_tableau_dyn = place;

	if (alloc_tableau_dyn == nullptr) {
		auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);
		alloc_tableau_dyn = cree_allocation(type_tableau_dyn, nullptr);
	}

	auto ptr_pointeur_donnees = cree_acces_membre(alloc_tableau_dyn, 0);
	auto premier_elem = cree_acces_index(pointeur_tableau_fixe, cree_z64(0ul));
	cree_stocke_mem(ptr_pointeur_donnees, premier_elem);

	auto ptr_taille = cree_acces_membre(alloc_tableau_dyn, 1);
	auto constante = cree_z64(uint64_t(type_tableau_fixe->taille));
	cree_stocke_mem(ptr_taille, constante);

	return alloc_tableau_dyn;
}

AtomeConstante *ConstructriceRI::cree_chaine(dls::vue_chaine_compacte const &chaine)
{
	auto iter = table_chaines.trouve(chaine);

	if (iter != table_chaines.fin()) {
		return iter->second;
	}

	auto type_chaine = m_compilatrice.typeuse.type_chaine;

	auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(m_compilatrice.typeuse[TypeBase::Z8], chaine.taille());
	auto tableau = cree_constante_tableau_donnees_constantes(type_tableau, const_cast<char *>(chaine.pointeur()), chaine.taille());

	auto globale_tableau = cree_globale(type_tableau, tableau, false, true);
	auto pointeur_chaine = cree_acces_index_constant(globale_tableau, cree_z64(0));
	auto taille_chaine = cree_z64(static_cast<unsigned long>(chaine.taille()));

	auto membres = kuri::tableau<AtomeConstante *>(2);
	membres[0] = pointeur_chaine;
	membres[1] = taille_chaine;

	auto constante_chaine = cree_constante_structure(type_chaine, std::move(membres));

	table_chaines.insere({ chaine, constante_chaine });

	return constante_chaine;
}

void ConstructriceRI::genere_ri_pour_fonction_main()
{
	nombre_labels = 0;
	nombre_instructions = 0;

	// déclare une fonction de type int(int, char**) appelée main
	auto type_int = m_compilatrice.typeuse[TypeBase::Z32];
	auto type_argc = type_int;

	auto type_argv = m_compilatrice.typeuse[TypeBase::Z8];
	type_argv = m_compilatrice.typeuse.type_pointeur_pour(type_argv);
	type_argv = m_compilatrice.typeuse.type_pointeur_pour(type_argv);

	auto types_entrees = kuri::tableau<Type *>(2);
	types_entrees[0] = type_argc;
	types_entrees[1] = type_argv;

	auto types_sorties = kuri::tableau<Type *>(1);
	types_sorties[0] = type_int;

	auto type_fonction = m_compilatrice.typeuse.type_fonction(std::move(types_entrees), std::move(types_sorties));

	auto ident_argc = m_compilatrice.table_identifiants.identifiant_pour_chaine("argc");
	auto ident_argv = m_compilatrice.table_identifiants.identifiant_pour_chaine("argv");

	auto alloc_argc = cree_allocation(type_argc, ident_argc);
	auto alloc_argv = cree_allocation(type_argv, ident_argv);

	auto params = kuri::tableau<Atome *>(2);
	params[0] = alloc_argc;
	params[1] = alloc_argv;

	auto fonction = cree_fonction(nullptr, "main", std::move(params));
	fonction->type = type_fonction;
	fonction->sanstrace = true;

	fonction_courante = fonction;

	cree_label();

	auto ident_ARGC = IDENT_CODE("__ARGC");
	auto ident_ARGV = IDENT_CODE("__ARGV");

	auto valeur_ARGC = table_globales[ident_ARGC];
	auto valeur_ARGV = table_globales[ident_ARGV];

	if (valeur_ARGC) {
		auto charge_argc = cree_charge_mem(alloc_argc);
		cree_stocke_mem(valeur_ARGC, charge_argc);
	}

	if (valeur_ARGV) {
		auto charge_argv = cree_charge_mem(alloc_argv);
		cree_stocke_mem(valeur_ARGV, charge_argv);
	}

	auto assigne_membre = [this](Atome *structure, unsigned index, Atome *valeur)
	{
		auto ptr = cree_acces_membre(structure, index);
		cree_stocke_mem(ptr, valeur);
	};

	// ----------------------------------
	// création de l'information trace d'appel
	auto type_info_trace_appel = m_compilatrice.typeuse.type_info_fonction_trace_appel;
	auto alloc_info_trace_appel = cree_allocation(type_info_trace_appel, IDENT_CODE("mon_info"));
	assigne_membre(alloc_info_trace_appel, trouve_index_membre(type_info_trace_appel, "nom"), cree_chaine("main"));
	assigne_membre(alloc_info_trace_appel, trouve_index_membre(type_info_trace_appel, "fichier"), cree_chaine("???"));
	assigne_membre(alloc_info_trace_appel, trouve_index_membre(type_info_trace_appel, "adresse"), cree_transtype(m_compilatrice.typeuse[TypeBase::PTR_RIEN], fonction));

	// ----------------------------------
	// création de la trace d'appel
	auto type_trace_appel = m_compilatrice.typeuse.type_trace_appel;
	auto alloc_trace = cree_allocation(type_trace_appel, IDENT_CODE("ma_trace"));
	cree_stocke_mem(alloc_trace, genere_initialisation_defaut_pour_type(type_trace_appel));
	assigne_membre(alloc_trace, trouve_index_membre(type_trace_appel, "info_fonction"), alloc_info_trace_appel);

	// ----------------------------------
	// création du stockage temporaire
	auto ident_stock_temp = IDENT_CODE("STOCKAGE_TEMPORAIRE");
	auto type_tabl_stock_temp = m_compilatrice.typeuse[TypeBase::OCTET];
	type_tabl_stock_temp = m_compilatrice.typeuse.type_tableau_fixe(type_tabl_stock_temp, 16384);
	auto tabl_stock_temp = cree_globale(type_tabl_stock_temp, nullptr, false, false);
	tabl_stock_temp->ident = ident_stock_temp;

	auto type_stock_temp = m_compilatrice.typeuse.type_stockage_temporaire;

	auto alloc_stocke_temp = cree_allocation(type_stock_temp, IDENT_CODE("stockage_temporaire"));
	auto ptr_tabl_stock_temp = cree_acces_index(tabl_stock_temp, cree_z64(0));

	assigne_membre(alloc_stocke_temp, trouve_index_membre(type_stock_temp, "données"), ptr_tabl_stock_temp);
	assigne_membre(alloc_stocke_temp, trouve_index_membre(type_stock_temp, "taille"), cree_z32(16384));
	assigne_membre(alloc_stocke_temp, trouve_index_membre(type_stock_temp, "occupé"), cree_z32(0));
	assigne_membre(alloc_stocke_temp, trouve_index_membre(type_stock_temp, "occupation_maximale"), cree_z32(0));

	// ----------------------------------
	// création de l'allocatrice de base
	auto type_base_alloc = m_compilatrice.typeuse.type_base_allocatrice;
	auto alloc_base_alloc = cree_allocation(type_base_alloc, IDENT_CODE("base_allocatrice"));

	// ----------------------------------
	// construit le contexte du programme

	auto alloc_contexte = cree_allocation(m_compilatrice.type_contexte, ident_contexte);

	// À FAIRE : la trace d'appel est réinitialisée après l'initialisation
	assigne_membre(alloc_contexte, trouve_index_membre(m_compilatrice.type_contexte, "trace_appel"), alloc_trace);

	{
		auto nom_fonction_init = "initialise_" + dls::vers_chaine(m_compilatrice.type_contexte);
		auto fonction_init = table_fonctions[nom_fonction_init];

		auto params_init = kuri::tableau<Atome *>(2);
		params_init[0] = cree_charge_mem(alloc_contexte);
		params_init[1] = alloc_contexte;

		static Lexeme lexeme_appel_init = { "initialise_contexte", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };

		cree_appel(&lexeme_appel_init, fonction_init, std::move(params_init));
	}

	// après avoir appelé la fonction d'initialisation il nous reste à
	// manuellement initialiser les membres suivants
	// - données allocatrice
	// - stockage temporaire
	// - trace appel
	assigne_membre(alloc_contexte, trouve_index_membre(m_compilatrice.type_contexte, "données_allocatrice"), alloc_base_alloc);
	assigne_membre(alloc_contexte, trouve_index_membre(m_compilatrice.type_contexte, "stockage_temporaire"), alloc_stocke_temp);
	assigne_membre(alloc_contexte, trouve_index_membre(m_compilatrice.type_contexte, "trace_appel"), alloc_trace);

	// ----------------------------------
	// initialise l'allocatrice défaut
	{
		auto nom_fonction_init = "initialise_" + dls::vers_chaine(type_base_alloc);
		auto fonction_init = table_fonctions[nom_fonction_init];

		auto params_init = kuri::tableau<Atome *>(2);
		params_init[0] = cree_charge_mem(alloc_contexte);
		params_init[1] = alloc_base_alloc;

		static Lexeme lexeme_appel_init = { "initialise_alloc", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };

		cree_appel(&lexeme_appel_init, fonction_init, std::move(params_init));
	}

	// ----------------------------------
	// constructeur des valeurs globales
	table_locales[ident_contexte] = alloc_contexte;

	POUR (this->constructeurs_globaux) {
		genere_ri_transformee_pour_noeud(it.second, it.first);
	}

	// ----------------------------------
	// construit l'info pour l'appel

	auto type_info_appel = m_compilatrice.typeuse.type_info_appel_trace_appel;
	auto alloc_info_appel = cree_allocation(type_info_appel, IDENT_CODE("info_appel"));
	assigne_membre(alloc_info_appel, trouve_index_membre(type_info_appel, "ligne"), cree_z32(1));
	assigne_membre(alloc_info_appel, trouve_index_membre(type_info_appel, "colonne"), cree_z32(0));
	assigne_membre(alloc_info_appel, trouve_index_membre(type_info_appel, "texte"), cree_chaine("principale(contexte);"));

	assigne_membre(alloc_trace, trouve_index_membre(type_trace_appel, "info_appel"), alloc_info_appel);

	// ----------------------------------
	// appel notre fonction principale en passant le contexte et le tableau
	auto fonc_princ = table_fonctions["principale"];

	auto params_principale = kuri::tableau<Atome *>(1);
	params_principale[0] = cree_charge_mem(alloc_contexte);

	static Lexeme lexeme_appel_principale = { "principale", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0 };

	auto valeur_princ = cree_appel(&lexeme_appel_principale, fonc_princ, std::move(params_principale));

	// return
	cree_retour(valeur_princ);
}

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

#pragma once

#include "instructions.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tablet.hh"

struct ContexteGenerationCode;
struct NoeudDependance;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionBinaire;
struct NoeudPour;
struct NoeudStruct;
struct NoeudTente;

template <typename T0, typename T1, typename T2>
struct triplet {
	T0 t0;
	T1 t1;
	T2 t2;
};

struct ConstructriceRI {
private:
	kuri::tableau<AtomeFonction *> atomes_fonction{};
	kuri::tableau<AtomeConstante *> atomes_constante{};
	kuri::tableau<AtomeGlobale *> atomes_globale{};
	kuri::tableau<InstructionAllocation *> insts_allocation{};
	kuri::tableau<InstructionAppel *> insts_appel{};
	kuri::tableau<InstructionBranche *> insts_branche{};
	kuri::tableau<InstructionBrancheCondition *> insts_branche_condition{};
	kuri::tableau<InstructionChargeMem *> insts_charge_memoire{};
	kuri::tableau<InstructionLabel *> insts_label{};
	kuri::tableau<InstructionOpBinaire *> insts_opbinaire{};
	kuri::tableau<InstructionOpUnaire *> insts_opunaire{};
	kuri::tableau<InstructionRetour *> insts_retour{};
	kuri::tableau<InstructionStockeMem *> insts_stocke_memoire{};
	kuri::tableau<InstructionAccedeIndex *> insts_accede_index{};
	kuri::tableau<InstructionAccedeMembre *> insts_accede_membre{};
	kuri::tableau<InstructionTranstype *> insts_transtype{};

	ContexteGenerationCode &m_contexte;

	AtomeFonction *fonction_courante = nullptr;

	int nombre_labels = 0;
	int nombre_instructions = 0;

	IdentifiantCode *ident_contexte = nullptr;

	dls::dico<IdentifiantCode *, Atome *> table_locales{};
	dls::dico<IdentifiantCode *, AtomeGlobale *> table_globales{};
	dls::dico<dls::chaine, AtomeFonction *> table_fonctions{};

	dls::tablet<triplet<IdentifiantCode *, InstructionLabel *, InstructionLabel *>, 12> insts_continue_arrete{};

public:
	// stocke les atomes des fonctions et des variables globales
	kuri::tableau<Atome *> programme{};

	ConstructriceRI(ContexteGenerationCode &contexte);

	COPIE_CONSTRUCT(ConstructriceRI);

	~ConstructriceRI();

	void genere_ri();

	void imprime_programme() const;

	size_t memoire_utilisee() const;

private:
	AtomeFonction *cree_fonction(dls::chaine const &nom, kuri::tableau<Atome *> &&params);
	AtomeConstante *cree_constante_booleenne(Type *type, bool valeur);
	AtomeConstante *cree_constante_caractere(Type *type, unsigned long long valeur);
	AtomeConstante *cree_constante_chaine(Type *type, const kuri::chaine &chaine);
	AtomeConstante *cree_constante_entiere(Type *type, unsigned long long valeur);
	AtomeConstante *cree_constante_nulle(Type *type);
	AtomeConstante *cree_constante_reelle(Type *type, double valeur);
	AtomeConstante *cree_constante_structure(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
	AtomeGlobale *cree_globale(Type *type, AtomeConstante *initialisateur);

	InstructionAllocation *cree_allocation(Type *type, IdentifiantCode *ident);
	InstructionBranche *cree_branche(InstructionLabel *label);
	InstructionBrancheCondition *cree_branche_condition(Atome *valeur, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	InstructionLabel *cree_label();
	InstructionLabel *reserve_label();
	void insere_label(InstructionLabel *label);
	InstructionRetour *cree_retour(Atome *valeur);
	InstructionStockeMem *cree_stocke_mem(Type *type, Instruction *ou, Atome *valeur);
	InstructionChargeMem *cree_charge_mem(Type *type, Atome *ou);
	InstructionAppel *cree_appel(Type *type, Atome *appele, kuri::tableau<Atome *> &&args);

	InstructionOpUnaire *cree_op_unaire(Type *type, OperateurUnaire::Genre op, Atome *valeur);
	InstructionOpBinaire *cree_op_binaire(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite);

	InstructionAccedeIndex *cree_acces_index(Type *type, Atome *accede, Atome *index);
	InstructionAccedeMembre *cree_acces_membre(Type *type, Atome *accede, Atome *index);

	InstructionTranstype *cree_transtype(Type *type, Atome *valeur);

	void empile_controle_boucle(IdentifiantCode *ident, InstructionLabel *label_continue, InstructionLabel *label_arrete);
	void depile_controle_boucle();

	Atome *genere_ri_pour_noeud(NoeudExpression *noeud);
	Atome *genere_ri_pour_expression_droite(NoeudExpression *noeud);
	Atome *genere_ri_transformee_pour_noeud(NoeudExpression *noeud);
	Atome *genere_ri_pour_discr(NoeudDiscr *noeud);
	Atome *genere_ri_pour_tente(NoeudTente *noeud);
	Atome *genere_ri_pour_boucle_pour(NoeudPour *noeud);
	Atome *genere_ri_pour_logement(Type *type,
								   int mode,
								   NoeudExpression *b,
								   NoeudExpression *variable,
								   NoeudExpression *expression,
								   NoeudExpression *bloc_sinon);
	Atome *genere_ri_pour_comparaison_chainee(NoeudExpression *noeud);
	Atome *genere_ri_pour_declaration_structure(NoeudStruct *noeud);
	Atome *genere_ri_pour_acces_membre(NoeudExpressionBinaire *noeud);
	Atome *genere_ri_pour_acces_membre_union(NoeudExpressionBinaire *noeud);
	AtomeConstante *genere_initialisation_defaut_pour_type(Type *type);

	void traverse_graphe(NoeudDependance *racine);

	void imprime_instruction(Instruction const *inst, std::ostream &os) const;
};

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

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tablet.hh"
#include "biblinternes/structures/tuples.hh"

struct Compilatrice;
struct NoeudBloc;
struct NoeudDirectiveExecution;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudExpressionMembre;
struct NoeudPour;
struct NoeudStruct;
struct NoeudTente;
struct TypeEnum;
struct TypeTableauFixe;

struct ConstructriceRI {
private:
	tableau_page<AtomeValeurConstante> atomes_constante{};
	tableau_page<Instruction> insts_simples{};
	tableau_page<InstructionAllocation> insts_allocation{};
	tableau_page<InstructionAppel> insts_appel{};
	tableau_page<InstructionBranche> insts_branche{};
	tableau_page<InstructionBrancheCondition> insts_branche_condition{};
	tableau_page<InstructionChargeMem> insts_charge_memoire{};
	tableau_page<InstructionLabel> insts_label{};
	tableau_page<InstructionOpBinaire> insts_opbinaire{};
	tableau_page<InstructionOpUnaire> insts_opunaire{};
	tableau_page<InstructionRetour> insts_retour{};
	tableau_page<InstructionStockeMem> insts_stocke_memoire{};
	tableau_page<InstructionAccedeIndex> insts_accede_index{};
	tableau_page<InstructionAccedeMembre> insts_accede_membre{};
	tableau_page<InstructionTranstype> insts_transtype{};
	tableau_page<TranstypeConstant> transtypes_constants{};
	tableau_page<OpBinaireConstant> op_binaires_constants{};
	tableau_page<OpUnaireConstant> op_unaires_constants{};
	tableau_page<AccedeIndexConstant> accede_index_constants{};

	Compilatrice &m_compilatrice;

	kuri::tableau<InstructionAccedeMembre *> acces_membres{};
	kuri::tableau<InstructionChargeMem *> charge_mems{};

	int nombre_labels = 0;

	NoeudExpressionAppel *m_noeud_pour_appel = nullptr;

	dls::dico<IdentifiantCode *, Atome *> table_locales{};

	dls::tablet<dls::triplet<IdentifiantCode *, InstructionLabel *, InstructionLabel *>, 12> insts_continue_arrete{};

	bool expression_gauche = true;

	EspaceDeTravail *m_espace = nullptr;

	/* cette pile est utilisée pour stocker les valeurs des noeuds, quand nous
	 * appelons les genere_ri_*, il faut dépiler la valeur que nous désirons, si
	 * nous en désirons une */
	dls::tablet<Atome *, 8> m_pile{};

public:
	AtomeFonction *fonction_courante = nullptr;

	double temps_generation = 0.0;

	ConstructriceRI(Compilatrice &compilatrice);

	COPIE_CONSTRUCT(ConstructriceRI);

	~ConstructriceRI();

	void genere_ri_pour_noeud(EspaceDeTravail *espace, NoeudExpression *noeud);
	void genere_ri_pour_fonction_metaprogramme(EspaceDeTravail *espace, NoeudDirectiveExecution *noeud);
	AtomeFonction *genere_ri_pour_fonction_main(EspaceDeTravail *espace);
	AtomeFonction *genere_fonction_init_globales_et_appel(EspaceDeTravail *espace, const dls::tableau<AtomeGlobale *> &globales, AtomeFonction *fonction_pour);

	void imprime_programme(EspaceDeTravail *espace) const;

	long memoire_utilisee() const;

	Compilatrice &compilatrice() const
	{
		return m_compilatrice;
	}

	InstructionAllocation *cree_allocation(Type *type, IdentifiantCode *ident, bool cree_seulement = false);


private:
	AtomeFonction *genere_fonction_init_globales_et_appel(const dls::tableau<AtomeGlobale *> &globales, AtomeFonction *fonction_pour);

	AtomeConstante *cree_constante_booleenne(bool valeur);
	AtomeConstante *cree_constante_caractere(Type *type, unsigned long long valeur);
	AtomeConstante *cree_constante_entiere(Type *type, unsigned long long valeur);
	AtomeConstante *cree_constante_type(Type *pointeur_type);
	AtomeConstante *cree_z32(unsigned long long valeur);
	AtomeConstante *cree_z64(unsigned long long valeur);
	AtomeConstante *cree_constante_nulle(Type *type);
	AtomeConstante *cree_constante_reelle(Type *type, double valeur);
	AtomeConstante *cree_constante_structure(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
	AtomeConstante *cree_constante_tableau_fixe(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
	AtomeConstante *cree_constante_tableau_donnees_constantes(Type *type, kuri::tableau<char> &&donnees_constantes);
	AtomeConstante *cree_constante_tableau_donnees_constantes(Type *type, char *pointeur, long taille);
	AtomeGlobale *cree_globale(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante);
	AtomeConstante *cree_tableau_global(Type *type, kuri::tableau<AtomeConstante *> &&valeurs);
	AtomeConstante *cree_tableau_global(AtomeConstante *tableau_fixe);

	InstructionBranche *cree_branche(InstructionLabel *label, bool cree_seulement = false);
	InstructionBrancheCondition *cree_branche_condition(Atome *valeur, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	InstructionLabel *cree_label();
	InstructionLabel *reserve_label();
	void insere_label(InstructionLabel *label);
	InstructionRetour *cree_retour(Atome *valeur);
	InstructionStockeMem *cree_stocke_mem(Atome *ou, Atome *valeur, bool cree_seulement = false);
	InstructionChargeMem *cree_charge_mem(Atome *ou);
	InstructionAppel *cree_appel(Lexeme const *lexeme, Atome *appele);
	InstructionAppel *cree_appel(Lexeme const *lexeme, Atome *appele, kuri::tableau<Atome *> &&args);

	InstructionOpUnaire *cree_op_unaire(Type *type, OperateurUnaire::Genre op, Atome *valeur);
	InstructionOpBinaire *cree_op_binaire(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite);
	InstructionOpBinaire *cree_op_comparaison(OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite);

	InstructionAccedeIndex *cree_acces_index(Atome *accede, Atome *index);
	InstructionAccedeMembre *cree_acces_membre(Atome *accede, long index);
	Instruction *cree_acces_membre_et_charge(Atome *accede, long index);

	InstructionTranstype *cree_transtype(Type *type, Atome *valeur, TypeTranstypage op);

	TranstypeConstant *cree_transtype_constant(Type *type, AtomeConstante *valeur);
	OpUnaireConstant *cree_op_unaire_constant(Type *type, OperateurUnaire::Genre op, AtomeConstante *valeur);
	OpBinaireConstant *cree_op_binaire_constant(Type *type, OperateurBinaire::Genre op, AtomeConstante *valeur_gauche, AtomeConstante *valeur_droite);
	OpBinaireConstant *cree_op_comparaison_constant(OperateurBinaire::Genre op, AtomeConstante *valeur_gauche, AtomeConstante *valeur_droite);
	AccedeIndexConstant *cree_acces_index_constant(AtomeConstante *accede, AtomeConstante *index);

	void empile_controle_boucle(IdentifiantCode *ident, InstructionLabel *label_continue, InstructionLabel *label_arrete);
	void depile_controle_boucle();

	void genere_ri_pour_noeud(NoeudExpression *noeud);
	void genere_ri_pour_fonction_metaprogramme(NoeudDirectiveExecution *noeud);
	AtomeFonction *genere_ri_pour_fonction_main();
	Atome *genere_ri_pour_creation_contexte(AtomeFonction *fonction);
	void genere_ri_pour_expression_droite(NoeudExpression *noeud);
	void genere_ri_transformee_pour_noeud(NoeudExpression *noeud, Atome *place);
	void genere_ri_pour_discr(NoeudDiscr *noeud);
	void genere_ri_pour_tente(NoeudTente *noeud);
	void genere_ri_pour_boucle_pour(NoeudPour *noeud);
	void genere_ri_pour_logement(Type *type,
								   int mode,
								   NoeudExpression *b,
								   NoeudExpression *variable,
								   NoeudExpression *expression,
								   NoeudExpression *bloc_sinon);
	void genere_ri_pour_comparaison_chainee(NoeudExpression *noeud, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	void genere_ri_pour_declaration_structure(NoeudStruct *noeud);
	void genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud);
	void genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud);
	AtomeConstante *genere_initialisation_defaut_pour_type(Type *type);
	void genere_ri_pour_condition(NoeudExpression *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	void genere_ri_pour_expression_logique(NoeudExpression *noeud, Atome *place);
	void genere_ri_blocs_differes(NoeudBloc *bloc);
	void genere_ri_pour_position_code_source(NoeudExpression *noeud);

	void genere_ri_pour_coroutine(NoeudDeclarationCorpsFonction *noeud);
	void genere_ri_pour_retiens(NoeudExpression *noeud);

	AtomeConstante *cree_info_type(Type *type);
	AtomeConstante *cree_info_type_defaut(unsigned index, unsigned taille_octet);
	AtomeConstante *cree_info_type_entier(unsigned taille_octet, bool est_relatif);

	Atome *converti_vers_tableau_dyn(Atome *pointeur_tableau_fixe, TypeTableauFixe *type_tableau_fixe, Atome *place);

	AtomeConstante *cree_chaine(dls::vue_chaine_compacte const &chaine);

	void imprime_instruction(Instruction const *inst, std::ostream &os) const;
	Atome *valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident);
	void cree_incrementation_valeur(Type *type, Atome *valeur);

	void empile_valeur(Atome *valeur);
	Atome *depile_valeur();

	friend void enligne_fonctions(ConstructriceRI &constructrice, AtomeFonction *atome_fonc);
	friend void performe_enlignage(
			ConstructriceRI &constructrice,
			kuri::tableau<Instruction *> &nouvelles_instructions,
			kuri::tableau<Instruction *> const &instructions,
			AtomeFonction *fonction_appelee,
			kuri::tableau<Atome *> const &arguments, int &nombre_labels,
			InstructionLabel *label_post,
			InstructionAllocation *adresse_retour);

	friend Atome *copie_atome(ConstructriceRI &constructrice, Atome *atome);
};

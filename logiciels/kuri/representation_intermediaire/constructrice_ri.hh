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

struct Compilatrice;
struct NoeudBloc;
struct NoeudDependance;
struct NoeudDirectiveExecution;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudExpressionBinaire;
struct NoeudExpressionMembre;
struct NoeudPour;
struct NoeudStruct;
struct NoeudTente;
struct TypeEnum;
struct TypeTableauFixe;

template <typename T0, typename T1, typename T2>
struct triplet {
	T0 t0;
	T1 t1;
	T2 t2;
};

struct ConstructriceRI {
private:
	tableau_page<AtomeFonction> atomes_fonction{};
	tableau_page<AtomeValeurConstante> atomes_constante{};
	tableau_page<AtomeGlobale> atomes_globale{};
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

	AtomeFonction *fonction_courante = nullptr;
	kuri::tableau<InstructionAccedeMembre *> acces_membres{};
	kuri::tableau<InstructionChargeMem *> charge_mems{};

	int nombre_labels = 0;
	int nombre_instructions = 0;

	NoeudExpressionAppel *m_noeud_pour_appel = nullptr;

	dls::dico<IdentifiantCode *, Atome *> table_locales{};
	using TypeDicoGlobales = dls::dico<IdentifiantCode *, AtomeGlobale *>;
	dls::outils::Synchrone<TypeDicoGlobales> table_globales{};
	dls::dico<dls::chaine, AtomeConstante *> table_chaines{};

	dls::tablet<triplet<IdentifiantCode *, InstructionLabel *, InstructionLabel *>, 12> insts_continue_arrete{};

	dls::tableau<std::pair<AtomeGlobale *, NoeudExpression *>> constructeurs_globaux{};

	bool expression_gauche = true;

public:
	using TypeDicoFonction = dls::dico<dls::chaine, AtomeFonction *>;
	dls::outils::Synchrone<TypeDicoFonction> table_fonctions{};

	// stocke les atomes des fonctions et des variables globales
	kuri::tableau<Atome *> globales{};
	kuri::tableau<Atome *> fonctions{};

	double temps_generation = 0.0;

	ConstructriceRI(Compilatrice &compilatrice);

	COPIE_CONSTRUCT(ConstructriceRI);

	~ConstructriceRI();

	Atome *genere_ri_pour_noeud(NoeudExpression *noeud);
	void genere_ri_pour_fonction_metaprogramme(NoeudDirectiveExecution *noeud);
	AtomeFonction *genere_ri_pour_fonction_main();

	void imprime_programme() const;
	void imprime_fonction(AtomeFonction *atome_fonc, std::ostream &os) const;

	size_t memoire_utilisee() const;

	Compilatrice &compilatrice() const
	{
		return m_compilatrice;
	}

	void construit_table_types();

private:
	void cree_interface_programme();

	AtomeFonction *cree_fonction(Lexeme const *lexeme, dls::chaine const &nom);
	AtomeFonction *cree_fonction(Lexeme const *lexeme, dls::chaine const &nom, kuri::tableau<Atome *> &&params);
	AtomeFonction *trouve_ou_insere_fonction(NoeudDeclarationFonction const *decl);

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

	InstructionAllocation *cree_allocation(Type *type, IdentifiantCode *ident);
	InstructionBranche *cree_branche(InstructionLabel *label);
	InstructionBrancheCondition *cree_branche_condition(Atome *valeur, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	InstructionLabel *cree_label();
	InstructionLabel *reserve_label();
	void insere_label(InstructionLabel *label);
	InstructionRetour *cree_retour(Atome *valeur);
	InstructionStockeMem *cree_stocke_mem(Atome *ou, Atome *valeur);
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

	Atome *genere_ri_pour_creation_contexte(AtomeFonction *fonction);
	Atome *genere_ri_pour_expression_droite(NoeudExpression *noeud);
	Atome *genere_ri_transformee_pour_noeud(NoeudExpression *noeud, Atome *place);
	Atome *genere_ri_pour_discr(NoeudDiscr *noeud);
	Atome *genere_ri_pour_tente(NoeudTente *noeud);
	Atome *genere_ri_pour_boucle_pour(NoeudPour *noeud);
	Atome *genere_ri_pour_logement(Type *type,
								   int mode,
								   NoeudExpression *b,
								   NoeudExpression *variable,
								   NoeudExpression *expression,
								   NoeudExpression *bloc_sinon);
	void genere_ri_pour_comparaison_chainee(NoeudExpression *noeud, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	Atome *genere_ri_pour_declaration_structure(NoeudStruct *noeud);
	Atome *genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud);
	Atome *genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud);
	AtomeConstante *genere_initialisation_defaut_pour_type(Type *type);
	void genere_ri_pour_condition(NoeudExpression *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux);
	void genere_ri_blocs_differes(NoeudBloc *bloc);
	Atome *genere_ri_pour_position_code_source(NoeudExpression *noeud);

	void genere_ri_pour_coroutine(NoeudDeclarationFonction *noeud);
	void genere_ri_pour_retiens(NoeudExpression *noeud);

	AtomeConstante *cree_info_type(Type *type);
	AtomeConstante *cree_info_type_defaut(unsigned index, unsigned taille_octet);
	AtomeConstante *cree_info_type_entier(unsigned taille_octet, bool est_relatif);

	Atome *converti_vers_tableau_dyn(Atome *pointeur_tableau_fixe, TypeTableauFixe *type_tableau_fixe, Atome *place);

	AtomeConstante *cree_chaine(dls::vue_chaine_compacte const &chaine);

	void imprime_instruction(Instruction const *inst, std::ostream &os) const;
	Atome *valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident);
	void cree_incrementation_valeur(Type *type, Atome *valeur);
};

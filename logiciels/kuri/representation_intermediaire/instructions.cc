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

#include "instructions.hh"

#include "typage.hh"

AtomeValeurConstante::AtomeValeurConstante(Type *type_, unsigned long long valeur_)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::ENTIERE;
	this->valeur.valeur_entiere = valeur_;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, double valeur_)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::REELLE;
	this->valeur.valeur_reelle = valeur_;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, bool valeur_)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::BOOLEENNE;
	this->valeur.valeur_booleenne = valeur_;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::NULLE;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, Type *pointeur_type)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::TYPE;
	this->valeur.type = pointeur_type;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, kuri::tableau<char> &&donnees_constantes)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
	this->valeur.valeur_tdc.pointeur = donnees_constantes.pointeur;
	this->valeur.valeur_tdc.taille = donnees_constantes.taille;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, char *pointeur, long taille)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
	this->valeur.valeur_tdc.pointeur = pointeur;
	this->valeur.valeur_tdc.taille = taille;
}

AtomeValeurConstante::AtomeValeurConstante(Type *type_, kuri::tableau<AtomeConstante *> &&valeurs)
	: AtomeValeurConstante()
{
	this->type = type_;
	this->valeur.genre = Valeur::Genre::STRUCTURE;
	this->valeur.valeur_structure.pointeur = valeurs.pointeur;
	this->valeur.valeur_structure.taille = valeurs.taille;
	valeurs.pointeur = nullptr;
	valeurs.taille = 0;
}

AtomeGlobale::AtomeGlobale(Type *type_, AtomeConstante *initialisateur_, bool est_externe_, bool est_constante_)
	: AtomeGlobale()
{
	this->type = type_;
	this->initialisateur = initialisateur_;
	this->est_externe = est_externe_;
	this->est_constante = est_constante_;
}

TranstypeConstant::TranstypeConstant(Type *type_, AtomeConstante *valeur_)
	: TranstypeConstant()
{
	this->type = type_;
	this->valeur = valeur_;
}

OpBinaireConstant::OpBinaireConstant(Type *type_, OperateurBinaire::Genre op_, AtomeConstante *operande_gauche_, AtomeConstante *operande_droite_)
	: OpBinaireConstant()
{
	this->type = type_;
	this->op = op_;
	this->operande_gauche = operande_gauche_;
	this->operande_droite = operande_droite_;
}

OpUnaireConstant::OpUnaireConstant(Type *type_, OperateurUnaire::Genre op_, AtomeConstante *operande_)
	: OpUnaireConstant()
{
	this->type = type_;
	this->op = op_;
	this->operande = operande_;
}

AccedeIndexConstant::AccedeIndexConstant(Type *type_, AtomeConstante *accede_, AtomeConstante *index_)
	: AccedeIndexConstant()
{
	this->type = type_;
	this->accede = accede_;
	this->index = index_;
}

AtomeFonction::AtomeFonction(Lexeme const *lexeme_, dls::chaine const &nom_)
	: nom(nom_)
	, lexeme(lexeme_)
{
	genre_atome = Atome::Genre::FONCTION;
}

AtomeFonction::AtomeFonction(Lexeme const *lexeme_, dls::chaine const &nom_, kuri::tableau<Atome *> &&params_)
	: AtomeFonction(lexeme_, nom_)
{
	this->params_entrees = std::move(params_);
}

Instruction *AtomeFonction::derniere_instruction() const
{
	return instructions[instructions.taille - 1];
}

InstructionAppel::InstructionAppel(NoeudExpression *site_, Lexeme const *lexeme_, Atome *appele_)
	: InstructionAppel(site_)
{
	auto type_fonction = appele_->type->comme_fonction();
	// À FAIRE(retours multiples)
	this->type = type_fonction->types_sorties[0];

	this->appele = appele_;
	this->lexeme = lexeme_;
}

InstructionAppel::InstructionAppel(NoeudExpression *site_, Lexeme const *lexeme_, Atome *appele_, kuri::tableau<Atome *> &&args_)
	: InstructionAppel(site_, lexeme_, appele_)
{
	this->args = std::move(args_);
}

InstructionAllocation::InstructionAllocation(NoeudExpression *site_, Type *type_, IdentifiantCode *ident_)
	: InstructionAllocation(site_)
{
	this->type = type_;
	this->ident = ident_;
}

InstructionRetour::InstructionRetour(NoeudExpression *site_, Atome *valeur_)
	: InstructionRetour(site_)
{
	this->valeur = valeur_;
}

InstructionOpBinaire::InstructionOpBinaire(NoeudExpression *site_, Type *type_, OperateurBinaire::Genre op_, Atome *valeur_gauche_, Atome *valeur_droite_)
	: InstructionOpBinaire(site_)
{
	this->type = type_;
	this->op = op_;
	this->valeur_gauche = valeur_gauche_;
	this->valeur_droite = valeur_droite_;
}

InstructionOpUnaire::InstructionOpUnaire(NoeudExpression *site_, Type *type_, OperateurUnaire::Genre op_, Atome *valeur_)
	: InstructionOpUnaire(site_)
{
	this->type = type_;
	this->op = op_;
	this->valeur = valeur_;
}

InstructionChargeMem::InstructionChargeMem(NoeudExpression *site_, Type *type_, Atome *chargee_)
	: InstructionChargeMem(site_)
{
	this->type = type_;
	this->chargee = chargee_;
	this->est_chargeable = type->genre == GenreType::POINTEUR;
}

InstructionStockeMem::InstructionStockeMem(NoeudExpression *site_, Type *type_, Atome *ou_, Atome *valeur_)
	: InstructionStockeMem(site_)
{
	this->type = type_;
	this->ou = ou_;
	this->valeur = valeur_;
}

InstructionLabel::InstructionLabel(NoeudExpression *site_, int id_)
	: InstructionLabel(site_)
{
	this->id = id_;
}

InstructionBranche::InstructionBranche(NoeudExpression *site_, InstructionLabel *label_)
	: InstructionBranche(site_)
{
	this->label = label_;
}

InstructionBrancheCondition::InstructionBrancheCondition(NoeudExpression *site_, Atome *condition_, InstructionLabel *label_si_vrai_, InstructionLabel *label_si_faux_)
	: InstructionBrancheCondition(site_)
{
	this->condition = condition_;
	this->label_si_vrai = label_si_vrai_;
	this->label_si_faux = label_si_faux_;
}

InstructionAccedeMembre::InstructionAccedeMembre(NoeudExpression *site_, Type *type_, Atome *accede_, Atome *index_)
	: InstructionAccedeMembre(site_)
{
	this->type = type_;
	this->accede = accede_;
	this->index = index_;
}

InstructionAccedeIndex::InstructionAccedeIndex(NoeudExpression *site_, Type *type_, Atome *accede_, Atome *index_)
	: InstructionAccedeIndex(site_)
{
	this->type = type_;
	this->accede = accede_;
	this->index = index_;
}

InstructionTranstype::InstructionTranstype(NoeudExpression *site_, Type *type_, Atome *valeur_, TypeTranstypage op_)
	: InstructionTranstype(site_)
{
	this->type = type_;
	this->valeur = valeur_;
	this->op = op_;
}

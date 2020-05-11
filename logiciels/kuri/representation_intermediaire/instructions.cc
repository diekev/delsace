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

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, unsigned long long valeur)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::ENTIERE;
	atome->valeur.valeur_entiere = valeur;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, double valeur)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::REELLE;
	atome->valeur.valeur_reelle = valeur;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, bool valeur)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::BOOLEENNE;
	atome->valeur.valeur_booleenne = valeur;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::NULLE;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, kuri::tableau<char> &&donnees_constantes)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
	atome->valeur.valeur_tdc.pointeur = donnees_constantes.pointeur;
	atome->valeur.valeur_tdc.taille = donnees_constantes.taille;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, char *pointeur, long taille)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
	atome->valeur.valeur_tdc.pointeur = pointeur;
	atome->valeur.valeur_tdc.taille = taille;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::STRUCTURE;
	atome->valeur.valeur_structure.pointeur = valeurs.pointeur;
	atome->valeur.valeur_structure.taille = valeurs.taille;
	valeurs.pointeur = nullptr;
	valeurs.taille = 0;
	return atome;
}

AtomeValeurConstante *AtomeValeurConstante::cree_tableau_fixe(Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
	auto atome = memoire::loge<AtomeValeurConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::TABLEAU_FIXE;
	atome->valeur.valeur_tableau.pointeur = valeurs.pointeur;
	atome->valeur.valeur_tableau.taille = valeurs.taille;
	valeurs.pointeur = nullptr;
	valeurs.taille = 0;
	return atome;
}

AtomeGlobale *AtomeGlobale::cree(Type *type, AtomeConstante *initialisateur, bool est_externe, bool est_constante)
{
	auto atome_globale = memoire::loge<AtomeGlobale>("AtomeGlobale");
	atome_globale->type = type;
	atome_globale->initialisateur = initialisateur;
	atome_globale->est_externe = est_externe;
	atome_globale->est_constante = est_constante;
	return atome_globale;
}

TranstypeConstant *TranstypeConstant::cree(Type *type, AtomeConstante *valeur)
{
	auto atome = memoire::loge<TranstypeConstant>("TranstypeConstant");
	atome->type = type;
	atome->valeur = valeur;
	return atome;
}

OpBinaireConstant *OpBinaireConstant::cree(Type *type, OperateurBinaire::Genre op, AtomeConstante *operande_gauche, AtomeConstante *operande_droite)
{
	auto atome = memoire::loge<OpBinaireConstant>("OpBinaireConstante");
	atome->type = type;
	atome->op = op;
	atome->operande_gauche = operande_gauche;
	atome->operande_droite = operande_droite;
	return atome;
}

OpUnaireConstant *OpUnaireConstant::cree(Type *type, OperateurUnaire::Genre op, AtomeConstante *operande)
{
	auto atome = memoire::loge<OpUnaireConstant>("OpUnaireConstante");
	atome->type = type;
	atome->op = op;
	atome->operande = operande;
	return atome;
}

AccedeIndexConstant *AccedeIndexConstant::cree(Type *type, AtomeConstante *accede, AtomeConstante *index)
{
	auto atome = memoire::loge<AccedeIndexConstant>("InstructionAccedeIndex");
	atome->type = type;
	atome->accede = accede;
	atome->index = index;
	return atome;
}

AtomeFonction *AtomeFonction::cree(Lexeme const *lexeme, dls::chaine const &nom)
{
	auto atome = memoire::loge<AtomeFonction>("AtomeFonction");
	atome->genre_atome = Atome::Genre::FONCTION;
	atome->nom = nom;
	atome->lexeme = lexeme;
	return atome;
}

AtomeFonction *AtomeFonction::cree(Lexeme const *lexeme, dls::chaine const &nom, kuri::tableau<Atome *> &&params)
{
	auto atome = memoire::loge<AtomeFonction>("AtomeFonction");
	atome->genre_atome = Atome::Genre::FONCTION;
	atome->nom = nom;
	atome->params_entrees = std::move(params);
	atome->lexeme = lexeme;
	return atome;
}

InstructionAppel *InstructionAppel::cree(Lexeme const *lexeme, Atome *appele)
{
	auto inst = memoire::loge<InstructionAppel>("InstructionAppel");

	auto type_fonction = static_cast<TypeFonction *>(appele->type);
	// À FAIRE : retours multiples
	inst->type = type_fonction->types_sorties[0];

	inst->appele = appele;
	inst->lexeme = lexeme;
	return inst;
}

InstructionAppel *InstructionAppel::cree(Lexeme const *lexeme, Atome *appele, kuri::tableau<Atome *> &&args)
{
	auto inst = InstructionAppel::cree(lexeme, appele);
	inst->args = std::move(args);
	return inst;
}

InstructionAllocation *InstructionAllocation::cree(Type *type, IdentifiantCode *ident)
{
	auto inst = memoire::loge<InstructionAllocation>("InstructionAllocation");
	inst->type = type;
	inst->ident = ident;
	return inst;
}

InstructionRetour *InstructionRetour::cree(Atome *valeur)
{
	auto inst = memoire::loge<InstructionRetour>("InstructionRetour");
	inst->valeur = valeur;
	return inst;
}

InstructionOpBinaire *InstructionOpBinaire::cree(Type *type, OperateurBinaire::Genre op, Atome *valeur_gauche, Atome *valeur_droite)
{
	auto inst = memoire::loge<InstructionOpBinaire>("InstructionOpBinaire");
	inst->type = type;
	inst->op = op;
	inst->valeur_gauche = valeur_gauche;
	inst->valeur_droite = valeur_droite;
	return inst;
}

InstructionOpUnaire *InstructionOpUnaire::cree(Type *type, OperateurUnaire::Genre op, Atome *valeur)
{
	auto inst = memoire::loge<InstructionOpUnaire>("InstructionOpUnaire");
	inst->type = type;
	inst->op = op;
	inst->valeur = valeur;
	return inst;
}

InstructionChargeMem *InstructionChargeMem::cree(Type *type, Atome *chargee)
{
	auto inst = memoire::loge<InstructionChargeMem>("InstructionChargeMem");
	inst->type = type;
	inst->chargee = chargee;
	inst->est_chargeable = type->genre == GenreType::POINTEUR;
	return inst;
}

InstructionStockeMem *InstructionStockeMem::cree(Type *type, Atome *ou, Atome *valeur)
{
	auto inst = memoire::loge<InstructionStockeMem>("InstructionStockeMem");
	inst->type = type;
	inst->ou = ou;
	inst->valeur = valeur;
	return inst;
}

InstructionLabel *InstructionLabel::cree(int id)
{
	auto inst = memoire::loge<InstructionLabel>("InstructionLabel");
	inst->id = id;
	return inst;
}

InstructionBranche *InstructionBranche::cree(InstructionLabel *label)
{
	auto inst = memoire::loge<InstructionBranche>("InstructionBranche");
	inst->label = label;
	return inst;
}

InstructionBrancheCondition *InstructionBrancheCondition::cree(Atome *condition, InstructionLabel *label_si_vrai, InstructionLabel *label_si_faux)
{
	auto inst = memoire::loge<InstructionBrancheCondition>("InstructionBrancheCondition");
	inst->condition = condition;
	inst->label_si_vrai = label_si_vrai;
	inst->label_si_faux = label_si_faux;
	return inst;
}

InstructionAccedeMembre *InstructionAccedeMembre::cree(Type *type, Atome *accede, Atome *index)
{
	auto inst = memoire::loge<InstructionAccedeMembre>("InstructionAccedeMembre");
	inst->type = type;
	inst->accede = accede;
	inst->index = index;
	return inst;
}

InstructionAccedeIndex *InstructionAccedeIndex::cree(Type *type, Atome *accede, Atome *index)
{
	auto inst = memoire::loge<InstructionAccedeIndex>("InstructionAccedeIndex");
	inst->type = type;
	inst->accede = accede;
	inst->index = index;
	return inst;
}

InstructionTranstype *InstructionTranstype::cree(Type *type, Atome *valeur)
{
	auto inst = memoire::loge<InstructionTranstype>("InstructionTranstype");
	inst->type = type;
	inst->valeur = valeur;
	return inst;
}

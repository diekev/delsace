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

AtomeConstante *AtomeConstante::cree(Type *type, unsigned long long valeur)
{
	auto atome = memoire::loge<AtomeConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::ENTIERE;
	atome->valeur.valeur_entiere = valeur;
	return atome;
}

AtomeConstante *AtomeConstante::cree(Type *type, double valeur)
{
	auto atome = memoire::loge<AtomeConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::REELLE;
	atome->valeur.valeur_reelle = valeur;
	return atome;
}

AtomeConstante *AtomeConstante::cree(Type *type, bool valeur)
{
	auto atome = memoire::loge<AtomeConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::BOOLEENNE;
	atome->valeur.valeur_booleenne = valeur;
	return atome;
}

AtomeConstante *AtomeConstante::cree(Type *type)
{
	auto atome = memoire::loge<AtomeConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::NULLE;
	return atome;
}

AtomeConstante *AtomeConstante::cree(Type *type, const kuri::chaine &chaine)
{
	auto atome = memoire::loge<AtomeConstante>("AtomeEntierConstant");
	atome->type = type;
	atome->valeur.genre = Valeur::Genre::CHAINE;
	atome->valeur.valeur_chaine.pointeur = chaine.pointeur;
	atome->valeur.valeur_chaine.taille = chaine.taille;
	return atome;
}

AtomeFonction *AtomeFonction::cree(dls::chaine const &nom, kuri::tableau<Atome *> &&params)
{
	auto atome = memoire::loge<AtomeFonction>("AtomeFonction");
	atome->genre_atome = Atome::Genre::FONCTION;
	atome->nom = nom;
	atome->params_entrees = std::move(params);
	return atome;
}

InstructionAppel *InstructionAppel::cree(Type *type, Atome *appele, kuri::tableau<Atome *> &&args)
{
	auto inst = memoire::loge<InstructionAppel>("InstructionAppel");
	inst->type = type;
	inst->appele = appele;
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

InstructionOpBinaire *InstructionOpBinaire::cree(Type *type, const DonneesOperateur *op, Atome *valeur_gauche, Atome *valeur_droite)
{
	auto inst = memoire::loge<InstructionOpBinaire>("InstructionOpBinaire");
	inst->type = type;
	inst->op = op;
	inst->valeur_gauche = valeur_gauche;
	inst->valeur_droite = valeur_droite;
	return inst;
}

InstructionOpUnaire *InstructionOpUnaire::cree(Type *type, const DonneesOperateur *op, Atome *valeur)
{
	auto inst = memoire::loge<InstructionOpUnaire>("InstructionOpUnaire");
	inst->type = type;
	inst->op = op;
	inst->valeur = valeur;
	return inst;
}

InstructionChargeMem *InstructionChargeMem::cree(Type *type, Instruction *inst_chargee)
{
	auto inst = memoire::loge<InstructionChargeMem>("InstructionChargeMem");
	inst->type = type;
	inst->inst_chargee = inst_chargee;
	return inst;
}

InstructionStockeMem *InstructionStockeMem::cree(Type *type, InstructionAllocation *ou, Atome *valeur)
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

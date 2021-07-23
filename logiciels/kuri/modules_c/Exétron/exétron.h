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

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char bool;
#endif

struct ContexteKuri;

struct FilExecution;

unsigned int EXETRON_nombre_fils_materiel();

struct FilExecution *EXETRON_cree_fil(struct ContexteKuri *ctx_kuri, void (*rappel)(void *), void *argument);

/* À FAIRE : ajout d'une durée minimum à attendre avant d'annuler si le fil ne fut pas joint. */
bool EXETRON_joins_fil(struct FilExecution *ptr_fil);

void EXETRON_detache_fil(struct FilExecution *ptr_fil);

void EXETRON_detruit_fil(struct ContexteKuri *ctx_kuri, struct FilExecution *ptr_fil);

struct Mutex;

struct Mutex *EXETRON_cree_mutex(struct ContexteKuri *ctx_kuri);

void EXETRON_verrouille_mutex(struct Mutex *mutex);

void EXETRON_deverrouille_mutex(struct Mutex *mutex);

void EXETRON_detruit_mutex(struct ContexteKuri *ctx_kuri, struct Mutex *mutex);

struct VariableCondition;

struct VariableCondition *EXETRON_cree_variable_condition(struct ContexteKuri *ctx_kuri);

void EXETRON_variable_condition_notifie_tous(struct VariableCondition *condition_variable);

void EXETRON_variable_condition_notifie_un(struct VariableCondition *condition_variable);

void EXETRON_variable_condition_attend(struct VariableCondition *condition_variable, struct Mutex *mutex);

void EXETRON_detruit_variable_condition(struct ContexteKuri *ctx_kuri, struct VariableCondition *mutex);

#ifdef __cplusplus
}
#endif

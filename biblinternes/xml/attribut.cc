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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "attribut.h"

#include "erreur.h"
#include "outils.h"

namespace dls {
namespace xml {

const char* Attribut::nom() const
{
	return m_nom.str();
}

const char* Attribut::valeur() const
{
	return m_valeur.str();
}

const Attribut *Attribut::suivant() const
{
	return m_next;
}

int Attribut::valeur_int() const
{
	int i=0;
	requiers_valeur_int(&i);
	return i;
}

unsigned Attribut::valeur_unsigned() const
{
	unsigned i=0;
	requiers_valeur_unsigned(&i);
	return i;
}

bool Attribut::valeur_bool() const
{
	bool b=false;
	requiers_valeur_bool(&b);
	return b;
}

double Attribut::valeur_double() const
{
	double d=0;
	requiers_valeurr_double(&d);
	return d;
}

float Attribut::valeur_float() const
{
	float f=0;
	requiers_valeur_float(&f);
	return f;
}

char* Attribut::analyse_profonde(char *p, bool traite_entites)
{
	// Parse using the name rules: bug fix, was using ParseText before
	p = m_nom.analyse_nom(p);

	if (!p || !*p) {
		return 0;
	}

	// Skip white space before =
	p = XMLUtil::SkipWhiteSpace(p);
	if (*p != '=') {
		return 0;
	}

	++p;	// move up to opening quote
	p = XMLUtil::SkipWhiteSpace(p);
	if (*p != '\"' && *p != '\'') {
		return 0;
	}

	char endTag[2] = { *p, '\0' };
	++p;	// move past opening quote

	p = m_valeur.analyse_texte(p, endTag, traite_entites ? PaireString::ATTRIBUTE_VALUE : PaireString::ATTRIBUTE_VALUE_LEAVE_ENTITIES);
	return p;
}

void Attribut::nom(const char* n)
{
	m_nom.initialise_str(n);
}

XMLError Attribut::requiers_valeur_int(int *valeur) const
{
	if (XMLUtil::ToInt(this->valeur(), valeur)) {
		return XML_NO_ERROR;
	}

	return XML_WRONG_ATTRIBUTE_TYPE;
}

XMLError Attribut::requiers_valeur_unsigned(unsigned int *valeur) const
{
	if (XMLUtil::ToUnsigned(this->valeur(), valeur)) {
		return XML_NO_ERROR;
	}

	return XML_WRONG_ATTRIBUTE_TYPE;
}

XMLError Attribut::requiers_valeur_bool(bool *valeur) const
{
	if (XMLUtil::ToBool(this->valeur(), valeur)) {
		return XML_NO_ERROR;
	}

	return XML_WRONG_ATTRIBUTE_TYPE;
}

XMLError Attribut::requiers_valeur_float(float *valeur) const
{
	if (XMLUtil::ToFloat(this->valeur(), valeur)) {
		return XML_NO_ERROR;
	}

	return XML_WRONG_ATTRIBUTE_TYPE;
}

XMLError Attribut::requiers_valeurr_double(double *valeur) const
{
	if (XMLUtil::ToDouble(this->valeur(), valeur)) {
		return XML_NO_ERROR;
	}

	return XML_WRONG_ATTRIBUTE_TYPE;
}

void Attribut::ajourne_valeur(const char *v)
{
	m_valeur.initialise_str(v);
}

void Attribut::ajourne_valeur(int v)
{
	char buf[TAILLE_TAMPON];
	XMLUtil::ToStr(v, buf, TAILLE_TAMPON);
	m_valeur.initialise_str(buf);
}

void Attribut::ajourne_valeur(unsigned v)
{
	char buf[TAILLE_TAMPON];
	XMLUtil::ToStr(v, buf, TAILLE_TAMPON);
	m_valeur.initialise_str(buf);
}

void Attribut::ajourne_valeur(bool v)
{
	char buf[TAILLE_TAMPON];
	XMLUtil::ToStr(v, buf, TAILLE_TAMPON);
	m_valeur.initialise_str(buf);
}

void Attribut::ajourne_valeur(double v)
{
	char buf[TAILLE_TAMPON];
	XMLUtil::ToStr(v, buf, TAILLE_TAMPON);
	m_valeur.initialise_str(buf);
}

void Attribut::ajourne_valeur(float v)
{
	char buf[TAILLE_TAMPON];
	XMLUtil::ToStr(v, buf, TAILLE_TAMPON);
	m_valeur.initialise_str(buf);
}

}  /* namespace xml */
}  /* namespace dls */

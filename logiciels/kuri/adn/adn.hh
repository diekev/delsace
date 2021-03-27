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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "parsage/base_syntaxeuse.hh"
#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

#include "outils.hh"

struct IdentifiantADN {
private:
	kuri::chaine_statique nom = "";
	kuri::chaine nom_sans_accent = "";

public:
	IdentifiantADN() = default;

	IdentifiantADN(dls::vue_chaine_compacte n)
		: nom(n)
		, nom_sans_accent(supprime_accents(nom))
	{}

	kuri::chaine_statique nom_cpp() const
	{
		return nom_sans_accent;
	}

	kuri::chaine_statique nom_kuri() const
	{
		return nom;
	}
};

template <typename T>
struct FormatKuri;

template <typename T>
struct FormatCPP;

struct Type {
	kuri::chaine nom = "rien";
	kuri::tableau<GenreLexeme> specifiants{};
};

template <>
struct FormatKuri<Type> {
	const Type &type;
};

std::ostream &operator<<(std::ostream &os, FormatKuri<Type> format);

template <>
struct FormatCPP<Type> {
	const Type &type;
};

std::ostream &operator<<(std::ostream &os, FormatCPP<Type> format);

struct Membre {
	IdentifiantADN nom{};
	Type type{};

	bool valeur_defaut_est_acces = false;
	kuri::chaine_statique valeur_defaut = "";
};

class Proteine {
protected:
	IdentifiantADN m_nom{};

public:
	Proteine(IdentifiantADN nom);

	virtual ~Proteine() = default;
	virtual void genere_code_cpp(std::ostream &os, bool pour_entete) = 0;
	virtual void genere_code_kuri(std::ostream &os) = 0;

	virtual bool est_fonction() const
	{
		return false;
	}

	const IdentifiantADN &nom() const
	{
		return m_nom;
	}
};

class ProteineStruct final : public Proteine {
	kuri::tableau<Membre> m_membres{};

public:
	ProteineStruct(IdentifiantADN nom);

	void genere_code_cpp(std::ostream &os, bool pour_entete) override;

	void genere_code_kuri(std::ostream &os) override;

	void ajoute_membre(Membre const membre);
};

class ProteineEnum final : public Proteine {
	kuri::tableau<Membre> m_membres{};

public:
	ProteineEnum(IdentifiantADN nom);

	void genere_code_cpp(std::ostream &os, bool pour_entete) override;

	void genere_code_kuri(std::ostream &os) override;

	void ajoute_membre(Membre const membre);
};

struct Parametre {
	IdentifiantADN nom{};
	Type type{};
};

class ProteineFonction final : public Proteine {
	kuri::tableau<Parametre> m_parametres{};
	Type m_type_sortie{};

public:
	ProteineFonction(IdentifiantADN nom);

	void genere_code_cpp(std::ostream &os, bool pour_entete) override;

	void genere_code_kuri(std::ostream &os) override;

	Type &type_sortie()
	{
		return m_type_sortie;
	}

	void ajoute_parametre(Parametre const parametre);

	bool est_fonction() const override
	{
		return true;
	}
};

struct SyntaxeuseADN : public BaseSyntaxeuse {
	kuri::tableau<Proteine *> proteines{};

	SyntaxeuseADN(Fichier *fichier);

	~SyntaxeuseADN() override;

	template <typename T>
	T *cree_proteine(IdentifiantADN nom)
	{
		auto proteine = new T(nom);
		proteines.ajoute(proteine);
		return proteine;
	}

private:
	void analyse_une_chose() override;

	void parse_fonction();

	void parse_enum();

	void parse_struct();

	void parse_type(Type &type);

	void gere_erreur_rapportee(const kuri::chaine &message_erreur) override;
};

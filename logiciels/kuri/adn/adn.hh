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

struct FluxSortieKuri {
private:
	std::ostream &m_os;

	template <typename T>
	friend FluxSortieKuri &operator<<(FluxSortieKuri &flux, const T &valeur);

public:
	FluxSortieKuri(std::ostream &os)
		: m_os(os)
	{}

	operator std::ostream&()
	{
		return m_os;
	}
};

template <typename T>
FluxSortieKuri &operator<<(FluxSortieKuri &flux, const T &valeur)
{
	flux.m_os << valeur;
	return flux;
}

struct FluxSortieCPP {
private:
	std::ostream &m_os;

	template <typename T>
	friend FluxSortieCPP &operator<<(FluxSortieCPP &flux, const T &valeur);

public:
	FluxSortieCPP(std::ostream &os)
		: m_os(os)
	{}

	operator std::ostream&()
	{
		return m_os;
	}
};

template <typename T>
FluxSortieCPP &operator<<(FluxSortieCPP &flux, const T &valeur)
{
	flux.m_os << valeur;
	return flux;
}

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

FluxSortieCPP &operator<<(FluxSortieCPP &flux, IdentifiantADN const &ident);

FluxSortieKuri &operator<<(FluxSortieKuri &flux, IdentifiantADN const &ident);

struct Type {
	kuri::chaine nom = "rien";
	kuri::tableau<GenreLexeme> specifiants{};
	bool est_enum = false;
};

FluxSortieCPP &operator<<(FluxSortieCPP &os, Type const &type);

FluxSortieKuri &operator<<(FluxSortieKuri &os, Type const &type);

struct Membre {
	IdentifiantADN nom{};
	Type type{};

	bool valeur_defaut_est_acces = false;
	kuri::chaine_statique valeur_defaut = "";
};

class ProteineEnum;
class ProteineStruct;

class Proteine {
protected:
	IdentifiantADN m_nom{};

public:
	Proteine(IdentifiantADN nom);

	virtual ~Proteine() = default;
	virtual void genere_code_cpp(FluxSortieCPP &os, bool pour_entete) = 0;
	virtual void genere_code_kuri(FluxSortieKuri &os) = 0;

	virtual bool est_fonction() const
	{
		return false;
	}

	const IdentifiantADN &nom() const
	{
		return m_nom;
	}

	virtual ProteineEnum *comme_enum()
	{
		return nullptr;
	}

	virtual ProteineStruct *comme_struct()
	{
		return nullptr;
	}
};

class ProteineStruct final : public Proteine {
	kuri::tableau<Membre> m_membres{};

	ProteineStruct *m_mere = nullptr;

public:
	ProteineStruct(IdentifiantADN nom);

	ProteineStruct(ProteineStruct const &) = default;
	ProteineStruct &operator=(ProteineStruct const &) = default;

	void genere_code_cpp(FluxSortieCPP &os, bool pour_entete) override;

	void genere_code_kuri(FluxSortieKuri &os) override;

	void ajoute_membre(Membre const membre);

	void descend_de(ProteineStruct *proteine)
	{
		m_mere = proteine;
	}

	ProteineStruct *comme_struct() override
	{
		return this;
	}

	void pour_chaque_membre_recursif(std::function<void(Membre const &)> rappel);
};

class ProteineEnum final : public Proteine {
	kuri::tableau<Membre> m_membres{};

public:
	ProteineEnum(IdentifiantADN nom);

	void genere_code_cpp(FluxSortieCPP &os, bool pour_entete) override;

	void genere_code_kuri(FluxSortieKuri &os) override;

	void ajoute_membre(Membre const membre);

	ProteineEnum *comme_enum() override
	{
		return this;
	}
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

	void genere_code_cpp(FluxSortieCPP &os, bool pour_entete) override;

	void genere_code_kuri(FluxSortieKuri &os) override;

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

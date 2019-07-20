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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "makego.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "../outils/conditions.h"

struct StructField {
	dls::chaine type = {};
	dls::chaine name = {};
	dls::chaine default_value = {};
	bool is_pointer = false;
	bool is_owner = false;
};

struct StructDecl {
	dls::chaine name = {};
	bool need_dtor = false;

	dls::tableau<std::shared_ptr<StructField>> fields = {};
};

dls::tableau<dls::chaine> types;

bool is_type(const dls::chaine &token)
{
	if (dls::outils::est_element(token, "bool", "char", "short", "int", "long", "float", "double")) {
		return true;
	}

	auto iter = std::find(types.debut(), types.fin(), token);

	if (iter != types.fin()) {
		return true;
	}

	return false;
}

StructDecl read_struct(std::ifstream &is, const dls::chaine &name)
{
	std::string line;
	dls::chaine token;

	StructDecl decl;
	decl.name = name;

	types.pousse(name);

	while (std::getline(is, line)) {
		if (line == "};") {
			break;
		}

		std::stringstream ss;
		ss << line;

		auto field = std::shared_ptr<StructField>(new StructField);
		field->name = name;

		while (ss >> token) {
			if (token == "own") {
				field->is_owner = true;
			}
			else if (is_type(token)) {
				field->type = token;
			}
			else {
				auto strip_ptr = false;

				if (token[0] == '*') {
					field->is_pointer = true;

					decl.need_dtor |= field->is_owner;
					strip_ptr = true;
				}

				if (token.taille() == 1) {
					continue;
				}

				field->name = token.sous_chaine(
							(0 + strip_ptr),
							token.taille() - (1l + strip_ptr));
			}
		}

		decl.fields.pousse(field);
	}

	return decl;
}

void write_struct(const StructDecl &struct_decl, std::ofstream &outfile);

void read_file(const dls::chaine &filename)
{
	std::ifstream is;
	is.open(filename.c_str());

	if (!is.is_open()) {
		std::cerr << "Cannot open file: " << filename << '\n';
	}

	std::string line;
	dls::chaine token;

	dls::tableau<StructDecl> decls;

	while (std::getline(is, line)) {
		std::stringstream ss;
		ss << line;

		while (ss >> token) {
			if (token == "struct") {
				ss >> token;

				StructDecl decl = read_struct(is, token);
				decls.pousse(decl);
			}
		}
	}

	std::ofstream os("/tmp/test_out.cc");

	for (const auto &decl : decls) {
		write_struct(decl, os);
	}
}

void write_ctor(const StructDecl &struct_decl, std::ofstream &outfile);
void write_move_ctor(const StructDecl &struct_decl, std::ofstream &outfile);
void write_dtor(const StructDecl &struct_decl, std::ofstream &outfile);

void write_ctor_default(const StructDecl &struct_decl, std::ofstream &outfile);
void write_move_ctor_default(const StructDecl &struct_decl, std::ofstream &outfile);
void write_dtor_default(const StructDecl &struct_decl, std::ofstream &outfile);

void write_move_operator(const StructDecl &struct_decl, std::ofstream &outfile);

void write_struct(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << "struct " << struct_decl.name << " {\n";

	/* write fields */
	for (auto &field : struct_decl.fields) {
		outfile << '\t' << field->type << ' ';

		if (field->is_pointer) {
			outfile << '*';
		}

		outfile << field->name << ";\n";
	}

	if (struct_decl.need_dtor) {
		write_ctor(struct_decl, outfile);
		write_move_ctor(struct_decl, outfile);
		write_dtor(struct_decl, outfile);
		write_move_operator(struct_decl, outfile);
	}
	else {
		write_ctor_default(struct_decl, outfile);
		write_move_ctor_default(struct_decl, outfile);
		write_dtor_default(struct_decl, outfile);
	}

	outfile << "};";
}

void write_ctor(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << struct_decl.name << "()\n\t{\n";

	for (const auto &field : struct_decl.fields) {
		if (!field->is_owner || !field->is_pointer) {
			continue;
		}

		outfile << "\t\tthis->" << field->name << " = nullptr;\n";
	}

	outfile << "\t}\n";
}

void write_ctor_default(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << struct_decl.name << "() = default;\n";
}

void write_move_ctor(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << struct_decl.name << "(" << struct_decl.name <<  " &&rhs)\n\t{\n";

	for (const auto &field : struct_decl.fields) {
		outfile << "\t\tthis->" << field->name << " = rhs." << field->name << ";\n";

		if (field->is_owner && field->is_pointer) {
			outfile << "\t\trhs." << field->name << " = nullptr;\n";
		}
		else {
			outfile << "\t\trhs." << field->name << " = static_cast<" << field->type << ">(0);\n";
		}
	}

	outfile << "\t}\n";
}

void write_move_ctor_default(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << struct_decl.name << "(" << struct_decl.name <<  " &&rhs) = default;\n";
}

void write_dtor(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << '~' << struct_decl.name << "()\n\t{\n";

	for (const auto &field : struct_decl.fields) {
		if (!field->is_owner || !field->is_pointer) {
			continue;
		}

		outfile << "\t\tdelete " << field->name << ";\n";
	}

	outfile << "\t}\n";
}

void write_dtor_default(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << '~' << struct_decl.name << "() = default;\n";
}

void write_move_operator(const StructDecl &struct_decl, std::ofstream &outfile)
{
	outfile << '\n';
	outfile << '\t' << struct_decl.name << " &operator=(" << struct_decl.name <<  " &&rhs)\n\t{\n";

	for (const auto &field : struct_decl.fields) {
		outfile << "\t\tthis->" << field->name << " = rhs." << field->name << ";\n";

		if (field->is_owner && field->is_pointer) {
			outfile << "\t\trhs." << field->name << " = nullptr;\n";
		}
		else {
			outfile << "\t\trhs." << field->name << " = static_cast<" << field->type << ">(0);\n";
		}
	}

	outfile << "\t}\n";
}

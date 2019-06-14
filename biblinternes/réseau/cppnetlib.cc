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

#if 0
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/uri/uri.hpp>
#include <iostream>

namespace http = boost::network::http;
namespace uri = boost::network::uri;

struct hello_world;

using serveur = http::server<hello_world>;

void mappemonde_get(const serveur::request &/*requete*/, serveur::connection_ptr reponse)
{
	std::ostringstream os;
	os << "<HTML>\n";
	os << "<head><meta charset=\"utf-8\"></head>\n";
	os << "<body>\n";
	os << "<p>mappemonde:GET à IMPLÉMENTER!</p>";
	os << "</body>\n";

	/* requiert base de données */
	/* charge patron page */

	reponse->write(os.str());
}

void mappemonde_post(const serveur::request &/*requete*/, serveur::connection_ptr reponse)
{
	std::ostringstream os;
	os << "mappemonde:POST à IMPLÉMENTER!";

	reponse->write(os.str());
}

void chemin_inexistant(const serveur::request &/*requete*/, serveur::connection_ptr reponse)
{
	static serveur::response_header error_headers[] = {
		{"Connection", "keep-alive"}
	};

	reponse->set_status(serveur::connection::not_found);
	reponse->set_headers(
				boost::make_iterator_range(error_headers, error_headers + 1));
	//reponse->write("Page non trouvée.");
}

struct hello_world {
	void operator()(const serveur::request &requete, serveur::connection_ptr reponse)
	{
		auto destination = requete.destination;
		auto method = requete.method;

		auto url = uri::uri("http://www.ludocensor.fr");
		url.append(destination);

		auto chemin = url.path();

		if (method == "GET") {
			if (chemin == "/ludocensor/mappemonde") {
				mappemonde_get(requete, reponse);
			}
			else {
				chemin_inexistant(requete, reponse);
			}
		}
		else if (method == "POST") {
			if (chemin == "/ludocensor/mappemonde") {
				mappemonde_post(requete, reponse);
			}
			else {
				chemin_inexistant(requete, reponse);
			}
		}
		else {
			static serveur::response_header error_headers[] = {
				{"Connection", "close"}
			};

			reponse->set_status(serveur::connection::not_supported);
			reponse->set_headers(
						boost::make_iterator_range(error_headers, error_headers + 1));

			reponse->write("Méthode non supporté.");
		}
	}
};

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "Utilisation : " << argv[0] << " adresse port\n";
		return 1;
	}

	try {
		hello_world handler;
		serveur::options options(handler);
		serveur serveur_(options.address(argv[1]).port(argv[2]));

		serveur_.run();
	}
	catch (const std::exception &e) {
		std::cerr << e.what() << '\n';

		return 1;
	}

	return 0;
}
//#else
#include <girafeenfeu/patron_html/patron_html.h>

#include <experimental/filesystem>
#include <fstream>

namespace std_filesystem = std::experimental::filesystem;

using paire_string = std::pair<std::string, std::string>;

class ParametresModelage {
	std::vector<paire_string> m_dictionaire = {};

public:
	void ajoute(const std::string &nom, const std::string &valeur)
	{
		for (auto &property : m_dictionaire) {
			if (property.first == nom) {
				property.second = valeur;
				return;
			}
		}

		m_dictionaire.push_back(paire_string(nom, valeur));
	}

	std::vector<paire_string> dictionaire() const
	{
		return m_dictionaire;
	}
};

class Modeleur {
	modelage::LoaderMemory m_chargeur{};

public:
	Modeleur() = default;

	void charge_modeles()
	{
		if (std_filesystem::is_directory("modèles")) {
			for (const auto &fichier : std_filesystem::directory_iterator("modèles")) {
				if (!std_filesystem::is_regular_file(fichier.status())) {
					continue;
				}

				std_filesystem::path chemin = fichier.path();

				std::ifstream input;
				input.open(chemin.c_str());

				if (!input.is_open()) {
					continue;
				}

				std::string contenu((std::istreambuf_iterator<char>(input)),
									 (std::istreambuf_iterator<char>()));

				auto nom = chemin.stem().string();

				m_chargeur.add(nom, contenu);
			}
		}

		m_chargeur.add("base", "<html><head>{{ title }}</head>\n<body><p>{% include text %}</p></body></html>\n");
		m_chargeur.add("text", "Hi there, {{ name }}. How are you??");
		m_chargeur.add("block", "<p>Items:</p>\n{% block items %}<p>\n"
								"Title: {{ title }}<br/>\n"
								"Text: {{ text }}<br/>\n"
								"{% block details %}Detail: {{ detail }}{% endblock %}\n"
								"</p>{% endblock %}");
	}

	std::string traduit(const std::string &nom_modele, const ParametresModelage &parametres)
	{
		modelage::Template modeleur(m_chargeur);

		modeleur.load(nom_modele);

		for (const paire_string &paire : parametres.dictionaire()) {
			modeleur.set(paire.first, paire.second);
		}

		std::ostringstream oss;
		modeleur.render(oss);

		return oss.str();
	}
};

extern	"C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"

#include <handlebars.h>
#include <handlebars_compiler.h>
#include <handlebars_vm.h>
#include <handlebars_value.h>
#include <handlebars_opcode_serializer.h>

#pragma GCC diagnostic pop
}

int test_handlebars()
{
	unsigned long compiler_flags = 0;
	struct handlebars_context * ctx;
		struct handlebars_parser * parser;
		struct handlebars_compiler * compiler;
		struct handlebars_vm * vm;
		int error = 0;
		jmp_buf jmp;

		ctx = handlebars_context_ctor();

		// Save jump buffer
		if( handlebars_setjmp_ex(ctx, &jmp) ) {
			std::fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
			handlebars_context_dtor(ctx);
			return error;
		}

		parser = handlebars_parser_ctor(ctx);
		compiler = handlebars_compiler_ctor(ctx);
		vm = handlebars_vm_ctor(ctx);
		vm->helpers = handlebars_value_ctor(ctx);
		vm->partials = handlebars_value_ctor(ctx);

		if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
			parser->ignore_standalone = 1;
		}

		handlebars_compiler_set_flags(compiler, compiler_flags);


		// Read
		std::ifstream input0;
		input0.open("modèles/layouts/main.handlebars");

		std::string contenu0((std::istreambuf_iterator<char>(input0)),
							 (std::istreambuf_iterator<char>()));

		std::ifstream input;
		input.open("modèles/erreur_video.handlebars");

		std::string contenu((std::istreambuf_iterator<char>(input)),
							 (std::istreambuf_iterator<char>()));


		auto pos = contenu0.find("{{{body}}}");

		contenu0.replace(pos, 10, contenu);

		parser->tmpl = (handlebars_string*)handlebars_string_ctor(HBSCTX(parser), contenu0.c_str(), contenu0.size());

		// Read context
		struct handlebars_value * context = NULL;
#if 0
		if( input_data_name ) {
			char * context_str = file_get_contents(input_data_name);
			if( context_str && strlen(context_str) ) {
				context = handlebars_value_from_json_string(ctx, context_str);
			}
		}
#endif
		if( !context ) {
			context = handlebars_value_ctor(ctx);
		}

		// Parse
		handlebars_parse(parser);

		// Compile
		handlebars_compiler_compile(compiler, parser->program);

		// Serialize
		struct handlebars_module * module = handlebars_program_serialize(ctx, compiler->program);

		// Execute
		handlebars_vm_execute(vm, module, context);

		fprintf(stdout, "%.*s", (int) vm->buffer->len, vm->buffer->val);

		handlebars_context_dtor(ctx);
		return error;
}

void test_template()
{

	ParametresModelage parametres;
	parametres.ajoute("title", "Testing memory loader");
	parametres.ajoute("name", "Kévin");

	Modeleur modeleur;
	modeleur.charge_modeles();

	auto page = modeleur.traduit("base", parametres);

	std::cerr << page << '\n';
}

int main()
{
	test_handlebars();
}
#endif

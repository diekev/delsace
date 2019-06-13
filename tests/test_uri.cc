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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "test_uri.hh"

#include "../réseau/uri.hh"

void test_uri(dls::test_unitaire::Controleuse &controleuse)
{
	{
		auto uri = reseau::uri("https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("https"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view("//john.doe@www.example.com:123"));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view("john.doe"));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view("www.example.com"));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view("123"));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("/forum/questions/"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view("?tag=networking&order=newest"));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view("#top"));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("ldap://[2001:db8::7]/c=GB?objectClass?one");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("ldap"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view("//[2001:db8::7]"));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view("[2001:db8::7]"));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("/c=GB"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view("?objectClass?one"));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("mailto:John.Doe@example.com");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("mailto"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("John.Doe@example.com"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("news:comp.infosystems.www.servers.unix");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("news"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("comp.infosystems.www.servers.unix"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("tel:+1-816-555-1212");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("tel"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("+1-816-555-1212"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("telnet://192.0.2.16:80/");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("telnet"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view("//192.0.2.16:80"));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view("192.0.2.16"));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view("80"));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("/"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
	{
		auto uri = reseau::uri("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");

		CU_VERIFIE_EGALITE(controleuse, uri.schema(), std::string_view("urn"));
		CU_VERIFIE_EGALITE(controleuse, uri.autorite(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.userinfo(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.hote(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.port(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.chemin(), std::string_view("oasis:names:specification:docbook:dtd:xml:4.1.2"));
		CU_VERIFIE_EGALITE(controleuse, uri.requete(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.fragment(), std::string_view(""));
		CU_VERIFIE_EGALITE(controleuse, uri.est_valide(), true);
	}
}

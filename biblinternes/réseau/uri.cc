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

#include "uri.hh"

namespace reseau {

uri::uri(const std::string &chaine)
	: m_uri(chaine)
{
	/* le schéma se trouve entre le début et ':' */
	auto pos = m_uri.find(':');

	if (pos != std::string::npos) {
		m_schema = std::string_view(&m_uri[0], pos);
		pos += 1;
	}
	else {
		pos = 0;
	}

	if (m_uri[pos] == '/' && m_uri[pos + 1] == '/') {
		pos = pos + 2;
		auto fin_autorite = m_uri.find('/', pos);

		if (fin_autorite == std::string::npos) {
			fin_autorite = m_uri.size();
		}

		m_autorite = std::string_view(&m_uri[pos - 2], fin_autorite - pos + 2);

		auto fin_user_info = m_uri.find('@', pos);

		/* il est possible d'avoir un mot de passe : username:motdepasse */
		if (fin_user_info != std::string::npos) {
			m_userinfo = std::string_view(&m_uri[pos], fin_user_info - (pos));

			pos = fin_user_info + 1;
		}

		// il est possible d'avoir des addresse IPv6 entre []

		if (m_uri[pos] == '[') {
			auto fin_adresse = m_uri.find(']');
			m_hote = std::string_view(&m_uri[pos], fin_adresse - pos + 1);

			if (m_uri[fin_adresse + 1] == ':') {
				m_port = std::string_view(&m_uri[fin_adresse + 2], fin_autorite - fin_adresse - 2);
			}
		}
		else {
			auto debut_port = m_uri.find(':', pos);

			if (debut_port != std::string::npos) {
				m_port = std::string_view(&m_uri[debut_port + 1], fin_autorite - debut_port - 1);
				m_hote = std::string_view(&m_uri[pos], debut_port - pos);
			}
			else {
				m_hote = std::string_view(&m_uri[pos], fin_autorite - pos);
			}
		}

		pos = fin_autorite;
	}

	auto pos_requete = m_uri.find('?');
	auto pos_fragment = m_uri.find('#');
	auto pos_fin_chemin = m_uri.size();

	if (pos_requete != std::string::npos) {
		pos_fin_chemin = pos_requete;
	}
	else if (pos_fragment != std::string::npos) {
		pos_fin_chemin = pos_requete;
	}

	m_chemin = std::string_view(&m_uri[pos], pos_fin_chemin - pos);

	if (pos_requete != std::string::npos) {
		if (pos_fragment == std::string::npos) {
			pos_fragment = m_uri.size();
		}

		m_requete = std::string_view(&m_uri[pos_requete], pos_fragment - pos_requete);
	}

	if (pos_fragment != std::string::npos) {
		m_fragment = std::string_view(&m_uri[pos_fragment], m_uri.size() - pos_fragment);
	}
}

bool uri::est_valide() const
{
	if (m_schema.empty()) {
		return false;
	}

	if (!m_autorite.empty()) {
		if (m_hote.empty()) {
			return false;
		}
	}

	return true;
}

std::string_view uri::schema() const
{
	return m_schema;
}

std::string_view uri::autorite() const
{
	return m_autorite;
}

std::string_view uri::chemin() const
{
	return m_chemin;
}

std::string_view uri::requete() const
{
	return m_requete;
}

std::string_view uri::fragment() const
{
	return m_fragment;
}

std::string_view uri::hote() const
{
	return m_hote;
}

std::string_view uri::userinfo() const
{
	return m_userinfo;
}

std::string_view uri::port() const
{
	return m_port;
}

}  /* namespace reseau */

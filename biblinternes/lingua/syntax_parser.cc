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

#include "syntax_parser.h"

#include <algorithm>
#include <iostream>
#include "biblinternes/structures/tableau.hh"

/* Natural Langage Processing - Syntax Parser, based on:
 * - http://www.tutorialspoint.com/artificial_intelligence/artificial_intelligence_natural_language_processing.htm
 */

extern dls::tableau<dls::chaine> determiners;
extern dls::tableau<dls::chaine> adjectives;
extern dls::tableau<dls::chaine> nouns;
extern dls::tableau<dls::chaine> verbs;

/* ************************************************************************** */

std::ostream &operator<<(std::ostream &os, token tok)
{
	switch (tok) {
		case token::determiner:
			os << "determiner";
			break;
		case token::noun:
			os << "noun";
			break;
		case token::adjective:
			os << "adjective";
			break;
		case token::verb:
			os << "verb";
			break;
		case token::invalid:
			os << "invalid";
			break;
	}

	return os;
}

/* ************************************************************************** */

static inline auto find_token(const dls::tableau<dls::chaine> &dict, const dls::chaine &str)
{
	auto iter = std::find(dict.debut(), dict.fin(), str);
	return iter != dict.fin();
}

static bool parse_noun_phrase(SyntaxParser *parser)
{
	/* A noun-phrase has the structure determiner-noun or determiner-adjective-noun. */
	if (parser->get_token() != token::determiner) {
		return false;
	}

	auto last_token = parser->get_token();

	if (last_token == token::noun) {
		return true;
	}

	if (last_token == token::adjective) {
		if (parser->get_token() != token::noun) {
			return false;
		}
	}

	return true;
}

static bool parse_verb_phrase(SyntaxParser *parser)
{
	/* A verb-phrase has the structure verb-noun-phrase. */
	if (parser->get_token() != token::verb) {
		return false;
	}

	return parse_noun_phrase(parser);
}

static void parse_sentence(SyntaxParser *parser)
{
	/* A sentence is either a noun-phrase or a verb-phrase. */
	if (!parse_noun_phrase(parser)) {
		return;
	}

	if (!parse_verb_phrase(parser)) {
		return;
	}
}

/* ************************************************************************** */

void SyntaxParser::operator()(const dls::chaine &sentence)
{
	m_sentence = sentence;
	m_stream << m_sentence;

	parse_sentence(this);
}

token SyntaxParser::get_token()
{
	dls::chaine str;
	if (!(m_stream >> str)) {
		return token::invalid;
	}

	token tok;

	if (find_token(determiners, str)) {
		tok = token::determiner;
	}
	else if (find_token(adjectives, str)) {
		tok = token::adjective;
	}
	else if (find_token(nouns, str)) {
		tok = token::noun;
	}
	else if (find_token(verbs, str)) {
		tok = token::verb;
	}
	else {
		tok = token::invalid;
	}

	std::cerr << tok << ' ';

	return tok;
}

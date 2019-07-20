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
 * The Original Code is Copyright (c) 2013 Jared Grubb.
 * Modifications Copyright (c) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "docopt.hh"
#include "docopt_util.hh"
#include "docopt_private.hh"

#include "docopt_value.hh"

#include <unordered_set>
#include <iostream>
#include <cassert>
#include <cstddef>

namespace dls {
namespace docopt {

std::ostream& operator<<(std::ostream& os, value const& val)
{
	if (val.isBool()) {
		bool b = val.asBool();
		os << (b ? "true" : "false");
	}
	else if (val.isLong()) {
		long v = val.asLong();
		os << v;
	}
	else if (val.isString()) {
		auto const& str = val.asString();
		os << '"' << str << '"';
	}
	else if (val.isStringList()) {
		auto const& list = val.asStringList();
		os << "[";
		bool first = true;

		for(auto const& el : list) {
			if (first) {
				first = false;
			}
			else {
				os << ", ";
			}
			os << '"' << el << '"';
		}

		os << "]";
	}
	else {
		os << "null";
	}

	return os;
}

class Tokens {
public:
	explicit Tokens(const dls::tableau<dls::chaine> &tokens, bool isParsingArgv = true)
		: fTokens(tokens)
		, fIsParsingArgv(isParsingArgv)
	{}

	explicit operator bool() const {
		return fIndex < fTokens.taille();
	}

	static Tokens from_pattern(dls::chaine const& source) {
		static const std::regex re_separators {
			"(?:\\s*)" // any spaces (non-matching subgroup)
			"("
			"[\\[\\]\\(\\)\\|]" // one character of brackets or parens or pipe character
			"|"
			"\\.\\.\\."  // elipsis
			")" };

		static const std::regex re_strings {
			"(?:\\s*)" // any spaces (non-matching subgroup)
			"("
			"\\S*<.*?>"  // strings, but make sure to keep "< >" strings together
			"|"
			"[^<>\\s]+"     // string without <>
			")" };

		// We do two stages of regex matching. The '[]()' and '...' are strong delimeters
		// and need to be split out anywhere they occur (even at the end of a token). We
		// first split on those, and then parse the stuff between them to find the string
		// tokens. This is a little harder than the python version, since they have regex.split
		// and we dont have anything like that.

		dls::tableau<dls::chaine> tokens;
		auto std_source = std::string(source.c_str());
		std::for_each(std::sregex_iterator{ std_source.begin(), std_source.end(), re_separators },
					  std::sregex_iterator{},
					  [&](std::smatch const& match)
		{
			// handle anything before the separator (this is the "stuff" between the delimeters)
			if (match.prefix().matched) {
				std::for_each(std::sregex_iterator{match.prefix().first, match.prefix().second, re_strings},
							  std::sregex_iterator{},
							  [&](std::smatch const& m)
				{
					tokens.pousse(m[1].str());
				});
			}

			// handle the delimter token itself
			if (match[1].matched) {
				tokens.pousse(match[1].str());
			}
		});

		return Tokens(tokens, false);
	}

	dls::chaine const& current() const {
		if (*this)
			return fTokens[fIndex];

		static dls::chaine const empty;
		return empty;
	}

	dls::chaine the_rest() const {
		if (!*this)
			return {};
		return join(fTokens.debut()+static_cast<std::ptrdiff_t>(fIndex),
					fTokens.fin(),
					" ");
	}

	dls::chaine pop() {
		return std::move(fTokens.a(fIndex++));
	}

	bool isParsingArgv() const { return fIsParsingArgv; }

	struct OptionError : std::runtime_error { using runtime_error::runtime_error; };

private:
	dls::tableau<dls::chaine> fTokens;
	long fIndex = 0;
	bool fIsParsingArgv;
};

// Get all instances of 'T' from the pattern
template <typename T>
dls::tableau<T*> flat_filter(Pattern& pattern) {
	dls::tableau<Pattern*> flattened = pattern.flat([](Pattern const* p) -> bool {
			return dynamic_cast<T const*>(p) != nullptr;
});

	// now, we're guaranteed to have T*'s, so just use static_cast
	dls::tableau<T*> ret;
	std::transform(flattened.debut(), flattened.fin(), std::back_inserter(ret), [](Pattern* p) {
		return static_cast<T*>(p);
	});
	return ret;
}

static dls::tableau<dls::chaine> parse_section(dls::chaine const& name, dls::chaine const& source)
{
	auto std_name = std::string(name.c_str());
	auto std_source = std::string(source.c_str());
	// ECMAScript regex only has "?=" for a non-matching lookahead. In order to make sure we always have
	// a newline to anchor our matching, we have to avoid matching the final newline of each grouping.
	// Therefore, our regex is adjusted from the docopt Python one to use ?= to match the newlines before
	// the following lines, rather than after.
	std::regex const re_section_pattern {
		"(?:^|\\n)"  // anchored at a linebreak (or start of string)
		"("
		"[^\\n]*" + std_name + "[^\\n]*(?=\\n?)" // a line that contains the name
				"(?:\\n[ \\t].*?(?=\\n|$))*"         // followed by any number of lines that are indented
				")",
				std::regex::icase
	};

	dls::tableau<dls::chaine> ret;
	std::for_each(std::sregex_iterator(std_source.begin(), std_source.end(), re_section_pattern),
				  std::sregex_iterator(),
				  [&](std::smatch const& match)
	{
		ret.pousse(trim(match[1].str()));
	});

	return ret;
}

static bool is_argument_spec(dls::chaine const& token) {
	if (token.est_vide())
		return false;

	if (token[0]=='<' && token[token.taille()-1]=='>')
		return true;

	if (std::all_of(token.debut(), token.fin(), &::isupper))
		return true;

	return false;
}

template <typename I>
dls::tableau<dls::chaine> longOptions(I iter, I end) {
	dls::tableau<dls::chaine> ret;
	std::transform(iter, end,
				   std::back_inserter(ret),
				   [](typename I::reference opt) { return opt->longOption(); });
	return ret;
}

static PatternList parse_long(Tokens& tokens, dls::tableau<Option>& options)
{
	// long ::= '--' chars [ ( ' ' | '=' ) chars ] ;
	dls::chaine longOpt, equal, str_val;
	std::tie(longOpt, equal, str_val) = partition(tokens.pop(), "=");
	value val(str_val);

	assert(starts_with(longOpt, "--"));

	if (equal.est_vide()) {
		val = value{};
	}

	// detect with options match this long option
	dls::tableau<Option const*> similar;
	for(auto const& option : options) {
		if (option.longOption()==longOpt)
			similar.pousse(&option);
	}

	// maybe allow similar options that match by prefix
	if (tokens.isParsingArgv() && similar.est_vide()) {
		for(auto const& option : options) {
			if (option.longOption().est_vide())
				continue;
			if (starts_with(option.longOption(), longOpt))
				similar.pousse(&option);
		}
	}

	PatternList ret;

	if (similar.taille() > 1) { // might be simply specified ambiguously 2+ times?
		dls::tableau<dls::chaine> prefixes = longOptions(similar.debut(), similar.fin());
		auto error = "'" + longOpt + "' is not a unique prefix: ";
		error.append(join(prefixes.debut(), prefixes.fin(), ", "));
		throw Tokens::OptionError(error.c_str());
	} else if (similar.est_vide()) {
		int argcount = equal.est_vide() ? 0 : 1;
		options.emplace_back("", longOpt, argcount);

		auto o = std::make_shared<Option>(options.back());
		if (tokens.isParsingArgv()) {
			o->setValue(argcount ? value{val} : value{true});
		}
		ret.pousse(o);
	} else {
		auto o = std::make_shared<Option>(*similar[0]);
		if (o->argCount() == 0) {
			if (val) {
				auto error = o->longOption() + " must not have an argument";
				throw Tokens::OptionError(error.c_str());
			}
		} else {
			if (!val) {
				auto const& token = tokens.current();
				if (token.est_vide() || token=="--") {
					auto error = o->longOption() + " requires an argument";
					throw Tokens::OptionError(error.c_str());
				}
				val = value(tokens.pop());
			}
		}
		if (tokens.isParsingArgv()) {
			o->setValue(val ? std::move(val) : value{true});
		}
		ret.pousse(o);
	}

	return ret;
}

static PatternList parse_short(Tokens& tokens, dls::tableau<Option>& options)
{
	// shorts ::= '-' ( chars )* [ [ ' ' ] chars ] ;

	auto token = tokens.pop();

	assert(starts_with(token, "-"));
	assert(!starts_with(token, "--"));

	auto i = token.debut();
	++i; // skip the leading '-'

	PatternList ret;
	while (i != token.fin()) {
		dls::chaine shortOpt = { '-', *i };
		++i;

		dls::tableau<Option const*> similar;
		for(auto const& option : options) {
			if (option.shortOption()==shortOpt)
				similar.pousse(&option);
		}

		if (similar.taille() > 1) {
			auto error = shortOpt + " is specified ambiguously "
					+ std::to_string(similar.taille()) + " times";
			throw Tokens::OptionError(std::move(error.c_str()));
		} else if (similar.est_vide()) {
			options.emplace_back(shortOpt, "", 0);

			auto o = std::make_shared<Option>(options.back());
			if (tokens.isParsingArgv()) {
				o->setValue(value{true});
			}
			ret.pousse(o);
		} else {
			auto o = std::make_shared<Option>(*similar[0]);
			value val;
			if (o->argCount()) {
				if (i == token.fin()) {
					// consume the next token
					auto const& ttoken = tokens.current();
					if (ttoken.est_vide() || ttoken=="--") {
						dls::chaine error = shortOpt + " requires an argument";
						throw Tokens::OptionError(error.c_str());
					}
					val = value(tokens.pop());
				} else {
					// consume all the rest
					val = value(dls::chaine{i, token.fin()});
					i = token.fin();
				}
			}

			if (tokens.isParsingArgv()) {
				o->setValue(val ? std::move(val) : value{true});
			}
			ret.pousse(o);
		}
	}

	return ret;
}

static PatternList parse_expr(Tokens& tokens, dls::tableau<Option>& options);

static PatternList parse_atom(Tokens& tokens, dls::tableau<Option>& options)
{
	// atom ::= '(' expr ')' | '[' expr ']' | 'options'
	//             | long | shorts | argument | command ;

	auto const& token = tokens.current();

	PatternList ret;

	if (token == "[") {
		tokens.pop();

		auto expr = parse_expr(tokens, options);

		auto trailing = tokens.pop();
		if (trailing != "]") {
			throw DocoptLanguageError("Mismatched '['");
		}

		ret.emplace_back(std::make_shared<Optional>(std::move(expr)));
	} else if (token=="(") {
		tokens.pop();

		auto expr = parse_expr(tokens, options);

		auto trailing = tokens.pop();
		if (trailing != ")") {
			throw DocoptLanguageError("Mismatched '('");
		}

		ret.emplace_back(std::make_shared<Required>(std::move(expr)));
	} else if (token == "options") {
		tokens.pop();
		ret.emplace_back(std::make_shared<OptionsShortcut>());
	} else if (starts_with(token, "--") && token != "--") {
		ret = parse_long(tokens, options);
	} else if (starts_with(token, "-") && token != "-" && token != "--") {
		ret = parse_short(tokens, options);
	} else if (is_argument_spec(token)) {
		ret.emplace_back(std::make_shared<Argument>(tokens.pop()));
	} else {
		ret.emplace_back(std::make_shared<Command>(tokens.pop()));
	}

	return ret;
}

static PatternList parse_seq(Tokens& tokens, dls::tableau<Option>& options)
{
	// seq ::= ( atom [ '...' ] )* ;"""

	PatternList ret;

	while (tokens) {
		auto const& token = tokens.current();

		if (token=="]" || token==")" || token=="|")
			break;

		auto atom = parse_atom(tokens, options);
		if (tokens.current() == "...") {
			ret.emplace_back(std::make_shared<OneOrMore>(std::move(atom)));
			tokens.pop();
		} else {
			std::move(atom.debut(), atom.fin(), std::back_inserter(ret));
		}
	}

	return ret;
}

static std::shared_ptr<Pattern> maybe_collapse_to_required(PatternList&& seq)
{
	if (seq.taille()==1) {
		return std::move(seq[0]);
	}
	return std::make_shared<Required>(std::move(seq));
}

static std::shared_ptr<Pattern> maybe_collapse_to_either(PatternList&& seq)
{
	if (seq.taille()==1) {
		return std::move(seq[0]);
	}
	return std::make_shared<Either>(std::move(seq));
}

PatternList parse_expr(Tokens& tokens, dls::tableau<Option>& options)
{
	// expr ::= seq ( '|' seq )* ;

	auto seq = parse_seq(tokens, options);

	if (tokens.current() != "|")
		return seq;

	PatternList ret;
	ret.emplace_back(maybe_collapse_to_required(std::move(seq)));

	while (tokens.current() == "|") {
		tokens.pop();
		seq = parse_seq(tokens, options);
		ret.emplace_back(maybe_collapse_to_required(std::move(seq)));
	}

	return { maybe_collapse_to_either(std::move(ret)) };
}

static Required parse_pattern(dls::chaine const& source, dls::tableau<Option>& options)
{
	auto tokens = Tokens::from_pattern(source);
	auto result = parse_expr(tokens, options);

	if (tokens)
		throw DocoptLanguageError(("Unexpected ending: '" + tokens.the_rest() + "'").c_str());

	assert(result.taille() == 1  &&  "top level is always one big");
	return Required{ std::move(result) };
}


static dls::chaine formal_usage(dls::chaine const& section)
{
	auto ret = dls::chaine("(");

	auto i = section.trouve(':')+1;  // skip past "usage:"
	auto parts = split(section, i);
	for(auto ii = 1l; ii < parts.taille(); ++ii) {
		if (parts[ii] == parts[0]) {
			ret += " ) | (";
		} else {
			ret.pousse(' ');
			ret += parts[ii];
		}
	}

	ret += " )";
	return ret;
}

static PatternList parse_argv(Tokens tokens, dls::tableau<Option>& options, bool options_first)
{
	// Parse command-line argument vector.
	//
	// If options_first:
	//    argv ::= [ long | shorts ]* [ argument ]* [ '--' [ argument ]* ] ;
	// else:
	//    argv ::= [ long | shorts | argument ]* [ '--' [ argument ]* ] ;

	PatternList ret;
	while (tokens) {
		auto const& token = tokens.current();

		if (token=="--") {
			// option list is done; convert all the rest to arguments
			while (tokens) {
				ret.emplace_back(std::make_shared<Argument>("", value(tokens.pop())));
			}
		}
		else if (starts_with(token, "--")) {
			auto&& parsed = parse_long(tokens, options);
			std::move(parsed.debut(), parsed.fin(), std::back_inserter(ret));
		}
		else if (token[0]=='-' && token != "-") {
			auto&& parsed = parse_short(tokens, options);
			std::move(parsed.debut(), parsed.fin(), std::back_inserter(ret));
		}
		else if (options_first) {
			// option list is done; convert all the rest to arguments
			while (tokens) {
				ret.emplace_back(std::make_shared<Argument>("", value(tokens.pop())));
			}
		}
		else {
			ret.emplace_back(std::make_shared<Argument>("", value(tokens.pop())));
		}
	}

	return ret;
}

dls::tableau<Option> parse_defaults(dls::chaine const& doc)
{
	// This pattern is a delimiter by which we split the options.
	// The delimiter is a new line followed by a whitespace(s) followed by one or two hyphens.
	static std::regex const re_delimiter{
		"(?:^|\\n)[ \\t]*"  // a new line with leading whitespace
		"(?=-{1,2})"        // [split happens here] (positive lookahead) ... and followed by one or two hyphes
	};

	dls::tableau<Option> defaults;
	for (auto s : parse_section("options:", doc)) {
		s.efface(s.debut(), s.debut() + s.trouve(':') + 1); // get rid of "options:"

		for (const auto& opt : regex_split(s, re_delimiter)) {
			if (starts_with(opt, "-")) {
				defaults.emplace_back(Option::parse(opt));
			}
		}
	}

	return defaults;
}

static bool isOptionSet(PatternList const& options, dls::chaine const& opt1, dls::chaine const& opt2 = "")
{
	return std::any_of(options.debut(), options.fin(), [&](std::shared_ptr<Pattern const> const& opt) -> bool {
		auto const& name = opt->name();
		if (name==opt1 || (!opt2.est_vide() && name==opt2)) {
			return opt->hasValue();
		}
		return false;
	});
}

static void extras(bool help, bool version, PatternList const& options)
{
	if (help && isOptionSet(options, "-h", "--help")) {
		throw DocoptExitHelp();
	}

	if (version && isOptionSet(options, "--version")) {
		throw DocoptExitVersion();
	}
}

// Parse the doc string and generate the Pattern tree
static std::pair<Required, dls::tableau<Option>> create_pattern_tree(dls::chaine const& doc)
{
	auto usage_sections = parse_section("usage:", doc);
	if (usage_sections.est_vide()) {
		throw DocoptLanguageError("'usage:' (case-insensitive) not found.");
	}
	if (usage_sections.taille() > 1) {
		throw DocoptLanguageError("More than one 'usage:' (case-insensitive).");
	}

	dls::tableau<Option> options = parse_defaults(doc);
	Required pattern = parse_pattern(formal_usage(usage_sections[0]), options);

	dls::tableau<Option const*> pattern_options = flat_filter<Option const>(pattern);

	using UniqueOptions = std::unordered_set<Option const*, PatternHasher, PatternPointerEquality>;
	UniqueOptions const uniq_pattern_options { pattern_options.debut(), pattern_options.fin() };

	// Fix up any "[options]" shortcuts with the actual option tree
	for(auto& options_shortcut : flat_filter<OptionsShortcut>(pattern)) {
		dls::tableau<Option> doc_options = parse_defaults(doc);

		// set(doc_options) - set(pattern_options)
		UniqueOptions uniq_doc_options;
		for(auto const& opt : doc_options) {
			if (uniq_pattern_options.count(&opt))
				continue;
			uniq_doc_options.insert(&opt);
		}

		// turn into shared_ptr's and set as children
		PatternList children;
		std::transform(uniq_doc_options.begin(), uniq_doc_options.end(),
					   std::back_inserter(children), [](Option const* opt) {
			return std::make_shared<Option>(*opt);
		});
		options_shortcut->setChildren(std::move(children));
	}

	return { std::move(pattern), std::move(options) };
}

dls::dico<dls::chaine, value>
docopt_parse(dls::chaine const& doc,
					 dls::tableau<dls::chaine> const& argv,
					 bool help,
					 bool version,
					 bool options_first)
{
	Required pattern;
	dls::tableau<Option> options;
	try {
		std::tie(pattern, options) = create_pattern_tree(doc);
	} catch (Tokens::OptionError const& error) {
		throw DocoptLanguageError(error.what());
	}

	PatternList argv_patterns;
	try {
		argv_patterns = parse_argv(Tokens(argv), options, options_first);
	} catch (Tokens::OptionError const& error) {
		throw DocoptArgumentError(error.what());
	}

	extras(help, version, argv_patterns);

	dls::tableau<std::shared_ptr<LeafPattern>> collected;
	bool matched = pattern.fix().match(argv_patterns, collected);
	if (matched && argv_patterns.est_vide()) {
		dls::dico<dls::chaine, value> ret;

		// (a.name, a.value) for a in (pattern.flat() + collected)
		for (auto* p : pattern.leaves()) {
			ret[p->name()] = p->getValue();
		}

		for (auto const& p : collected) {
			ret[p->name()] = p->getValue();
		}

		return ret;
	}

	if (matched) {
		auto leftover = join(argv.debut(), argv.fin(), ", ");
		throw DocoptArgumentError(("Unexpected argument: " + leftover).c_str());
	}

	throw DocoptArgumentError("Arguments did not match expected patterns"); // BLEH. Bad error.
}

dls::dico<dls::chaine, value>
docopt(dls::chaine const& doc,
			   dls::tableau<dls::chaine> const& argv,
			   bool help,
			   dls::chaine const& version,
			   bool options_first) noexcept
{
	try {
		return docopt_parse(doc, argv, help, !version.est_vide(), options_first);
	}
	catch (DocoptExitHelp const&) {
		std::cout << doc << std::endl;
		std::exit(0);
	}
	catch (DocoptExitVersion const&) {
		std::cout << version << std::endl;
		std::exit(0);
	}
	catch (DocoptLanguageError const& error) {
		std::cerr << "Docopt usage string could not be parsed" << std::endl;
		std::cerr << error.what() << std::endl;
		std::exit(-1);
	}
	catch (DocoptArgumentError const& error) {
		std::cerr << error.what();
		std::cout << std::endl;
		std::cout << doc << std::endl;
		std::exit(-1);
	} /* Any other exception is unexpected: let std::terminate grab it */
}

bool get_bool(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept
{
	const auto iter = args.trouve(nom_argument);

	if (iter == args.fin()) {
		return false;
	}

	return (*iter).second.asBool();
}

long get_long(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept
{
	const auto iter = args.trouve(nom_argument);

	if (iter == args.fin()) {
		return 0ul;
	}

	return (*iter).second.asLong();
}

dls::chaine get_string(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept
{
	const auto iter = args.trouve(nom_argument);

	if (iter == args.fin()) {
		return {};
	}

	return (*iter).second.asString();
}

dls::tableau<dls::chaine> get_string_list(
		const dls::dico<dls::chaine, value> &args,
		const dls::chaine &nom_argument) noexcept
{
	const auto iter = args.trouve(nom_argument);

	if (iter == args.fin()) {
		return {};
	}

	return (*iter).second.asStringList();
}

}  /* namespace docopt */
}  /* namespace dls */

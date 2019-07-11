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

#pragma once

#include <cassert>
#include <memory>
#include <regex>
#include <unordered_set>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "docopt_value.hh"

namespace dls {
namespace docopt {

class Pattern;
class LeafPattern;

using PatternList = dls::tableau<std::shared_ptr<Pattern>>;

// Utility to use Pattern types in std hash-containers
struct PatternHasher {
	template <typename P>
	size_t operator()(const std::shared_ptr<P> &pattern) const
	{
		return pattern->hash();
	}

	template <typename P>
	size_t operator()(const P *pattern) const
	{
		return pattern->hash();
	}

	template <typename P>
	size_t operator()(const P & pattern) const
	{
		return pattern.hash();
	}
};

// Utility to use 'hash' as the equality operator as well in std containers
struct PatternPointerEquality {
	template <typename P1, typename P2>
	bool operator()(const std::shared_ptr<P1> & p1, const std::shared_ptr<P2> & p2) const
	{
		return p1->hash()==p2->hash();
	}

	template <typename P1, typename P2>
	bool operator()(const P1 * p1, const P2 * p2) const
	{
		return p1->hash()==p2->hash();
	}
};

// A hash-set that uniques by hash value
using UniquePatternSet = std::unordered_set<std::shared_ptr<Pattern>, PatternHasher, PatternPointerEquality>;


class Pattern {
public:
	// flatten out children, stopping descent when the given filter returns 'true'
	virtual dls::tableau<Pattern *> flat(bool (*filter)(const Pattern *)) = 0;

	// flatten out all children into a list of LeafPattern objects
	virtual void collect_leaves(dls::tableau<LeafPattern*> &) = 0;

	// flatten out all children into a list of LeafPattern objects
	dls::tableau<LeafPattern*> leaves();

	// Attempt to find something in 'left' that matches this pattern's spec, and if so, move it to 'collected'
	virtual bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const = 0;

	virtual const dls::chaine & name() const = 0;

	virtual bool hasValue() const { return false; }

	virtual size_t hash() const = 0;

	virtual ~Pattern() = default;
};

class LeafPattern : public Pattern {
	dls::chaine fName;
	value fValue;

public:
	LeafPattern(const dls::chaine &name, const value &v = {})
		: fName(name)
		, fValue(v)
	{}

	virtual dls::tableau<Pattern*> flat(bool (*filter)(const Pattern *)) override
	{
		if (filter(this)) {
			return { this };
		}

		return {};
	}

	virtual void collect_leaves(dls::tableau<LeafPattern*>& lst) override final
	{
		lst.pousse(this);
	}

	virtual bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const override;

	virtual bool hasValue() const override { return static_cast<bool>(fValue); }

	value const& getValue() const { return fValue; }
	void setValue(value&& v) { fValue = std::move(v); }

	virtual dls::chaine const& name() const override { return fName; }

	virtual size_t hash() const override
	{
		size_t seed = typeid(*this).hash_code();
		hash_combine(seed, fName);
		hash_combine(seed, fValue);
		return seed;
	}

protected:
	virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const&) const = 0;
};

class BranchPattern : public Pattern {
	void fix_repeating_arguments();

public:
	explicit BranchPattern(PatternList children = {})
		: fChildren(std::move(children))
	{}

	Pattern& fix()
	{
		UniquePatternSet patterns;
		fix_identities(patterns);
		fix_repeating_arguments();
		return *this;
	}

	virtual dls::chaine const& name() const override
	{
		throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
	}

	virtual value const& getValue() const
	{
		throw std::runtime_error("Logic error: name() shouldnt be called on a BranchPattern");
	}

	virtual dls::tableau<Pattern*> flat(bool (*filter)(Pattern const*)) override
	{
		if (filter(this)) {
			return {this};
		}

		dls::tableau<Pattern*> ret;

		for (auto& child : fChildren) {
			auto sublist = child->flat(filter);
			ret.insere(ret.fin(), sublist.debut(), sublist.fin());
		}

		return ret;
	}

	virtual void collect_leaves(dls::tableau<LeafPattern*>& lst) override final
	{
		for (auto& child : fChildren) {
			child->collect_leaves(lst);
		}
	}

	void setChildren(PatternList children)
	{
		fChildren = std::move(children);
	}

	PatternList const& children() const { return fChildren; }

	virtual void fix_identities(UniquePatternSet& patterns)
	{
		for (auto& child : fChildren) {
			// this will fix up all its children, if needed
			if (auto bp = dynamic_cast<BranchPattern*>(child.get())) {
				bp->fix_identities(patterns);
			}

			// then we try to add it to the list
			auto insereed = patterns.insere(child);

			if (!insereed.second) {
				// already there? then reuse the existing shared_ptr for that thing
				child = *insereed.first;
			}
		}
	}

	virtual size_t hash() const override
	{
		auto seed = typeid(*this).hash_code();
		hash_combine(seed, fChildren.taille());

		for (auto const& child : fChildren) {
			hash_combine(seed, child->hash());
		}

		return seed;
	}

protected:
	PatternList fChildren;
};

class Argument : public LeafPattern {
public:
	using LeafPattern::LeafPattern;

protected:
	virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
};

class Command : public Argument {
public:
	explicit Command(const dls::chaine &name, const value &v = value{false})
		: Argument(name, v)
	{}

protected:
	virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;
};

class Option final : public LeafPattern {
public:
	static Option parse(dls::chaine const& option_description);

	Option(const dls::chaine &shortOption,
		   const dls::chaine &longOption,
		   int argcount = 0,
		   const value &v = value{false})
		: LeafPattern(longOption.est_vide() ? shortOption : longOption,
					  std::move(v)),
		  fShortOption(std::move(shortOption)),
		  fLongOption(std::move(longOption)),
		  fArgcount(argcount)
	{
		// From Python:
		//   self.value = None if value is False and argcount else value
		if (argcount && v.isBool() && !v.asBool()) {
			setValue(value{});
		}
	}

	Option(Option const&) = default;
	Option(Option&&) = default;
	Option& operator=(Option const&) = default;
	Option& operator=(Option&&) = default;

	using LeafPattern::setValue;

	dls::chaine const& longOption() const { return fLongOption; }
	dls::chaine const& shortOption() const { return fShortOption; }
	int argCount() const { return fArgcount; }

	virtual size_t hash() const override {
		size_t seed = LeafPattern::hash();
		hash_combine(seed, fShortOption);
		hash_combine(seed, fLongOption);
		hash_combine(seed, fArgcount);
		return seed;
	}

protected:
	virtual std::pair<size_t, std::shared_ptr<LeafPattern>> single_match(PatternList const& left) const override;

private:
	dls::chaine fShortOption;
	dls::chaine fLongOption;
	int fArgcount;
};

class Required : public BranchPattern {
public:
	using BranchPattern::BranchPattern;

	bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const override;
};

class Optional : public BranchPattern {
public:
	using BranchPattern::BranchPattern;

	bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const override {
		for (auto const& pattern : fChildren) {
			pattern->match(left, collected);
		}
		return true;
	}
};

class OptionsShortcut : public Optional {
	using Optional::Optional;
};

class OneOrMore : public BranchPattern {
public:
	using BranchPattern::BranchPattern;

	bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const override;
};

class Either : public BranchPattern {
public:
	using BranchPattern::BranchPattern;

	bool match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const override;
};

#pragma mark -
#pragma mark inline implementations

inline dls::tableau<LeafPattern*> Pattern::leaves()
{
	dls::tableau<LeafPattern*> ret;
	collect_leaves(ret);
	return ret;
}

static inline dls::tableau<PatternList> transform(PatternList pattern)
{
	dls::tableau<PatternList> result;

	dls::tableau<PatternList> groups;
	groups.emplace_back(std::move(pattern));

	while(!groups.est_vide()) {
		// pop off the first element
		auto children = std::move(groups[0]);
		groups.erase(groups.debut());

		// find the first branch node in the list
		auto child_iter = std::find_if(children.debut(), children.fin(), [](std::shared_ptr<Pattern> const& p) {
				return dynamic_cast<BranchPattern const*>(p.get());
	});

		// no branch nodes left : expansion is complete for this grouping
		if (child_iter == children.fin()) {
			result.emplace_back(std::move(children));
			continue;
		}

		// pop the child from the list
		auto child = std::move(*child_iter);
		children.erase(child_iter);

		// expand the branch in the appropriate way
		if (Either* either = dynamic_cast<Either*>(child.get())) {
			// "[e] + children" for each child 'e' in Either
			for (auto const& eitherChild : either->children()) {
				PatternList group = { eitherChild };
				group.insere(group.fin(), children.debut(), children.fin());

				groups.emplace_back(std::move(group));
			}
		} else if (OneOrMore* oneOrMore = dynamic_cast<OneOrMore*>(child.get())) {
			// child.children * 2 + children
			auto const& subchildren = oneOrMore->children();
			PatternList group = subchildren;
			group.insere(group.fin(), subchildren.debut(), subchildren.fin());
			group.insere(group.fin(), children.debut(), children.fin());

			groups.emplace_back(std::move(group));
		} else { // Required, Optional, OptionsShortcut
			BranchPattern* branch = dynamic_cast<BranchPattern*>(child.get());

			// child.children + children
			PatternList group = branch->children();
			group.insere(group.fin(), children.debut(), children.fin());

			groups.emplace_back(std::move(group));
		}
	}

	return result;
}

inline void BranchPattern::fix_repeating_arguments()
{
	dls::tableau<PatternList> either = transform(children());
	for (auto const& group : either) {
		// use multiset to help identify duplicate entries
		std::unordered_multiset<std::shared_ptr<Pattern>, PatternHasher> group_set {group.debut(), group.fin()};
		for (auto const& e : group_set) {
			if (group_set.count(e) == 1)
				continue;

			LeafPattern* leaf = dynamic_cast<LeafPattern*>(e.get());
			if (!leaf) continue;

			bool ensureList = false;
			bool ensureInt = false;

			if (dynamic_cast<Command*>(leaf)) {
				ensureInt = true;
			} else if (dynamic_cast<Argument*>(leaf)) {
				ensureList = true;
			} else if (Option* o = dynamic_cast<Option*>(leaf)) {
				if (o->argCount()) {
					ensureList = true;
				} else {
					ensureInt = true;
				}
			}

			if (ensureList) {
				dls::tableau<dls::chaine> newValue;
				if (leaf->getValue().isString()) {
					newValue = split(leaf->getValue().asString());
				}
				if (!leaf->getValue().isStringList()) {
					leaf->setValue(value{newValue});
				}
			} else if (ensureInt) {
				leaf->setValue(value{0});
			}
		}
	}
}

inline bool LeafPattern::match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const
{
	auto match = single_match(left);
	if (!match.second) {
		return false;
	}

	left.erase(left.debut()+static_cast<std::ptrdiff_t>(match.first));

	auto same_name = std::find_if(collected.debut(), collected.fin(), [&](std::shared_ptr<LeafPattern> const& p) {
			return p->name()==name();
});
	if (getValue().isLong()) {
		long val = 1;
		if (same_name == collected.fin()) {
			collected.pousse(match.second);
			match.second->setValue(value{val});
		} else if ((**same_name).getValue().isLong()) {
			val += (**same_name).getValue().asLong();
			(**same_name).setValue(value{val});
		} else {
			(**same_name).setValue(value{val});
		}
	} else if (getValue().isStringList()) {
		dls::tableau<dls::chaine> val;
		if (match.second->getValue().isString()) {
			val.pousse(match.second->getValue().asString());
		} else if (match.second->getValue().isStringList()) {
			val = match.second->getValue().asStringList();
		} else {
			/// cant be!?
		}

		if (same_name == collected.fin()) {
			collected.pousse(match.second);
			match.second->setValue(value{val});
		} else if ((**same_name).getValue().isStringList()) {
			dls::tableau<dls::chaine> const& list = (**same_name).getValue().asStringList();
			val.insere(val.debut(), list.debut(), list.fin());
			(**same_name).setValue(value{val});
		} else {
			(**same_name).setValue(value{val});
		}
	} else {
		collected.pousse(match.second);
	}
	return true;
}

inline std::pair<size_t, std::shared_ptr<LeafPattern>> Argument::single_match(PatternList const& left) const
{
	std::pair<size_t, std::shared_ptr<LeafPattern>> ret {};

	for (size_t i = 0, size = left.taille(); i < size; ++i)
	{
		auto arg = dynamic_cast<Argument const*>(left[i].get());
		if (arg) {
			ret.first = i;
			ret.second = std::make_shared<Argument>(name(), arg->getValue());
			break;
		}
	}

	return ret;
}

inline std::pair<size_t, std::shared_ptr<LeafPattern>> Command::single_match(PatternList const& left) const
{
	std::pair<size_t, std::shared_ptr<LeafPattern>> ret {};

	for (size_t i = 0, size = left.taille(); i < size; ++i)
	{
		auto arg = dynamic_cast<Argument const*>(left[i].get());
		if (arg) {
			if (value(name()) == arg->getValue()) {
				ret.first = i;
				ret.second = std::make_shared<Command>(name(), value{true});
			}
			break;
		}
	}

	return ret;
}

inline Option Option::parse(dls::chaine const& option_description)
{
	dls::chaine shortOption, longOption;
	int argcount = 0;
	value val { false };

	auto double_space = option_description.trouve("  ");
	auto options_end = option_description.fin();
	if (double_space != dls::chaine::npos) {
		options_end = option_description.debut() + static_cast<std::ptrdiff_t>(double_space);
	}

	static const std::regex pattern {"(-{1,2})?(.*?)([,= ]|$)"};
	for (std::sregex_iterator i {option_description.debut(), options_end, pattern, std::regex_constants::match_not_null},
		e{};
		i != e;
		++i)
	{
		std::smatch const& match = *i;
		if (match[1].matched) { // [1] is optional.
			if (match[1].length()==1) {
				shortOption = "-" + match[2].str();
			}
			else {
				longOption =  "--" + match[2].str();
			}
		}
		else if (match[2].length() > 0) { // [2] always matches.
			dls::chaine m = match[2];
			argcount = 1;
		}
		else {
			// delimeter
		}

		if (match[3].length() == 0) { // [3] always matches.
			// Hit end of string. For some reason 'match_not_null' will let us match empty
			// at the end, and then we'll spin in an infinite loop. So, if we hit an empty
			// match, we know we must be at the end.
			break;
		}
	}

	if (argcount) {
		std::smatch match;
		if (std::regex_search(options_end, option_description.fin(),
							  match,
							  std::regex{"\\[default: (.*)\\]", std::regex::icase}))
		{
			val = value(match[1].str());
		}
	}

	return {
		std::move(shortOption),
		std::move(longOption),
		argcount,
		std::move(val)
	};
}

inline std::pair<size_t, std::shared_ptr<LeafPattern>> Option::single_match(PatternList const& left) const
{
	auto thematch = find_if(left.debut(), left.fin(), [this](std::shared_ptr<Pattern> const& a)
	{
			auto leaf = std::dynamic_pointer_cast<LeafPattern>(a);
			return leaf && this->name() == leaf->name();
	});

	if (thematch == left.fin()) {
		return {};
	}

	return {
		std::distance(left.debut(), thematch),
		std::dynamic_pointer_cast<LeafPattern>(*thematch)
	};
}

inline bool Required::match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const
{
	auto l = left;
	auto c = collected;

	for (auto const& pattern : fChildren) {
		bool ret = pattern->match(l, c);

		if (!ret) {
			// leave (left, collected) untouched
			return false;
		}
	}

	left = std::move(l);
	collected = std::move(c);

	return true;
}

inline bool OneOrMore::match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const
{
	assert(fChildren.taille() == 1);

	auto l = left;
	auto c = collected;

	bool matched = true;
	size_t times = 0;

	decltype(l) l_;
	bool firstLoop = true;

	while (matched) {
		// could it be that something didn't match but changed l or c?
		matched = fChildren[0]->match(l, c);

		if (matched)
			++times;

		if (firstLoop) {
			firstLoop = false;
		} else if (l == l_) {
			break;
		}

		l_ = l;
	}

	if (times == 0) {
		return false;
	}

	left = std::move(l);
	collected = std::move(c);
	return true;
}

inline bool Either::match(PatternList& left, dls::tableau<std::shared_ptr<LeafPattern>>& collected) const
{
	using Outcome = std::pair<PatternList, dls::tableau<std::shared_ptr<LeafPattern>>>;

	dls::tableau<Outcome> outcomes;

	for (auto const& pattern : fChildren) {
		// need a copy so we apply the same one for every iteration
		auto l = left;
		auto c = collected;
		bool matched = pattern->match(l, c);
		if (matched) {
			outcomes.emplace_back(std::move(l), std::move(c));
		}
	}

	auto min = std::min_element(outcomes.debut(), outcomes.fin(),
								[](Outcome const& o1, Outcome const& o2)
	{
			return o1.first.taille() < o2.first.taille();
	});

	if (min == outcomes.fin()) {
		// (left, collected) unchanged
		return false;
	}

	std::tie(left, collected) = std::move(*min);
	return true;
}

}  /* namespace docopt */
}  /* namespace dls */

#include "rangee.hh"

#include <cstdio>
#include <cstring>

struct RangeeEntreeChar {
	RangeeEntreeChar(char const *d, char const *f)
		: debut(d)
		, fin(f)
	{}

	char front() const
	{
		return *debut;
	}

	void effronte()
	{
		++debut;
	}

	bool vide() const
	{
		return debut == fin;
	}

	char cul() const
	{
		return *(fin - 1);
	}

	void ecule()
	{
		--fin;
	}

	RangeeEntreeChar sauvegarde()
	{
		return RangeeEntreeChar{debut, fin};
	}

	using type_valeur = char;

private:
	char const *debut;
	char const *fin;
};

struct RangeeSortieChar {
	using type_valeur = char;

	void pousse(char c)
	{
		putc(c, stdout);
	}
};

int main()
{
	auto str = "Salut, tout le monde !\n";

	auto entree = RangeeEntreeChar(str, str + std::strlen(str));
	auto sortie = RangeeSortieChar{};

	copie(entree, sortie);

	auto entree2 = trouve(entree, 't');

	copie(entree2, sortie);

	auto entree3 = trouve_si(entree, [](char c) { return c == '\n'; });

	copie(entree3, sortie);

	return 0;
}

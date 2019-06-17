#include <iostream>

#define soit auto

template <typename T>
auto porte_et(T a, T b) -> T
{
	return (a & b);
}

template <typename T>
auto porte_ou(T a, T b) -> T
{
	return (a | b);
}

template <typename T>
auto porte_ou_ex(T a, T b) -> T
{
	return (a ^ b);
}

template <typename T>
auto porte_pas(T a) -> T
{
	return ~a;
}

template <typename T>
auto demi_ajouteur(T a, T b) -> std::pair<T, T>
{
	soit somme = porte_ou_ex(a, b);
	soit retenue = porte_et(a, b);
	return { somme, retenue };
}

template <typename T>
auto plein_ajouteur(T a, T b, T r_entree) -> std::pair<T, T>
{
	soit xor_ab = porte_ou_ex(a, b);
	soit somme = porte_ou_ex(xor_ab, r_entree);

	soit et_ab_c = porte_et(xor_ab, r_entree);
	soit et_ab = porte_et(a, b);
	soit retenue = porte_ou(et_ab_c, et_ab);

	return { somme, retenue };
}

template <typename T>
auto ajoute(T a, T b) -> T
{
	soit somme = 0;
	soit retenue = false;
	soit resultat = static_cast<T>(0);

	for (int i = 0; i < (sizeof(T) * 8); ++i) {
		soit paire = plein_ajouteur((a & (1 << i)) != 0, (b & (1 << i)) != 0, retenue);
		somme = paire.first;
		retenue = paire.second != 0;

		resultat |= (somme << i);
	}

	return resultat;
}

template <typename T>
auto plein_soutracteur(T a, T b, T c) -> std::pair<T, T>
{
	soit xor1 = porte_ou_ex(a, b);
	soit difference = porte_ou_ex(c, xor1);

	soit not1 = porte_pas(a);
	soit and1 = porte_et(not1, b);

	soit not2 = porte_pas(xor1);
	soit and2 = porte_et(c, not2);

	soit emprunt = porte_ou(and1, and2);

	return { difference, emprunt };
}

template <typename T>
auto soustrait(T a, T b) -> T
{
	soit somme = 0;
	soit retenue = T(0);
	soit resultat = static_cast<T>(0);

	for (int i = 0; i < (sizeof(T) * 8); ++i) {
		soit paire = plein_soutracteur((a & (1 << i)) >> i, (b & (1 << i)) >> i, retenue);
		somme = paire.first;
		retenue = paire.second;

		resultat |= (somme << i);
	}

	return resultat;
}

template <typename T>
auto multiplie(T a, T b) -> T
{
	soit resultat = 0;

	while (b-- > 0) {
		resultat = ajoute(resultat, a);
	}

	return resultat;
}

template <typename T>
auto divise(T a, T b) -> T
{
	auto resultat = 0;

	while (b <= a) {
		a = soustrait(a, b);
		resultat = ajoute(1, resultat);
	}

	return resultat;
}

int main(int argc, char **argv)
{
	auto a = 30;
	auto b = argc;

	auto res = divise(a, b);

	std::cout << a << " / " << b << " = " << res << '\n';

	return 0;
}

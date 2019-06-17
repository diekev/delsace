- x : x
- xy : x + xy
- xyz : x + xy + xyz
- xyzw : x + xy + xyz + xyzw


template <typename T, T... ts>
struct dimension {

	std::array<int, sizeof(...)> valeurs;

	dimension(T... ts)
	: array(std::forward(ts...))
	{
	}

	size_t nombre_element()
	{
		return std::inner_product(valeurs.begin(), valeurs.end());
	}
};

template <typename T, int N>
struct matrix {
	matrix(dimension<T, N> dim)
	{
		auto taille = dim.nombre_element();
	}

	T &operator[dimension<T, N> d];
};

// Example program
#include <iostream>
#include <string>
#include <algorithm>
#include <array>

template <typename T, T... ts>
struct dimensions {

    std::array<T, sizeof...(ts)> valeurs;

    dimensions()
        : valeurs({ts...})
    {}

    dimensions(T &&ns...)
        : valeurs(ns...)
    {}

    T nombre_elements()
    {
        return std::accumulate(valeurs.begin(), valeurs.end(), 1, [](T a, T b){ return a*b;});
    }

    //template <T... ns>
    T index(T &&ns...)
    {
        static_assert(sizeof...(ns) == sizeof...(ts), "");
        return 0;
    }
};

int main()
{
     auto dim = dimensions<int, 32, 32, 21>();
     //std::cout << dim.valeurs.size();

     for (auto v : dim.valeurs) {
         std::cout << v << '\n';
     }

     std::cout << dim.nombre_elements() << '\n';
}

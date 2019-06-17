#include <iostream>
#include <map>
#include <string>

int main()
{
	const std::string texte = "début {% si variable %} milieu {% finsi %} fin {{ variable }}";

	std::map<std::string, std::string> donnees = {
		{ "variable", "valeur" }
	};

	std::string nom_variable;
	std::string resultat;

	for (size_t i = 0; i < texte.size(); ++i) {
		if (texte[i] == '{' && texte[i + 1] == '{') {
			i += 2;

			while (texte[i] == ' ') {
				++i;
			}

			nom_variable.clear();

			while (texte[i] != ' ') {
				nom_variable += texte[i];
				++i;
			}

			std::cerr << "Nom variable '" << nom_variable << "'\n";

			auto iter = donnees.find(nom_variable);

			if (iter != donnees.end()) {
				resultat += iter->second;
			}

			while (texte[i] != '}') {
				++i;
			}

			if (texte[i] != '}' && texte[i + 1] != '}') {
				// erreur
			}

			i += 1;
			continue;
		}

		if (texte[i] == '{' && texte[i + 1] == '%') {
			i += 2;

			while (texte[i] == ' ') {
				++i;
			}

			if (texte[i] != 's') {
				// erreur
			}
			i += 2;

			while (texte[i] == ' ') {
				++i;
			}


			nom_variable.clear();

			while (texte[i] != ' ') {
				nom_variable += texte[i];
				++i;
			}

			auto succes = donnees.find(nom_variable) != donnees.end();

			while (texte[i] != '%') {
				++i;
			}

			if (texte[i] != '%' && texte[i + 1] != '}') {
				// erreur
			}

			i += 2;

			if (succes) {
				while (!(texte[i] == '{' && texte[i + 1] == '%')) {
					resultat += texte[i];
					++i;
				}

				i+=2;
			}

			while (!(texte[i] == '%' && texte[i + 1] == '}')) {
				++i;
			}

			i+=2;
			continue;
		}

		resultat += texte[i];
	}

	std::cerr << "texte    : " << texte << '\n';
	std::cerr << "Résultat : " << resultat << '\n';

	return 0;
}

/*
 * ***** DÉBUT BLOQUE LICENCE GPL *****
 *
 * Ce programme est un logiciel libre ; vous pouvez le redistribuer et/ou le
 * modifier sous les termes de la Licence Publique Générale GNU telle que
 * publiée par la Free Software Foundation ; soit la version 2 de la License,
 * ou (à votre option) une version ultérieure.
 *
 * Ce programme est distribué dans l'espoir qu'il sera util, mais SANS AUCUNE
 * GARANTIE ; sans même la garantie implicite de COMMERCIALISATION ou
 * d'ADAPTATION À UN USAGE PARTICULIER. Réferez-vous à la Licence Publique
 * Générale GNU pour plus amples détails.
 *
 * Vous auriez dû recevoir une copie de la Licence Générale Publique GNU avec ce
 * programme ; sinon, veuillez écrire à l'adresse suivante :
 *
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301
 * USA.
 *
 * Les droits d'auteur du Code Original appartiennent à Kévin Dietrich.
 * Tous droits réservés.
 *
 * ***** FIN BLOQUE LICENCE GPL *****
 */

#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../../../utils/io/io_utils.h"

/**
 * Ce programme sert à extraire jusq'à 10 000 mots aléatoires depuis une liste
 * des mots appartenant à une langue donnée.
 */

static constexpr auto PLAFOND = 10000;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Utilisation:\n" << argv[0] << " langue" << '\n';
		return 1;
	}

	std::string entree("../listes_pleines/");
	entree += argv[1];
	entree += ".txt";

	std::string sortie("../échantillons/");
	sortie += argv[1];
	sortie += ".txt";

	std::mt19937 generateur(19937);
	std::uniform_real_distribution<double> dist(0.0, 1.0);

	std::vector<std::string> mots;
	mots.reserve(PLAFOND);

	std::ifstream fichier_entree(entree.c_str());

	std::cerr << "Fichier d'entrée : " << entree << '\n';

	if  (!fichier_entree.is_open()) {
		std::cerr << "Fichier d'entrée introuvable !\n";
		return 1;
	}

	auto nombre_lignes = 0ul;

	io::foreach_line(fichier_entree, [&](const std::string &)
			{
		        ++nombre_lignes;
			});

	const auto limite = static_cast<double>(PLAFOND) / static_cast<double>(nombre_lignes);

	fichier_entree.clear();
	fichier_entree.seekg(0);

	auto plafond_longueur = std::numeric_limits<size_t>::min();

	io::foreach_line(fichier_entree, [&](const std::string &mot)
			{
		        if (mots.size() == PLAFOND) {
					return;
				}

				if (dist(generateur) > limite) {
					return;
				}

				if (mot.size() > plafond_longueur) {
					plafond_longueur = mot.size();
				}

				mots.push_back(mot);
			});

	std::cerr << "Longueur du plus long mot : " << plafond_longueur << '\n';

	fichier_entree.close();

	std::ofstream fichier_sortie(sortie.c_str());

	std::cerr << "Fichier de sortie : " << sortie << '\n';

	for (const auto &mot : mots) {
		fichier_sortie << mot << '\n';
	}
}

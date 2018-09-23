#include <iostream>
#include <string>

#if 0
static const MASQUE_0 = 0x3f;
static const MASQUE_1 = 0x1f << 6;
static const MASQUE_2 = 0x0f << 12;
static const MASQUE_3 = 0x07 << 18;

class chaine_utf8 {
    std::vector<int> donnees;

public:

};
#endif

const std::string lettres = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ,?;.:/!-*0123456789";

const std::string lettres_accent[] = {
    "É", "Ê", "È", "À", "Â", "Ï", "Î", "Ô", "Ù", "Ü",
    "é", "ê", "è", "à", "â", "ï", "î", "ô", "ù", "ü",
};

void imprime_lettres(std::ostream &os)
{
    os << '"';
    for (const auto &l : lettres) {
        os << l;
    }
    for (const auto &l : lettres_accent) {
        os << l;
    }
    os << '"';
    os << '\n';
}

void imprime_paire_lettre_valeur(std::ostream &os)
{
    os << "static std::map<int, int> table_uv_texte({\n\t";

    auto index = 0;

    for (const auto &l : lettres) {
        os << '{' << static_cast<int>(l) << ", " << index++ << "}, ";
    }

    for (const auto &lettre : lettres_accent) {
        int valeur;

        if (static_cast<unsigned char>(lettre[0]) < 224 && 1 < lettre.size() && static_cast<unsigned char>(lettre[1]) > 127) {
            // double byte character
            valeur = ((static_cast<unsigned char>(lettre[0]) & 0x1f) << 6) | (static_cast<unsigned char>(lettre[1]) & 0x3f);
        }
        else if (static_cast<unsigned char>(lettre[0]) < 240 && 2 < lettre.size() && lettre[1] > 127 && lettre[2] > 127) {
            // triple byte character
            valeur = ((static_cast<unsigned char>(lettre[0]) & 0x0f) << 12) | ((static_cast<unsigned char>(lettre[1]) & 0x1f) << 6) | (static_cast<unsigned char>(lettre[2]) & 0x3f);

        }
        else if (static_cast<unsigned char>(lettre[0]) < 248 && 3 < lettre.size() && lettre[1] > 127 && lettre[2] > 127 && lettre[3] > 127) {
            // 4-byte character
            valeur = ((static_cast<unsigned char>(lettre[0]) & 0x07) << 18) | ((static_cast<unsigned char>(lettre[1]) & 0x0f) << 12) | ((static_cast<unsigned char>(lettre[2]) & 0x1f) << 6) | (static_cast<unsigned char>(lettre[3]) & 0x3f);
        }
        else {
            continue;
        }

        os << '{' << valeur << ", " << index++ << "}, ";
    }
    os << "\n});\n";
}

size_t taille_chaine(const std::string &chaine)
{
    size_t taille = 0;

    for (size_t i = 0; i < chaine.size();) {
        if (static_cast<unsigned char>(chaine[i]) < 192) {
            taille += 1;
            i += 1;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 224 && i + 1 < chaine.size() && static_cast<unsigned char>(chaine[i + 1]) > 127) {
            // double byte character
            //*out++ = ((*it & 0x1F) << 6) | (*(it+1) & 0x3F);
            //it += 2;
            int valeur = ((static_cast<unsigned char>(chaine[i]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 1]) & 0x3f);
            std::cerr << "valeur : " << valeur << '\n';
            taille += 1;
            i += 2;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 240 && i + 2 < chaine.size() && chaine[i + 1] > 127 && chaine[i + 2] > 127) {
            // triple byte character
            //*out++ = ((*it & 0x0F) << 12) | ((*(it+1) & 0x3F) << 6) | (*(it+2) & 0x3F);
            //it += 3;
            taille += 1;
            i += 3;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 248 && i + 3 < chaine.size() && chaine[i + 1] > 127 && chaine[i + 2] > 127 && chaine[i + 3] > 127) {
            // 4-byte character
           // *out++ = ((*it & 0x07) << 18) | ((*(it+1) & 0x3F) << 12) | ((*(it+2) & 0x3F) << 6) | (*(it+3) & 0x3F);
          //  it += 4;
            taille += 1;
            i += 4;
        }
    }

    return taille;
}

void index_lettre(const std::string &chaine, std::ostream &os)
{
    for (size_t i = 0; i < chaine.size();) {
        int valeur;
        if (static_cast<unsigned char>(chaine[i]) < 192) {
            valeur = static_cast<int>(chaine[i]);
            i += 1;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 224 && i + 1 < chaine.size() && static_cast<unsigned char>(chaine[i + 1]) > 127) {
            // double byte character
            valeur = ((static_cast<unsigned char>(chaine[i]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 1]) & 0x3f);
            i += 2;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 240 && i + 2 < chaine.size() && chaine[i + 1] > 127 && chaine[i + 2] > 127) {
            // triple byte character
            valeur = ((static_cast<unsigned char>(chaine[i]) & 0x0f) << 12) | ((static_cast<unsigned char>(chaine[i + 1]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 2]) & 0x3f);

            i += 3;
        }
        else if (static_cast<unsigned char>(chaine[i]) < 248 && i + 3 < chaine.size() && chaine[i + 1] > 127 && chaine[i + 2] > 127 && chaine[i + 3] > 127) {
            // 4-byte character
            valeur = ((static_cast<unsigned char>(chaine[i]) & 0x07) << 18) | ((static_cast<unsigned char>(chaine[i + 1]) & 0x0f) << 12) | ((static_cast<unsigned char>(chaine[i + 2]) & 0x1f) << 6) | (static_cast<unsigned char>(chaine[i + 3]) & 0x3f);

            i += 4;
        }
		else {
			i += 1;
			continue;
		}

        os << valeur << '\n';
    }
}

int main()
{
   // std::cerr << "Taille chaine : " << taille_chaine("kévin") << '\n';
   imprime_lettres(std::cerr);
   imprime_paire_lettre_valeur(std::cerr);
//   index_lettre("kévin", std::cerr);
}

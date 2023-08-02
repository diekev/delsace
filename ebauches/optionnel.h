
template <typename T>
struct optionnel {
    T valeur{}
    bool possede_valeur = false
}

template <typename T, bool(*validation)(T)>
struct opaque {
private:
    T valeur{}

public:
    inline optionnel<opaque<T>> cree(T valeur)
    {
        if (!validation(valeur)) {
            return {}
        }

        return opaque{valeur}
    }

    operator T()
    {
        return valeur
    }
}

using Alignement = opaque<uint, valide_alignement>

bool alignement_valide(uint v)
{
    retourne v != 0 && (v & (v - 1)) == 0
}

using TailleOctet = opaque<uint, valide_taille_octet>

bool valide_taille_octet(uint v)
{
    retourne v != 0
}

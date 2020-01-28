#include "biblinternes/structures/tableau.hh"

#include <iostream>

enum CodeInst : int {
    /* allocation sur la pile */
    // pour l'allocation, nous devons retourner le pointeur de la où se trouve la valeur sur la pile
    ALLOUE,
    /* charge des valeurs depuis la mémoire selon leur taille en bits */
    CHARGE8,
    CHARGE16,
    CHARGE32,
    CHARGE64,
	CHARGE_CONST_8,
	CHARGE_CONST_16,
	CHARGE_CONST_32,
	CHARGE_CONST_64,
    /* stocke les valeurs en mémoire selon leur taille en bits */
    STOCKE8,
    STOCKE16,
    STOCKE32,
    STOCKE64,
    /* opérations mathématiques */
    AJOUTE,
    SOUSTRAIT,
    MULTIPLIE,
    DIVISE,
    MODULO,
    /* opérations binaires */
    /* opérations comparatives */
};

const char *chaine_instruction(CodeInst inst)
{
#define CAS_TYPE(x) case CodeInst::x: return #x;
	switch (inst) {
		CAS_TYPE(ALLOUE)
		CAS_TYPE(CHARGE8)
		CAS_TYPE(CHARGE16)
		CAS_TYPE(CHARGE32)
		CAS_TYPE(CHARGE64)
		CAS_TYPE(CHARGE_CONST_8)
		CAS_TYPE(CHARGE_CONST_16)
		CAS_TYPE(CHARGE_CONST_32)
		CAS_TYPE(CHARGE_CONST_64)
		CAS_TYPE(STOCKE8)
		CAS_TYPE(STOCKE16)
		CAS_TYPE(STOCKE32)
		CAS_TYPE(STOCKE64)
		CAS_TYPE(AJOUTE)
		CAS_TYPE(SOUSTRAIT)
		CAS_TYPE(MULTIPLIE)
		CAS_TYPE(DIVISE)
		CAS_TYPE(MODULO)
	}

	return "erreur : relation inconnue";
#undef CAS_TYPE
}

static const int NOMBRE_REGISTRES = 4;

struct Memoire {
	dls::tableau<char> donnees{};
	long pointeur = 0;

	Memoire(long taille)
		: donnees(taille)
	{}

	long alloue_taille(long taille)
	{
		if (pointeur + taille >= donnees.taille()) {
			// À FAIRE : erreur
		}

		auto ancien_pointeur = pointeur;
		pointeur += taille;
		return ancien_pointeur;
	}

	int charge32(long ptr) const
	{
		std::cerr << __func__ << ", ptr : " << ptr << '\n';
		return *reinterpret_cast<const int *>(&donnees[ptr]);
	}

	void stocke32(long ptr, int valeur)
	{
		std::cerr << __func__ << ", ptr : " << ptr << ", val " << valeur << '\n';
		*reinterpret_cast<int *>(&donnees[ptr]) = valeur;
	}
};

struct CodeMachine {
	dls::tableau<long> donnees{};

	void ajoute_instruction(CodeInst inst)
	{
		donnees.pousse(static_cast<long>(inst));
	}

	CodeInst charge_instruction(int &compteur)
	{
		return static_cast<CodeInst>(donnees[compteur++]);
	}

	long ajoute_donnees_instruction(long valeur)
	{
		auto ptr = donnees.taille();
		donnees.pousse(valeur);
		return ptr;
	}

	long charge_donnees_instruction(int &compteur) const
	{
		return donnees[compteur++];
	}

	void ajourne_donnees(int &compteur, long valeur)
	{
		donnees[compteur++] = valeur;
	}
};

struct Interpreteuse {
    long registres[NOMBRE_REGISTRES];
	Memoire memoire{8192};

    CodeMachine code{};
};

void execute_code(Interpreteuse *inter)
{
    int compteur = 0;

	while (compteur < inter->code.donnees.taille()) {
        // prend l'instruction
		auto inst = inter->code.charge_instruction(compteur);

		std::cerr << "Lecture de l'instruction : " << chaine_instruction(inst) << '\n';

        switch (inst) {
            case ALLOUE:
            {
				auto taille = inter->code.charge_donnees_instruction(compteur);
				auto ptr = inter->memoire.alloue_taille(taille);

				std::cerr << "--- allocation d'une variable sur la pile :\n";
				std::cerr << "------ taille : " << taille << '\n';
				std::cerr << "------ ptr : " << ptr << '\n';

				inter->code.ajourne_donnees(compteur, ptr);

                break;
            }
            case CHARGE8:
            {
                break;
            }
            case CHARGE16:
            {
                break;
            }
            case CHARGE32:
			{
				auto ptr_inst = static_cast<int>(inter->code.charge_donnees_instruction(compteur));
				auto reg = inter->code.charge_donnees_instruction(compteur);

				auto ptr = inter->code.charge_donnees_instruction(ptr_inst);

				inter->registres[reg] = inter->memoire.charge32(ptr);

				std::cerr << "--- charge vers le registre :\n";
				std::cerr << "------ reg : " << reg << '\n';
				std::cerr << "------ ptr : " << ptr << '\n';
				std::cerr << "------ val : " << inter->registres[reg] << '\n';

                break;
            }
            case CHARGE64:
            {
                break;
            }
			case CHARGE_CONST_8:
			{
				break;
			}
			case CHARGE_CONST_16:
			{
				break;
			}
			case CHARGE_CONST_32:
			{
				auto val = static_cast<int>(inter->code.charge_donnees_instruction(compteur));
				auto reg = inter->code.charge_donnees_instruction(compteur);

				inter->registres[reg] = val;

				std::cerr << "--- charge vers le registre :\n";
				std::cerr << "------ reg : " << reg << '\n';
				std::cerr << "------ val : " << val << '\n';
				std::cerr << "------ val : " << inter->registres[reg] << '\n';

				break;
			}
			case CHARGE_CONST_64:
			{
				break;
			}
            case STOCKE8:
			{
                break;
            }
            case STOCKE16:
            {
                break;
            }
            case STOCKE32:
            {
				auto ptr_inst = static_cast<int>(inter->code.charge_donnees_instruction(compteur));
				auto reg = inter->code.charge_donnees_instruction(compteur);

				auto ptr = inter->code.charge_donnees_instruction(ptr_inst);

				int valeur = static_cast<int>(inter->registres[reg]);
				inter->memoire.stocke32(ptr, valeur);

				std::cerr << "--- stocke depuis le registre :\n";
				std::cerr << "------ reg : " << reg << '\n';
				std::cerr << "------ ptr : " << ptr << '\n';

                break;
            }
            case STOCKE64:
            {
                break;
            }
            case AJOUTE:
			{
                int reg0 = 0;
                int reg1 = 1;
				int reg2 = 2;

                inter->registres[reg2] = inter->registres[reg0] + inter->registres[reg1];

				std::cerr << "--- ajoute les registres :\n";
				std::cerr << "------ r0 : " << inter->registres[reg0] << '\n';
				std::cerr << "------ r1 : " << inter->registres[reg1] << '\n';
				std::cerr << "------ r2 : " << inter->registres[reg2] << '\n';

                break;
            }
            case SOUSTRAIT:
            {
                int reg0 = 0;
                int reg1 = 1;
                int reg2 = 2;

                inter->registres[reg2] = inter->registres[reg0] - inter->registres[reg1];

                break;
            }
            case MULTIPLIE:
            {
                int reg0 = 0;
                int reg1 = 1;
                int reg2 = 2;

                inter->registres[reg2] = inter->registres[reg0] * inter->registres[reg1];

                break;
            }
            case DIVISE:
            {
                int reg0 = 0;
                int reg1 = 1;
                int reg2 = 2;

                inter->registres[reg2] = inter->registres[reg0] / inter->registres[reg1];

                break;
            }
            case MODULO:
            {
                int reg0 = 0;
                int reg1 = 1;
                int reg2 = 2;

                inter->registres[reg2] = inter->registres[reg0] % inter->registres[reg1];

                break;
            }
        }
    }
}

int main()
{
	auto inter = Interpreteuse{};

	auto &code = inter.code;

	// a : z32
	code.ajoute_instruction(CodeInst::ALLOUE);
	code.ajoute_donnees_instruction(4);
	auto ptr_a = code.ajoute_donnees_instruction(0);

	// b : z32
	code.ajoute_instruction(CodeInst::ALLOUE);
	code.ajoute_donnees_instruction(4);
	auto ptr_b = code.ajoute_donnees_instruction(0);

	// a = 5
	code.ajoute_instruction(CodeInst::CHARGE_CONST_32);
	code.ajoute_donnees_instruction(5);
	code.ajoute_donnees_instruction(0);

	code.ajoute_instruction(STOCKE32);
	code.ajoute_donnees_instruction(ptr_a);
	code.ajoute_donnees_instruction(0);

	// a = 5
	code.ajoute_instruction(CodeInst::CHARGE_CONST_32);
	code.ajoute_donnees_instruction(5);
	code.ajoute_donnees_instruction(0);

	code.ajoute_instruction(STOCKE32);
	code.ajoute_donnees_instruction(ptr_a);
	code.ajoute_donnees_instruction(0);

	// b = 6
	code.ajoute_instruction(CodeInst::CHARGE_CONST_32);
	code.ajoute_donnees_instruction(6);
	code.ajoute_donnees_instruction(0);

	code.ajoute_instruction(STOCKE32);
	code.ajoute_donnees_instruction(ptr_b);
	code.ajoute_donnees_instruction(0);

	// a = a + b
	code.ajoute_instruction(CodeInst::CHARGE32);
	code.ajoute_donnees_instruction(ptr_a);
	code.ajoute_donnees_instruction(0);

	code.ajoute_instruction(CodeInst::CHARGE32);
	code.ajoute_donnees_instruction(ptr_b);
	code.ajoute_donnees_instruction(1);

	code.ajoute_instruction(CodeInst::AJOUTE);

	code.ajoute_instruction(STOCKE32);
	code.ajoute_donnees_instruction(ptr_a);
	code.ajoute_donnees_instruction(2);

	execute_code(&inter);

	int ptr = inter.code.donnees[ptr_a];
	int a = inter.memoire.charge32(ptr);
	std::cerr << "a est de : " << a << '\n';

	return 0;
}

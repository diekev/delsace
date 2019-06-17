/*
opcode
- charge registre, adresse
- bouge registre, registre
- additionne registre, registre
- multiplie  registre, registre
- xor reg, reg

CRG
ADD
MUL
XOR
BOG

x = Wx + b

3 registres pour la multiplication de matrices (A, B, tmp)
2 registres pour les lignes/colonnes

taille registres?

pad 0
*/

using valeur_type = unsigned char;
static constexpr auto TAILLE_REGISTRE = 8;

struct Registre {
	valeur_type v[TAILLE_REGISTRE][TAILLE_REGISTRE];
};

struct TPU {
	Registre A, B;
};

void bouge(Registre *A, Registre *B)
{
	for (int i = 0; i < TAILLE_REGISTRE; ++i) {
		for (int j = 0; j < TAILLE_REGISTRE; ++j) {
			A->v[i][j] = B[i][j];
		}
	}
}

void xor(Registre *A, Registre *B)
{
	for (int i = 0; i < TAILLE_REGISTRE; ++i) {
		for (int j = 0; j < TAILLE_REGISTRE; ++j) {
			A->v[i][j] = A->v[i][j] ^ B[i][j];
		}
	}
}

void additionne(Registre *A, Registre *B)
{
	for (int i = 0; i < TAILLE_REGISTRE; ++i) {
		for (int j = 0; j < TAILLE_REGISTRE; ++j) {
			A->v[i][j] = A[i][j] + B[i][j];
		}
	}
}

void multiplie(Registre *A, Registre *B)
{
	// produit scalaire AB
	valeur_type ligneA[TAILLE_REGISTRE];
	valeur_type colonneB[TAILLE_REGISTRE];

	Registre tmp;

	for (int i = 0; i < TAILLE_REGISTRE; ++i) {
		// extrait ligne A
		for (int k = 0; k < TAILLE_REGISTRE; ++k) {
			ligneA[k] = A->[i][k];
		}

		for (int j = 0; j < TAILLE_REGISTRE; ++j) {

			// extrait colonne B
			for (int k = 0; k < TAILLE_REGISTRE; ++k) {
				colonneB[k] = B->[k][j];
			}

			// produit scalaire AB

			// 1. produit, on garde ligneA
			for (int k = 0; k < TAILLE_REGISTRE; ++k) {
				colonneB[k] = ligneA[k] * colonneB[k];
			}

			// 2. accumulation
			tmp.v[i][j] = 0;

			for (int k = 0; k < TAILLE_REGISTRE; ++k) {
				tmp.v[i][j] += colonneB[k];
			}
		}
	}

	bouge(&tmp, A);
}

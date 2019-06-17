class NoeudVariable {};
class NoeudNombreEntier {};
class NoeudNombreDecimal {};
class NoeudAppelFonction {};

class NoeudExpression {
	NoeudVariable *noeud_variable;
	NoeudOperation *noeud_operateur;
	NoeudNombreEntier *noeud_nombre_entier;
	NoeudNombreDecimal *noeud_nombre_decimal;
	NoeudAppelFonction *noeud_appel_fonction;
};

class NoeudDeclarationVariable {
	NoeudExpression *noeud;
};

class NoeudRetour {
	NoeudExpression *noeud;
};

class NoeudOperation {
	NoeudExpression *noeud1;
	NoeudExpression *noeud2;
};

class NoeudControleFlux {
	NoeudExpression *noeud;
};

class NoeudDeclarationFonction {
	std::vector<NoeudDeclarationVariable *> m_declarations_variables;
	std::vector<NoeudExpression *> m_expressions;
	std::vector<NoeudRetour *> m_retours;
	std::vector<NoeudControleFlux *> m_controles_flux;

	std::vector<char> m_operations;

	enum {
		DECLARATION_VARIABLE = 0,
		EXPRESSION = 1,
		RETOUR = 2,
		CONTROLE_FLUX = 3,
	};

public:
	void ajoute_declaration_variable(NoeudDeclarationVariable *noeud)
	{
		m_declarations_variables.push_back(noeud);
		m_operations.push_back(DECLARATION_VARIABLE);
	}

	void ajoute_expression(NoeudExpression *);
	void ajoute_retour(NoeudRetour *);
	void ajoute_controle_flux(NoeudControleFlux *);

	void genere_code()
	{
		auto ptr_decl = m_declarations_variables.data();
		auto ptr_ret = m_declarations_variables.data();
		auto ptr_decl = m_declarations_variables.data();
		auto ptr_decl = m_declarations_variables.data();

		for (const auto &operation : m_operations) {
			switch (operation) {
				case DECLARATION_VARIABLE:
					(*ptr_decl)->genere_code();
					++ptr_decl;
					break;
				case EXPRESSION:
					(*ptr_decl)->genere_code();
					++ptr_decl;
					break;
				case RETOUR:
					(*ptr_ret)->genere_code();
					++ptr_ret;
					break;
				case CONTROLE_FLUX:
					(*ptr_ret)->genere_code();
					++ptr_ret;
					break;
			}
		}
	}
};


void compile_fonction(NoeudDeclaration *declaration_fonction, ContexteGenerationCode &contexte)
{
	declaration_fonction->valide_typage(contexte, false);
	declaration_fonction->genere_code_llvm(contexte, false);
}

// condition :
// - module synchronisé
// - contexte général synchronisé (fonctions, structures, globales, magasin types)
// - contexte par-fil (locales, bloc courant)
// - arbre syntaxic synchronisé

enum etat_fonction {
	TERMINE = 0,
	NECESSITE_INFORMATION = 1,
	NON_COMMENCE = 2,
	EN_COURS = 2,
};

while (true) {
	for (auto &fonction : m_fonctions) {
		switch (fonction.etat()) {
			case etat_fonction::TERMINE:
			{
				/* rien à faire */
				break;
			}
			case etat_fonction::NECESSITE_INFORMATION:
			{
				if (analyse_non_finie) {
					/* retente */
					fonction->etat(etat_fonction::EN_COURS);
					std::async(compile_fonction, fonction);
				}
				else {
					/* erreur */
				}

				break;
			}
			case etat_fonction::NON_COMMENCE:
			{
				fonction->etat(etat_fonction::EN_COURS);
				std::async(compile_fonction, fonction);
				break;
			}
			case etat_fonction::EN_COURS:
			{
				/* rien à faire */
				break;
			}
		}
	}

	std::remove(m_fonctions, [](const Fonction &fonction { return fonction.etat() == etat_fonction::TERMINE; });

	if (m_fonctions.empty() && !analyse_non_finie) {
		break;
	}
}

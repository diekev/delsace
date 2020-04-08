struct UniteCompilation {
    enum Type {
        TEXTE,
        NOEUD,
    };

    enum Etat {
        LEXAGE_ATTENDU,
        SYNTAXAGE_ATTENDU,
        TYPAGE_ATTENDU,
        MESSAGE_ATTENDU,
        RI_ATTENDUE,
        CODE_MACHINE_ATTENDU,
    };

    union {
        NoeudBase *noeud;
        TamponSource *texte;
    };

    // données par thread
    Lexeuse lexeuse;
    Syntaxeuse syntaxeuse;
    ConstructriceRI constructrice_ri;
    ValideuseSyntaxe valideuse;
    Coulisse coulisse;

    // données globales
    Compilatrice compilatrice;
    Typeuse typeuse;
    Operateur operateurs;
    NormalisatriceNom normalisatrice_nom;
};


while (true) {
    auto ok = tente_defile_unite(&unite)

    if (!ok) {
        break;
    }

    switch (unite.etat) {
        case UniteCompilation::Etat::LEXAGE_ATTENDU:
        {
            if (!this->lexeuse.lexe(unite)) {
                // erreur de compilation
            }

            break;
        }
        case UniteCompilation::Etat::SYNTAXAGE_ATTENDU:
        {
            if (!this->syntaxage.analyse(unite)) {
                // erreur de compilation
            }

            break;
        }
        case UniteCompilation::Etat::TYPAGE_ATTENDU:
        {
            if (!this->valideuse.valide(unite)) {
                if (compilation_terminee) {
                    // erreur de compilation
                }
                else {
                    enfile(unite);
                }
            }

            break;
        }
        case UniteCompilation::Etat::MESSAGE_ATTENDU:
        {
            // vérifie que le message fut géré
            break;
        }
        case UniteCompilation::Etat::RI_ATTENDUE:
        {
            this->constructrice_ri.traite(unite);
            break;
        }
        case UniteCompilation::Etat::CODE_MACHINE:
        {
            this->coulisse->genere_code_pour(unite);
            break;
        }
    }
}

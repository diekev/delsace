/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "adn.hh"

#include <iostream>

#include "structures/ensemble.hh"

void IdentifiantADN::préserve_accents_si_nom_le_requiers()
{
    if (nom == "OpérateurUnaire" || nom == "OpérateurBinaire" || nom == "VisibilitéSymbole" ||
        nom == "DonnéesSymboleExterne" || nom == "MembreTypeComposé" || nom == "TableOpérateurs" ||
        nom == "Lexème" || nom == "GenreLexème" || nom == "NoeudDépendance") {

        std::cerr << "Préserve accent : " << nom << "\n";
        préserve_accents();
    }
}

const IdentifiantADN &Type::accede_nom() const
{
    if (est_pointeur()) {
        return comme_pointeur()->type_pointe->accede_nom();
    }

    if (est_tableau()) {
        return comme_tableau()->type_pointe->accede_nom();
    }

    return comme_nominal()->nom_cpp;
}

FluxSortieCPP &operator<<(FluxSortieCPP &flux, const IdentifiantADN &ident)
{
    return flux << ident.nom_cpp();
}

FluxSortieKuri &operator<<(FluxSortieKuri &flux, const IdentifiantADN &ident)
{
    return flux << ident.nom_kuri();
}

FluxSortieKuri &operator<<(FluxSortieKuri &os, Type const &type)
{
    if (type.est_tableau()) {
        const auto type_tableau = type.comme_tableau();
        os << "[..]";
        os << *type_tableau->type_pointe;
    }
    else if (type.est_pointeur()) {
        const auto type_pointeur = type.comme_pointeur();
        os << "*";
        os << *type_pointeur->type_pointe;
    }
    else if (type.est_nominal()) {
        const auto type_nominal = type.comme_nominal();
        os << type_nominal->nom_kuri;
    }

    return os;
}

FluxSortieCPP &operator<<(FluxSortieCPP &os, Type const &type)
{
    if (type.est_const) {
        os << "const ";
    }

    if (type.est_tableau()) {
        const auto type_tableau = type.comme_tableau();

        if (type_tableau->est_compresse) {
            // À FAIRE: le « int » échouera pour les NoeudsCodes
            os << "kuri::tableau_compresse<" << *type_tableau->type_pointe << ", int>";
        }
        else if (type_tableau->est_synchrone) {
            os << "kuri::tableau_synchrone<" << *type_tableau->type_pointe << ">";
        }
        else {
            // À FAIRE: le « int » échouera pour les NoeudsCodes
            os << "kuri::tableau<" << *type_tableau->type_pointe << ", int>";
        }
    }
    else if (type.est_pointeur()) {
        const auto type_pointeur = type.comme_pointeur();
        os << *type_pointeur->type_pointe << "*";
    }
    else if (type.est_nominal()) {
        const auto type_nominal = type.comme_nominal();
        os << type_nominal->nom_cpp;
    }

    return os;
}

Proteine::Proteine(IdentifiantADN nom) : m_nom(nom)
{
}

ProteineStruct::ProteineStruct(IdentifiantADN nom) : Proteine(nom), m_nom_code(nom)
{
}

void ProteineStruct::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
    if (pour_entete) {
        os << "struct " << m_nom.nom_cpp();

        if (m_mere) {
            os << " : public " << m_mere->nom().nom_cpp();
        }

        os << " {\n";

        if (!accede_nom_genre().est_nul()) {
            os << "\t" << m_nom << "() { genre = " << enum_discriminante()->nom()
               << "::" << accede_nom_genre() << ";";

            if (!m_genre_valeur.est_nul()) {
                os << " genre_valeur = GenreValeur::" << m_genre_valeur << ";";
            }

            os << " }\n";
            os << "\tCOPIE_CONSTRUCT(" << m_nom << ");\n";
            os << "\n";
        }

        os << "\tPOINTEUR_NUL(" << m_nom << ")\n";

        if (!m_membres.est_vide()) {
            os << "\n";
        }

        POUR (m_membres) {
            os << "\t";

            if (it.est_mutable) {
                os << "mutable ";
            }

            os << *it.type << ' ' << it.nom.nom_cpp();

            if (it.valeur_defaut != "") {
                os << " = ";

                if (it.valeur_defaut_est_acces) {
                    os << *it.type << "::";
                }

                if (it.type->est_nominal("kuri::chaine", "kuri::chaine_statique")) {
                    os << '"' << it.valeur_defaut << '"';
                }
                else if (it.valeur_defaut == "vrai") {
                    os << "true";
                }
                else if (it.valeur_defaut == "faux") {
                    os << "false";
                }
                else {
                    os << supprime_accents(it.valeur_defaut);
                }
            }
            else {
                os << " = " << it.type->valeur_defaut();
            }

            os << ";\n";
        }

        if (m_nom.nom_cpp() == "NoeudExpression") {
            os << "\n";
            os << "\tinline bool possède_drapeau(DrapeauxNoeud drapeaux_) const\n";
            os << "\t{\n";
            os << "\t\treturn (drapeaux & drapeaux_) != DrapeauxNoeud::AUCUN;\n";
            os << "\t}\n";
            os << "\tinline bool possède_drapeau(PositionCodeNoeud position_) const\n";
            os << "\t{\n";
            os << "\t\treturn (position & position_) != PositionCodeNoeud::AUCUNE;\n";
            os << "\t}\n";
        }
        else if (m_nom.nom_cpp() == "NoeudDeclarationEnteteFonction") {
            os << "\t";
            os << "\tBaseDeclarationVariable *parametre_entree(int64_t i) const\n";
            os << "\t{\n";
            os << "\t\tauto param = params[static_cast<int>(i)];\n";

            os << "\t\tif (param->est_empl()) {\n";
            os << "\t\t\treturn "
                  "param->comme_empl()->expression->comme_base_declaration_variable();\n";
            os << "\t\t}\n";

            os << "\t\treturn param->comme_base_declaration_variable();\n";
            os << "\t}\n";

            os << "\tkuri::chaine_statique donne_nom_broyé(Broyeuse &broyeuse);\n";
            os << "\tType *type_initialisé() const;\n";
            os << "\tinline bool possède_drapeau(DrapeauxNoeud drapeaux_) const\n";
            os << "\t{\n";
            os << "\t\treturn (drapeaux & drapeaux_) != DrapeauxNoeud::AUCUN;\n";
            os << "\t}\n";
            os << "\tinline bool possède_drapeau(DrapeauxNoeudFonction drapeaux_) const\n";
            os << "\t{\n";
            os << "\t\treturn (drapeaux_fonction & drapeaux_) != DrapeauxNoeudFonction::AUCUN;\n";
            os << "\t}\n";
        }
        else if (m_nom.nom_cpp() == "NoeudBloc") {
            os << "\tmutable int nombre_recherches = 0;\n";
            os << "\tkuri::table_hachage<IdentifiantCode const *, NoeudDeclaration *> "
                  "table_membres{\"membres_bloc\"};\n";
            os << "\tint nombre_de_membres() const;\n";
            os << "\tvoid réserve_membres(int nombre);\n";
            os << "\tvoid ajoute_membre(NoeudDeclaration *decl);\n";
            os << "\tvoid ajoute_membre_au_debut(NoeudDeclaration *decl);\n";
            os << "\tvoid fusionne_membres(NoeudBloc *de);\n";
            os << "\tNoeudDeclaration *membre_pour_index(int index) const;\n";
            os << "\tNoeudDeclaration *declaration_pour_ident(IdentifiantCode const "
                  "*ident_recherche) const;\n";
            os << "\tNoeudDeclaration *declaration_avec_meme_ident_que(NoeudExpression const "
                  "*expr) const;\n";
            os << "\tvoid ajoute_expression(NoeudExpression *expr);\n";
        }
        else if (m_nom.nom_cpp() == "NoeudDeclarationType") {
            os << "\tinline bool possède_drapeau(DrapeauxNoeud drapeaux_) const\n";
            os << "\t{\n";
            os << "\t\treturn (drapeaux & drapeaux_) != DrapeauxNoeud::AUCUN;\n";
            os << "\t}\n";
            os << "\tinline bool possède_drapeau(DrapeauxTypes drapeaux_) const\n";
            os << "\t{\n";
            os << "\t\treturn (drapeaux_type & drapeaux_) != DrapeauxTypes::AUCUN;\n";
            os << "\t}\n";
        }
        else if (m_nom.nom_cpp() == "NoeudDeclarationTypeCompose") {
            os << "\tkuri::tableau_statique<const MembreTypeComposé> "
                  "donne_membres_pour_code_machine() "
                  "const;\n";
        }

        // Prodéclare les fonctions de discrimination.
        if (est_racine_hiérarchie()) {
            pour_chaque_derivee_recursif([&os](const ProteineStruct &derivee) {
                if (derivee.m_nom_comme.nom_cpp() == "") {
                    return;
                }

                const auto nom_comme = derivee.m_nom_comme;
                const auto nom_noeud = derivee.m_nom;
                génère_déclaration_fonctions_discrimination(os, nom_noeud, nom_comme);
            });
        }

        os << "};\n\n";
    }

    if (!pour_entete) {
        os << "static void imprime_membres(std::ostream &os, " << m_nom.nom_cpp()
           << " const &valeur)\n";
        os << "{\n";

        if (m_mere) {
            os << "\timprime_membres(os, static_cast<" << m_mere->nom().nom_cpp()
               << " const &>(valeur));\n";
        }

        POUR (m_membres) {
            if (it.type->est_tableau()) {
                os << "\tos << \"\\t" << it.nom.nom_cpp() << " : \" << valeur."
                   << it.nom.nom_cpp();
                const auto type_tableau = it.type->comme_tableau();
                if (type_tableau->est_synchrone) {
                    os << "->";
                }
                else {
                    os << ".";
                }
                os << "taille() << \" éléments \\n\";" << '\n';
                continue;
            }

            os << "\tos << \"\\t" << it.nom.nom_cpp() << " : \" << valeur." << it.nom.nom_cpp()
               << " << '\\n';" << '\n';
        }
        os << "}\n\n";
    }

    os << "std::ostream &operator<<(std::ostream &os, " << m_nom.nom_cpp() << " const &valeur)";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        os << "\n";
        os << "{\n";
        os << "\tos << \"" << m_nom.nom_cpp() << " : {\\n\";" << '\n';
        os << "\timprime_membres(os, valeur);\n";
        os << "\tos << \"}\\n\";\n";
        os << "\treturn os;\n";
        os << "}\n\n";
    }

    os << "bool est_valide(" << m_nom.nom_cpp() << " const &valeur)";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        os << "\n";
        os << "{\n";

        if (!accede_nom_genre().est_nul()) {
            os << "\tif (valeur.genre != " << enum_discriminante()->nom()
               << "::" << accede_nom_genre() << ") {\n";
            os << "\t\treturn false;\n";
            os << "\t}\n";
        }

        pour_chaque_membre_recursif([&os](Membre const &it) {
            if (it.type->est_nominal() && it.type->comme_nominal()->est_proteine &&
                it.type->comme_nominal()->est_proteine->comme_enum()) {
                os << "\tif (!est_valeur_legale(valeur." << it.nom.nom_cpp() << ")) {\n";
                os << "\t\treturn false;\n";
                os << "\t}\n";
            }
        });

        os << "\treturn true;\n";
        os << "}\n\n";
    }
}

void ProteineStruct::genere_code_cpp_apres_declaration(FluxSortieCPP &os)
{
    // Implémente les fonctions de discrimination.
    // Nous devons attendre que toutes les structures soient déclarées avant de
    // pouvoir faire ceci.
    if (est_racine_hiérarchie()) {
        pour_chaque_derivee_recursif([&os, this](const ProteineStruct &derivee) {
            if (derivee.m_nom_comme.nom_cpp() == "") {
                return;
            }

            génère_définition_fonctions_discrimination(os, m_nom.nom_cpp(), derivee, false);
        });
    }
}

void ProteineStruct::genere_code_kuri(FluxSortieKuri &os)
{
    os << m_nom_code << " :: struct ";
    if (m_nom_code.nom_kuri() == "AnnotationCode") {
        os << "#interface ";
    }
    os << "{";
    if (m_mere) {
        if (m_mere->accede_nom_comme().est_nul()) {
            os << "\n\templ base";
        }
        else {
            os << "\n\templ base_" << m_mere->accede_nom_comme();
        }
        os << ": " << m_mere->nom() << "\n";
    }

    if (!membres().est_vide() || !m_mere) {
        os << "\n";
    }

    const auto est_chaine_litterale = m_nom_code.nom_kuri() == "NoeudCodeLittéraleChaine";

    POUR (m_membres) {
        if (it.type->est_pointeur() &&
            it.type->comme_pointeur()->type_pointe->est_nominal("Lexème")) {
            os << "\tchemin_fichier: chaine\n";
            os << "\tnom_fichier: chaine\n";
            os << "\tnuméro_ligne: z32\n";
            os << "\tnuméro_colonne: z32\n";
            continue;
        }

        if (it.type->est_pointeur() &&
            it.type->comme_pointeur()->type_pointe->est_nominal("IdentifiantCode")) {
            os << "\tnom: chaine\n";
            continue;
        }

        if (it.type->est_pointeur() &&
            it.type->comme_pointeur()->type_pointe->est_nominal("NoeudDeclarationType")) {
            os << "\t" << it.nom.nom_kuri() << ": *InfoType\n";
            continue;
        }

        if (est_chaine_litterale && it.nom.nom_cpp() == "valeur") {
            os << "\tvaleur: chaine\n";
            continue;
        }

        os << "\t" << it.nom;

        if (it.valeur_defaut == "") {
            os << ": " << *it.type;
        }
        else {
            os << " := ";
            if (it.valeur_defaut_est_acces) {
                os << *it.type << ".";
            }

            if (it.type->est_nominal("kuri::chaine", "kuri::chaine_statique")) {
                os << '"' << it.valeur_defaut << '"';
            }
            else {
                os << it.valeur_defaut;
            }
        }

        os << "\n";
    }
    os << "}\n\n";
}

void ProteineStruct::ajoute_membre(const Membre membre)
{
    if (membre.est_a_copier) {
        m_possède_membre_a_copier = true;
    }

    if (membre.est_enfant) {
        m_possède_enfant = true;
    }

    m_possède_tableaux |= membre.type->est_tableau();

    if (membre.est_code && m_paire) {
        m_paire->m_possède_enfant = membre.est_enfant;
        m_paire->m_possède_membre_a_copier = membre.est_a_copier;
        m_paire->m_membres.ajoute(membre);
    }

    m_membres.ajoute(membre);
}

void ProteineStruct::pour_chaque_membre_recursif(std::function<void(const Membre &)> rappel)
{
    if (m_mere) {
        m_mere->pour_chaque_membre_recursif(rappel);
    }

    POUR (m_membres) {
        rappel(it);
    }
}

void ProteineStruct::pour_chaque_copie_extra_recursif(std::function<void(const Membre &)> rappel)
{
    if (m_mere) {
        m_mere->pour_chaque_copie_extra_recursif(rappel);
    }

    POUR (m_membres) {
        if (it.est_a_copier) {
            rappel(it);
        }
    }
}

void ProteineStruct::pour_chaque_enfant_recursif(std::function<void(const Membre &)> rappel)
{
    if (m_mere) {
        m_mere->pour_chaque_enfant_recursif(rappel);
    }

    POUR (m_membres) {
        if (it.est_enfant) {
            rappel(it);
        }
    }
}

void ProteineStruct::pour_chaque_derivee_recursif(
    std::function<void(const ProteineStruct &)> rappel)
{
    POUR (derivees()) {
        rappel(*it);

        if (it->est_racine_soushierachie()) {
            it->pour_chaque_derivee_recursif(rappel);
        }
    }
}

ProteineEnum::ProteineEnum(IdentifiantADN nom) : Proteine(nom)
{
}

void ProteineEnum::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
    if (pour_entete) {
        os << "enum class " << m_nom.nom_cpp() << " : " << *m_type << " {\n";

        POUR (m_membres) {
            os << "\t" << it.nom.nom_cpp() << ",\n";
        }

        os << "};\n\n";
    }

    if (!pour_entete) {
        os << "static kuri::chaine_statique chaines_membres_" << m_nom.nom_cpp() << "["
           << m_membres.taille() << "] = {\n";

        POUR (m_membres) {
            os << "\t\"" << it.nom.nom_cpp() << "\",\n";
        }

        os << "};\n\n";
    }

    os << "std::ostream &operator<<(std::ostream &os, " << m_nom.nom_cpp() << " valeur)";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        os << "\n";
        os << "{\n";
        os << "\treturn os << chaines_membres_" << m_nom.nom_cpp()
           << "[static_cast<int>(valeur)];\n";
        os << "}\n\n";
    }

    os << "bool est_valeur_legale(" << m_nom.nom_cpp() << " valeur)";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        const auto &premier_membre = m_membres[0];
        const auto &dernier_membre = m_membres.dernier_élément();

        os << "\n";
        os << "{\n";
        os << "\treturn valeur >= " << m_nom.nom_cpp() << "::" << premier_membre.nom.nom_cpp()
           << " && valeur <= " << m_nom.nom_cpp() << "::" << dernier_membre.nom.nom_cpp() << ";\n";
        os << "}\n\n";
    }
}

void ProteineEnum::genere_code_kuri(FluxSortieKuri &os)
{
    os << m_nom.nom_kuri() << " :: énum " << *m_type << " {\n";
    POUR (m_membres) {
        os << "\t" << it.nom.nom_kuri() << "\n";
    }
    os << "}\n\n";
}

void ProteineEnum::ajoute_membre(const Membre membre)
{
    m_membres.ajoute(membre);
}

ProteineFonction::ProteineFonction(IdentifiantADN nom) : Proteine(nom)
{
}

void ProteineFonction::genere_code_cpp(FluxSortieCPP &os, bool pour_entete)
{
    os << *m_type_sortie << ' ' << m_nom.nom_cpp();

    auto virgule = "(";

    if (m_parametres.taille() == 0) {
        os << virgule;
    }
    else {
        for (auto &param : m_parametres) {
            os << virgule << *param.type << ' ' << param.nom.nom_cpp();
            virgule = ", ";
        }
    }

    os << ")";

    if (pour_entete) {
        os << ";\n\n";
    }
    else {
        os << "\n{\n";
        os << "\tassert(false);\n";
        os << "\treturn " << m_type_sortie->valeur_defaut() << ";\n";
        os << "}\n\n";
    }
}

void ProteineFonction::genere_code_kuri(FluxSortieKuri &os)
{
    os << m_nom.nom_kuri() << " :: fonc ";

    auto virgule = "(";

    if (m_parametres.taille() == 0) {
        os << virgule;
    }
    else {
        for (auto &param : m_parametres) {
            os << virgule << param.nom.nom_kuri() << ": " << *param.type;
            virgule = ", ";
        }
    }

    os << ")" << " -> " << *m_type_sortie;

    if (m_est_intrinsèque) {
        os << " #intrinsèque \"" << m_symbole_gcc << "\"";
    }
    else if (m_est_ipa_compilatrice) {
        os << " #compilatrice";
    }

    os << "\n\n";
}

void ProteineFonction::ajoute_parametre(Parametre const parametre)
{
    m_parametres.ajoute(parametre);
}

SyntaxeuseADN::SyntaxeuseADN(Fichier *fichier) : BaseSyntaxeuse(fichier)
{
}

SyntaxeuseADN::~SyntaxeuseADN()
{
    POUR (proteines) {
        delete it;
    }

    POUR (proteines_paires) {
        delete it;
    }
}

void SyntaxeuseADN::analyse_une_chose()
{
    if (apparie("énum")) {
        parse_enum();
    }
    else if (apparie("struct")) {
        parse_struct();
    }
    else if (apparie("fonction")) {
        parse_fonction();
    }
    else {
        rapporte_erreur("attendu la déclaration d'une structure ou d'une énumération");
    }
}

void SyntaxeuseADN::parse_fonction()
{
    consomme();

    if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
        rapporte_erreur("Attendu une chaine de caractère après « fonction »");
    }

    auto fonction = crée_proteine<ProteineFonction>(lexème_courant()->chaine);
    consomme();

    // paramètres
    if (!apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
        rapporte_erreur("Attendu une parenthèse ouvrante après le nom de la fonction");
    }
    consomme();

    while (!apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto parametre = Parametre{};
        parametre.type = parse_type();

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur(
                "Attendu une chaine de caractère pour le nom du paramètre après son type");
        }

        parametre.nom = lexème_courant()->chaine;
        consomme();

        fonction->ajoute_parametre(parametre);

        if (apparie(GenreLexème::VIRGULE)) {
            consomme();
        }
    }

    // consomme la parenthèse fermante
    consomme();

    // type de retour
    if (apparie(GenreLexème::RETOUR_TYPE)) {
        consomme();
        fonction->type_sortie() = parse_type();
    }
    else {
        fonction->type_sortie() = m_typeuse.type_rien();
    }

    if (apparie(GenreLexème::POINT_VIRGULE)) {
        consomme();
    }

    while (apparie(GenreLexème::AROBASE)) {
        consomme();

        if (apparie("gcc")) {
            consomme();
            fonction->définis_symbole_gcc(lexème_courant()->chaine);
            consomme();
        }
        else if (apparie("intrinsèque")) {
            if (fonction->est_marquée_ipa_compilarice()) {
                rapporte_erreur("Fonction marquée comme intrinsèque alors qu'elle fut marquée "
                                "comme faisant partie de l'IPA compilatrice");
            }

            consomme();
            fonction->marque_intrinsèque();
        }
        else if (apparie("compilatrice")) {
            if (fonction->est_marquée_intrinsèque()) {
                rapporte_erreur("Fonction marquée comme faisant partie de l'IPA compilatrice "
                                "alors qu'elle fut marquée comme intrinsèque");
            }

            consomme();
            fonction->marque_ipa_compilarice();
        }
        else {
            consomme();
            rapporte_erreur("Attribut inconnu pour l'énumération");
        }

        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }
    }
}

void SyntaxeuseADN::parse_enum()
{
    consomme();

    if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
        rapporte_erreur("Attendu une chaine de caractère après « énum »");
    }

    auto proteine = crée_proteine<ProteineEnum>(lexème_courant()->chaine);

    consomme();

    consomme(GenreLexème::ACCOLADE_OUVRANTE,
             "Attendu une accolade ouvrante après le nom de « énum »");

    while (apparie(GenreLexème::AROBASE)) {
        consomme();

        if (apparie("code")) {
            consomme();
            consomme();
            // À FAIRE : code
        }
        else if (apparie("type")) {
            consomme();

            proteine->type() = parse_type();
        }
        else if (apparie("discr")) {
            consomme();
            proteine->type_discrimine(lexème_courant()->chaine);
            consomme();
        }
        else if (apparie("horslignée")) {
            consomme();
            proteine->marque_horslignee();
        }
        else {
            consomme();
            rapporte_erreur("Attribut inconnu pour l'énumération");
        }

        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }
    }

    if (!proteine->type()) {
        proteine->type() = m_typeuse.crée_type_nominal("int");
    }

    while (true) {
        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            break;
        }

        if (proteine->est_horslignee()) {
            rapporte_erreur("Déclaration d'un membre pour une énumération horslignée");
        }

        auto membre = Membre{};
        membre.nom = lexème_courant()->chaine;

        proteine->ajoute_membre(membre);

        consomme();

        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }
    }

    consomme(GenreLexème::ACCOLADE_FERMANTE,
             "Attendu une accolade fermante à la fin de l'énumération");
}

void SyntaxeuseADN::parse_struct()
{
    consomme();

    if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
        rapporte_erreur("Attendu une chaine de caractère après « énum »");
    }

    auto proteine = crée_proteine<ProteineStruct>(lexème_courant()->chaine);
    auto type_proteine = m_typeuse.crée_type_nominal(lexème_courant()->chaine);
    type_proteine->est_proteine = proteine;

    consomme();

    if (apparie(GenreLexème::DOUBLE_POINTS)) {
        consomme();

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu le nom de la structure mère après « : »");
        }

        auto nom_struct_mere = lexème_courant()->chaine;

        POUR (proteines) {
            if (it->nom().nom_kuri() == kuri::chaine_statique(nom_struct_mere)) {
                if (!it->comme_struct()) {
                    rapporte_erreur("Impossible de trouver la structure mère !");
                }

                proteine->descend_de(it->comme_struct());
                break;
            }
        }

        consomme();
    }

    consomme(GenreLexème::ACCOLADE_OUVRANTE,
             "Attendu une accolade ouvrante après le nom de « struct »");

    while (apparie(GenreLexème::AROBASE)) {
        consomme();

        // utilise 'lexeme.chaine' car 'comme' est un mot-clé, il n'y a pas d'identifiant
        if (apparie("code")) {
            consomme();

            if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu une chaine de caractère après @code");
            }

            proteine->mute_nom_code(lexème_courant()->chaine);

            auto paire = new ProteineStruct(lexème_courant()->chaine);
            type_proteine->nom_kuri = lexème_courant()->chaine;

            if (proteine->mere()) {
                paire->descend_de(proteine->mere()->paire());
            }

            proteines_paires.ajoute(paire);
            proteine->mute_paire(paire);

            consomme();
        }
        else if (apparie("comme")) {
            consomme();

            if (!apparie(GenreLexème::CHAINE_CARACTERE) && !est_mot_clé(lexème_courant()->genre)) {
                rapporte_erreur("Attendu une chaine de caractère après @code");
            }

            proteine->mute_nom_comme(lexème_courant()->chaine);
            consomme();
        }
        else if (apparie("genre")) {
            consomme();

            if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu une chaine de caractère après @genre");
            }

            proteine->mute_nom_genre(lexème_courant()->chaine);

            auto enum_discriminante = proteine->enum_discriminante();

            if (!enum_discriminante) {
                rapporte_erreur(
                    "Aucune énumération discriminante pour le noeud déclarant un genre");
            }
            else {
                auto membre = Membre();
                membre.nom = lexème_courant()->chaine;
                enum_discriminante->ajoute_membre(membre);
            }

            consomme();
        }
        else if (apparie("discr")) {
            consomme();

            auto type_enum = lexème_courant()->chaine;

            POUR (proteines) {
                if (!it->comme_enum()) {
                    continue;
                }

                if (it->nom().nom_cpp() == kuri::chaine_statique(type_enum)) {
                    if (it->comme_enum()->type_discrimine() != proteine->nom().nom_cpp()) {
                        rapporte_erreur("L'énumération devant discriminer le noeud n'est pas "
                                        "déclarée comme le discriminant");
                    }

                    proteine->mute_enum_discriminante(it->comme_enum());
                }
            }

            consomme();
        }
        else if (apparie("genre_valeur")) {
            consomme();
            if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu une chaine de caractère après @genre_valeur");
            }

            proteine->mute_genre_valeur(lexème_courant()->chaine);
            consomme();
        }
        else {
            rapporte_erreur("attribut inconnu");
        }

        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }
    }

    while (!apparie(GenreLexème::ACCOLADE_FERMANTE)) {
        auto membre = Membre{};
        membre.type = parse_type();

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu le nom du membre après son type !");
        }

        membre.nom = lexème_courant()->chaine;
        consomme();

        if (apparie(GenreLexème::EGAL)) {
            consomme();

            if (apparie(GenreLexème::POINT)) {
                membre.valeur_defaut_est_acces = true;
                consomme();
            }

            membre.valeur_defaut = lexème_courant()->chaine;
            consomme();
        }

        if (apparie(GenreLexème::CROCHET_OUVRANT)) {
            consomme();

            while (apparie(GenreLexème::CHAINE_CARACTERE)) {
                if (apparie("code")) {
                    membre.est_code = true;
                }
                else if (apparie("enfant")) {
                    membre.est_enfant = true;
                }
                else if (apparie("copie")) {
                    membre.est_a_copier = true;
                }
                else if (apparie("préserve_accents")) {
                    membre.nom.préserve_accents();
                }
                else if (apparie("mutable")) {
                    membre.est_mutable = true;
                }
                else {
                    rapporte_erreur("attribut inconnu");
                }

                consomme();
            }

            consomme(GenreLexème::CROCHET_FERMANT,
                     "Attendu un crochet fermant à la fin de la liste des attributs du membres");
        }

        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }

        proteine->ajoute_membre(membre);
    }

    consomme(GenreLexème::ACCOLADE_FERMANTE,
             "Attendu une accolade fermante à la fin de la structure");
}

Type *SyntaxeuseADN::parse_type()
{
    if (!apparie(GenreLexème::CHAINE_CARACTERE) && !est_mot_clé(lexème_courant()->genre)) {
        rapporte_erreur("Attendu le nom d'un type");
    }

    auto type_nominal = m_typeuse.crée_type_nominal(lexème_courant()->chaine);
    Type *type = type_nominal;

    POUR (proteines) {
        if (it->nom().nom_cpp() == type_nominal->nom_cpp.nom_cpp()) {
            if (it->comme_enum()) {
                type_nominal->est_proteine = it;
            }

            break;
        }
    }

    consomme();

    auto est_const = false;

    if (apparie(GenreLexème::CHAINE_CARACTERE)) {
        if (apparie("const")) {
            est_const = true;
            consomme();
        }
    }

    while (est_spécifiant_type(lexème_courant()->genre)) {
        if (lexème_courant()->genre == GenreLexème::CROCHET_OUVRANT) {
            consomme();

            auto est_compresse = false;
            auto est_synchrone = false;

            if (apparie(GenreLexème::CHAINE_CARACTERE)) {
                if (apparie("compressé")) {
                    est_compresse = true;
                    consomme();
                }
                else if (apparie("synchrone")) {
                    est_synchrone = true;
                    consomme();
                }
                else {
                    rapporte_erreur("attribut de tableau inconnu");
                }
            }

            consomme(GenreLexème::CROCHET_FERMANT,
                     "Attendu un crochet fermant après le crochet ouvrant");
            type = m_typeuse.crée_type_tableau(type, est_compresse, est_synchrone);
        }
        else if (lexème_courant()->genre == GenreLexème::FOIS) {
            type = m_typeuse.crée_type_pointeur(type);
            consomme();
        }
        else {
            rapporte_erreur("spécifiant type inconnu");
        }
    }

    // À FAIRE : ceci n'est pas vraiment l'endroit pour stocker cette information
    type->est_const = est_const;
    return type;
}

void SyntaxeuseADN::gère_erreur_rapportée(const kuri::chaine &message_erreur)
{
    std::cerr << message_erreur << "\n";
}

bool est_type_noeud(const Type *type)
{
    if (type->est_pointeur()) {
        return est_type_noeud(type->comme_pointeur()->type_pointe);
    }

    if (type->est_nominal()) {
        auto proteine_ = type->comme_nominal()->est_proteine;
        if (!proteine_) {
            return false;
        }

        /* `comme_struct()` retourne nul si la protéine n'est pas une structure, donc
         * le test de la boucle nous sers également de test pour savoir si nous avons
         * une structure. */
        auto proteine = proteine_->comme_struct();
        while (proteine) {
            if (proteine->nom().nom_cpp() == "NoeudExpression") {
                return true;
            }

            proteine = proteine->mere();
        }

        return false;
    }

    if (type->est_tableau()) {
        return est_type_noeud(type->comme_tableau()->type_pointe);
    }

    return false;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxillaires.
 * \{ */

void genere_déclaration_identifiants_code(const kuri::tableau<Proteine *> &proteines,
                                          FluxSortieCPP &os,
                                          bool pour_entête,
                                          kuri::chaine_statique identifiant_fonction)
{
    if (pour_entête) {
        prodeclare_struct(os, "IdentifiantCode");
        prodeclare_struct(os, "TableIdentifiant");
        os << "\n";
    }
    else {
        inclus(os, "parsage/identifiant.hh");
        os << "\n";
    }

    kuri::ensemble<kuri::chaine_statique> identifiants;
    POUR (proteines) {
        if (!it->est_fonction()) {
            continue;
        }
        identifiants.insère(it->nom().nom_kuri());
    }

    os << "namespace ID {\n";
    identifiants.pour_chaque_element([&](kuri::chaine_statique it) {
        if (pour_entête) {
            os << "extern ";
        }
        os << "IdentifiantCode *" << it << ";\n";
    });
    os << "}\n\n";

    os << "void initialise_identifiants_" << identifiant_fonction << "(TableIdentifiant &table)";

    if (pour_entête) {
        os << ";\n\n";
        return;
    }

    os << "\n{\n";
    identifiants.pour_chaque_element([&](kuri::chaine_statique it) {
        os << "\tID::" << it << " = table.identifiant_pour_chaine(\"" << it << "\");\n";
    });
    os << "}\n\n";
}

void génère_déclaration_fonctions_discrimination(FluxSortieCPP &os,
                                                 IdentifiantADN const &nom_noeud,
                                                 IdentifiantADN const &nom_comme)
{
    os << "\tinline bool est_" << nom_comme << "() const;\n";
    os << "\tinline " << nom_noeud << " *comme_" << nom_comme << "();\n";
    os << "\tinline const " << nom_noeud << " *comme_" << nom_comme << "() const;\n";
}

void génère_définition_fonctions_discrimination(FluxSortieCPP &os,
                                                kuri::chaine_statique nom_classe,
                                                ProteineStruct const &derivee,
                                                bool pour_noeud_code)
{
    const auto &nom_comme = derivee.accede_nom_comme();
    const auto &nom_noeud = pour_noeud_code ? derivee.accede_nom_code() : derivee.nom();
    const auto &nom_genre = derivee.accede_nom_genre();

    // À FAIRE: si/discr
    if (derivee.est_racine_soushierachie() && nom_genre.nom_cpp() == "") {
        os << "inline bool " << nom_classe << "::est_" << nom_comme << "() const\n";
        os << "{\n";
        os << "\t ";

        auto separateur = "return";

        for (auto &derive : derivee.derivees()) {
            os << separateur << " this->est_" << derive->accede_nom_comme() << "()";
            separateur = "|| ";
        }

        os << ";\n";
        os << "}\n\n";
    }
    else {
        os << "inline bool " << nom_classe << "::est_" << nom_comme << "() const\n";
        os << "{\n";

        if (derivee.est_racine_soushierachie()) {
            os << "\treturn this->genre == GenreNoeud::" << nom_genre;

            for (auto &derive : derivee.derivees()) {
                os << " || this->genre == GenreNoeud::" << derive->accede_nom_genre();
            }

            os << ";\n";
        }
        else {
            os << "\treturn this->genre == GenreNoeud::" << nom_genre << ";\n";
        }

        os << "}\n\n";
    }

    os << "inline " << nom_noeud << " *" << nom_classe << "::comme_" << nom_comme << "()\n";
    os << "{\n";
    os << "\tassert_rappel(est_" << nom_comme
       << "(), [this]() { imprime_genre_noeud_pour_assert(this); });\n";
    os << "\treturn static_cast<" << nom_noeud << " *>(this);\n";
    os << "}\n\n";

    os << "inline const " << nom_noeud << " *" << nom_classe << "::comme_" << nom_comme
       << "() const\n";
    os << "{\n";
    os << "\tassert_rappel(est_" << nom_comme
       << "(), [this]() { imprime_genre_noeud_pour_assert(this); });\n";
    os << "\treturn static_cast<const " << nom_noeud << " *>(this);\n";
    os << "}\n\n";
}

void génère_code_cpp(FluxSortieCPP &os,
                     const kuri::tableau<Proteine *> &proteines,
                     bool pour_entête)
{
    POUR (proteines) {
        it->genere_code_cpp(os, pour_entête);
    }
}

void génère_code_kuri(FluxSortieKuri &os, const kuri::tableau<Proteine *> &proteines)
{
    POUR (proteines) {
        it->genere_code_kuri(os);
    }
}

/** \} */

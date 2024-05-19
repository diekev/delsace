/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "impression.hh"

#include "cas_genre_noeud.hh"
#include "etendue_code_source.hh"
#include "noeud_expression.hh"

#include "biblinternes/langage/unicode.hh"

#include "compilation/bibliotheque.hh"
#include "compilation/operateurs.hh"
#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "structures/enchaineuse.hh"

#include "utilitaires/log.hh"

/*
formattage :
    - indentation avant les sous-blocs (Vision, Dessin)
    - piège nonatteignable est ajouté (Berechtof)
    - commentaire entre si et sinon
    - commentaire entre les expressions des discrs
    - commentaire après { ou } (Delsace)
    - parenthèses expression retour
    - #si après sinon (Bits)
    - virgule après retour contenant un commentaire en fin de ligne (Courriel)
*/

enum class GenreNoeudFormattage : int8_t {
    CHAINE,
    IDENTIFIANT,
    LIGNE,
    ESPACE_OU_LIGNE,
    INDENTATION,
    GROUPE,
    NOEUDS,
};

struct NoeudFormattage {
    GenreNoeudFormattage genre;

    NoeudFormattage(GenreNoeudFormattage genre_) : genre(genre_)
    {
    }
};

struct NoeudFormattageIdentifiant : public NoeudFormattage {
    IdentifiantCode *ident = nullptr;
};

/* Pour les mots-clés et les ponctuations. */
struct NoeudFormattageChaine : public NoeudFormattage {
    kuri::chaine_statique chaine = "";
};

struct NoeudFormattageGroupe : public NoeudFormattage {
    kuri::tableau<NoeudFormattage *, int> noeuds{};
};

struct NoeudFormattageNoeuds : public NoeudFormattage {
    kuri::tableau<NoeudFormattage *, int> noeuds{};
};

struct GénératriceTexte {
  private:
    void génère_texte(NoeudFormattage const *noeud);

  private:
    void ajoute_texte(kuri::chaine_statique texte);
};

void GénératriceTexte::génère_texte(NoeudFormattage const *noeud)
{
    switch (noeud->genre) {
        case GenreNoeudFormattage::CHAINE:
        {
            auto noeud_chaine = static_cast<NoeudFormattageChaine const *>(noeud);
            ajoute_texte(noeud_chaine->chaine);
            break;
        }
        case GenreNoeudFormattage::IDENTIFIANT:
        {
            auto noeud_chaine = static_cast<NoeudFormattageIdentifiant const *>(noeud);
            ajoute_texte(noeud_chaine->ident->nom);
            break;
        }
        case GenreNoeudFormattage::LIGNE:
        {
            // si besoin retour
            break;
        }
        case GenreNoeudFormattage::ESPACE_OU_LIGNE:
        {
            // si besoin retour -> ligne
            ajoute_texte(" ");
            break;
        }
        case GenreNoeudFormattage::INDENTATION:
        {
            // si besoin retour
            break;
        }
        case GenreNoeudFormattage::GROUPE:
        {
            break;
        }
        case GenreNoeudFormattage::NOEUDS:
        {
            break;
        }
    }
}

void GénératriceTexte::ajoute_texte(kuri::chaine_statique texte)
{
}

static kuri::chaine_statique chaine_indentations_espace(int indentations)
{
    static std::string chaine = std::string(1024, ' ');
    return {chaine.c_str(), static_cast<int64_t>(indentations * 4)};
}

static std::ostream &operator<<(std::ostream &os, Indentation const indent)
{
    os << chaine_indentations_espace(indent.v);
    return os;
}

static NoeudExpression const *donne_expression_sans_parenthèse(NoeudExpression const *expression)
{
    while (expression->est_parenthèse()) {
        expression = expression->comme_parenthèse()->expression;
    }
    return expression;
}

static void imprime_ident(Enchaineuse &enchaineuse, IdentifiantCode const *ident)
{
    if (!ident) {
        return;
    }

    enchaineuse << ident->nom;
}

static void imprime_lexème_mot_clé(Enchaineuse &enchaineuse,
                                   NoeudExpression const *noeud,
                                   bool avec_espace)
{
    enchaineuse << noeud->lexème->chaine;
    if (avec_espace) {
        enchaineuse << " ";
    }
}

static void imprime_lexème_mot_clé(Enchaineuse &enchaineuse,
                                   kuri::chaine_statique mot_clé,
                                   bool avec_espace)
{
    enchaineuse << mot_clé;
    if (avec_espace) {
        enchaineuse << " ";
    }
}

static bool le_noeud_est_sur_une_ligne(NoeudExpression const *noeud)
{
    auto étendue_bloc = donne_étendue_source_noeud(noeud);
    return étendue_bloc.ligne_début == étendue_bloc.ligne_fin;
}

bool expression_eu_bloc(NoeudExpression const *noeud)
{
    switch (noeud->genre) {
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::INSTRUCTION_TANTQUE:
        case GenreNoeud::INSTRUCTION_POUR:
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        case GenreNoeud::INSTRUCTION_COMPOSÉE:
        case GenreNoeud::DÉCLARATION_UNION:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:
        {
            return true;
        }
        case GenreNoeud::INSTRUCTION_SI:
        case GenreNoeud::INSTRUCTION_SAUFSI:
        {
            return !le_noeud_est_sur_une_ligne(noeud);
        }
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            auto entête = noeud->comme_entête_fonction();
            return !entête->possède_drapeau(DrapeauxNoeud::EST_EXTERNE);
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto instruction = noeud->comme_tente();
            return instruction->bloc;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto structure = noeud->comme_type_structure();
            return structure->bloc;
        }
        case GenreNoeud::INSTRUCTION_DIFFÈRE:
        {
            auto instruction = noeud->comme_diffère();
            auto expression = instruction->expression;
            if (expression->est_bloc()) {
                auto bloc = expression->comme_bloc();
                if (bloc->expressions->taille() == 1) {
                    return expression_eu_bloc(bloc->expressions->a(0));
                }
                return true;
            }
            return expression_eu_bloc(expression);
        }
        case GenreNoeud::DIRECTIVE_EXÉCUTE:
        {
            return noeud->ident == ID::test;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE:
        {
            auto déclaration = noeud->comme_déclaration_variable();
            if (!déclaration->expression) {
                return false;
            }
            return expression_eu_bloc(déclaration->expression);
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto expression = noeud->comme_assignation_variable();
            if (!expression->expression) {
                return false;
            }
            return expression_eu_bloc(expression->expression);
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto expression = noeud->comme_expression_binaire();
            auto type_op = expression->lexème->genre;
            if (est_assignation_composée(type_op)) {
                return expression_eu_bloc(expression->opérande_droite);
            }
            return false;
        }
        default:
        {
            return false;
        }
    }
}

static bool doit_ajouter_guillemets(kuri::chaine_statique chn)
{
    if (chn.taille() == 0) {
        return true;
    }

    if (chn.pointeur()[0] == '"') {
        return false;
    }

    // À FAIRE : dernier caractère...
    auto nombre_octet = lng::nombre_octets(chn.pointeur());
    if (nombre_octet == 0) {
        return true;
    }

    auto rune = lng::converti_utf32(chn.pointeur(), nombre_octet);
    return rune != GUILLEMET_OUVRANT;
}

static bool doit_ajouter_apostrophes(kuri::chaine_statique chn)
{
    if (chn.taille() == 0) {
        return true;
    }

    if (chn.pointeur()[0] != '\'') {
        return true;
    }

    return false;
}

static void imprime_lexème_chaine_littérale(Enchaineuse &enchaineuse, Lexème const *lexème)
{
    if (doit_ajouter_guillemets(lexème->chaine)) {
        enchaineuse << "\"" << lexème->chaine << "\"";
    }
    else {
        enchaineuse << lexème->chaine;
    }
}

struct ÉtatImpression {
    Indentation indent{0};
    bool préfére_substitution = false;
    bool imprime_indent_avant_bloc = true;
    bool imprime_nouvelle_ligne_après_bloc = true;
};

static void imprime_arbre(Enchaineuse &enchaineuse,
                          ÉtatImpression état,
                          NoeudExpression const *noeud);

static ÉtendueSourceNoeud donne_étendue_source(
    kuri::tableau_statique<NoeudExpression *> expressions)
{
    auto résultat = ÉtendueSourceNoeud{};

    POUR (expressions) {
        auto étendue = donne_étendue_source_noeud(it);
        résultat.fusionne(étendue);
    }

    return résultat;
}

static void imprime_tableau_expression(Enchaineuse &enchaineuse,
                                       ÉtatImpression état,
                                       kuri::tableau_statique<NoeudExpression *> expressions,
                                       kuri::chaine_statique parenthèse_début,
                                       kuri::chaine_statique parenthèse_fin)
{
    if (expressions.taille() == 0) {
        enchaineuse << parenthèse_début << parenthèse_fin;
        return;
    }

    auto virgule = parenthèse_début;
    auto étendue = donne_étendue_source(expressions);

    auto séparation = ", ";
    auto séparation_est_nouvelle_ligne = false;
    if (étendue.ligne_début != étendue.ligne_fin) {
        séparation = ",\n";
        séparation_est_nouvelle_ligne = true;

        enchaineuse << parenthèse_début << "\n";
        virgule = "";
    }

    auto indent = état.indent;
    indent.v += 1;

    auto ignore_suivant = false;

    POUR_INDEX (expressions) {
        if (ignore_suivant) {
            ignore_suivant = false;
            continue;
        }

        enchaineuse << virgule;
        if (séparation_est_nouvelle_ligne) {
            enchaineuse << indent;
        }

        imprime_arbre(enchaineuse, état, it);

        if (index_it < expressions.taille() - 1) {
            auto expression_suivante = expressions[index_it + 1];
            if (expression_suivante->est_commentaire() &&
                expression_suivante->lexème->ligne == it->lexème->ligne) {
                enchaineuse << ", ";
                imprime_arbre(enchaineuse, état, expression_suivante);
                ignore_suivant = true;
                if (séparation_est_nouvelle_ligne) {
                    virgule = "\n";
                }
                continue;
            }
        }

        virgule = séparation;
    }

    if (expressions.taille() == 0) {
        enchaineuse << virgule;
    }
    if (séparation_est_nouvelle_ligne) {
        enchaineuse << "\n" << état.indent;
    }
    enchaineuse << parenthèse_fin;
}

static void imprime_annotations(Enchaineuse &enchaineuse,
                                kuri::tableau_statique<Annotation> annotations)
{
    POUR (annotations) {
        enchaineuse << " @" << it.nom;
        if (it.valeur.taille() == 0) {
            continue;
        }

        enchaineuse << " ";
        if (doit_ajouter_guillemets(it.valeur)) {
            enchaineuse << "\"" << it.valeur << "\"";
        }
        else {
            enchaineuse << it.valeur;
        }
    }
}

static void imprime_données_externes(Enchaineuse &enchaineuse,
                                     DonnéesSymboleExterne const *données,
                                     IdentifiantCode const *ident_symbole)
{
    if (!données) {
        return;
    }

    enchaineuse << " #externe ";
    imprime_ident(enchaineuse, données->ident_bibliothèque);
    if (données->nom_symbole.taille() != 0 && données->nom_symbole != ident_symbole->nom) {
        enchaineuse << " \"" << données->nom_symbole << "\"";
    }
}

static void imprime_paramètres_classe(Enchaineuse &enchaineuse, NoeudBloc const *bloc_constantes)
{
    if (!bloc_constantes) {
        return;
    }

    auto virgule = " (";

    POUR (*bloc_constantes->membres.verrou_lecture()) {
        enchaineuse << virgule;
        imprime_arbre(enchaineuse, {}, it);
        virgule = ", ";
    }

    enchaineuse << ")";
}

static void imprime_directives(Enchaineuse &enchaineuse,
                               ÉtatImpression état,
                               kuri::tableau_statique<NoeudDirectiveFonction *> directives)
{
    POUR (directives) {
        /* #corps_texte doit être imprimé différemment. */
        if (it->ident == ID::corps_texte) {
            continue;
        }
        imprime_arbre(enchaineuse, état, it);
    }
}

static void imprime_bloc(Enchaineuse &enchaineuse,
                         ÉtatImpression état,
                         NoeudBloc const *bloc,
                         bool const appartiens_à_module)
{
    auto chaine_nouvelle_ligne = kuri::chaine_statique("\n");
    auto le_bloc_est_sur_une_ligne = false;

    if (!appartiens_à_module) {
        if (le_noeud_est_sur_une_ligne(bloc)) {
            /* Le bloc est sur une seule ligne, utilisons des espaces plutôt que des nouvelles
             * lignes. */
            chaine_nouvelle_ligne = " ";
            le_bloc_est_sur_une_ligne = true;

            if (bloc->expressions->est_vide()) {
                if (état.imprime_indent_avant_bloc) {
                    enchaineuse << état.indent;
                }

                enchaineuse << "{}";

                if (état.imprime_nouvelle_ligne_après_bloc) {
                    enchaineuse << "\n";
                }

                return;
            }
        }

        if (état.imprime_indent_avant_bloc) {
            enchaineuse << état.indent;
        }

        if (bloc->est_nonsur) {
            enchaineuse << "nonsûr ";
        }

        enchaineuse << "{" << chaine_nouvelle_ligne;
        état.indent.v += 1;
    }

    std::optional<int> dernière_ligne_lexème;

    auto imprime_nouvelle_ligne_après_bloc = état.imprime_nouvelle_ligne_après_bloc;
    état.imprime_nouvelle_ligne_après_bloc = true;

    auto expressions = bloc->expressions.verrou_lecture();

    /* Pour les commentaires en fin de ligne. */
    auto ignore_indentation = false;

    POUR_INDEX (*expressions) {
        /* Ignore les expressions ajoutées lors de la validation sémantique (par exemple,
         * les variables capturées par les discriminations). */
        if (it->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE) && !état.préfére_substitution) {
            continue;
        }

        /* Essaie de préserver les séparations dans le texte originel. */
        if (dernière_ligne_lexème.has_value()) {
            if (it->lexème->ligne > (dernière_ligne_lexème.value() + 1)) {
                enchaineuse << "\n";
            }
        }

        if (!le_bloc_est_sur_une_ligne && !it->est_bloc() && !ignore_indentation) {
            enchaineuse << état.indent;
        }

        ignore_indentation = false;

        imprime_arbre(enchaineuse, état, it);

        /* Vérifie si l'expression suivante est un commentaire en fin de ligne. */
        auto commentaire_sur_même_ligne = false;
        if (index_it < expressions->taille() - 1) {
            auto expression_suivante = (*expressions)[index_it + 1];
            if (expression_suivante->est_commentaire()) {
                if (expression_suivante->lexème->ligne == it->lexème->ligne) {
                    commentaire_sur_même_ligne = true;
                    ignore_indentation = true;
                }
            }
        }

        /* N'insèrons pas de nouvelle ligne si la dernière expression eu un bloc (car ce
         * fut déjà fait). */
        if (!expression_eu_bloc(it)) {
            if (commentaire_sur_même_ligne) {
                enchaineuse << " ";
            }
            else {
                enchaineuse << chaine_nouvelle_ligne;
            }
        }

        dernière_ligne_lexème = donne_étendue_source_noeud(it).ligne_fin;
    }

    if (!appartiens_à_module) {
        état.indent.v -= 1;

        if (!le_bloc_est_sur_une_ligne) {
            enchaineuse << état.indent;
        }
        enchaineuse << "}";

        if (imprime_nouvelle_ligne_après_bloc) {
            enchaineuse << "\n";
        }
    }
}

static void imprime_arbre(Enchaineuse &enchaineuse,
                          ÉtatImpression état,
                          NoeudExpression const *noeud)
{
    if (!noeud) {
        return;
    }

    if (état.préfére_substitution && noeud->substitution) {
        imprime_arbre(enchaineuse, état, noeud->substitution);
        return;
    }

    switch (noeud->genre) {
        case GenreNoeud::DÉCLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        {
            /* Rien à faire, mais imprime un message au cas où. */
            dbg() << "Genre Noeud nom géré : " << noeud->genre;
            break;
        }
        case GenreNoeud::COMMENTAIRE:
        {
            /* VSCode n'insère pas proprement les fins de sections... */
            if (noeud->lexème->chaine == "/** } */") {
                enchaineuse << "/** \\} */";
            }
            else {
                enchaineuse << noeud->lexème->chaine;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            auto inst = noeud->comme_importe();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, inst->expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        {
            auto inst = noeud->comme_charge();
            const auto lexème = inst->expression->lexème;
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_lexème_chaine_littérale(enchaineuse, lexème);
            break;
        }
        case GenreNoeud::DÉCLARATION_BIBLIOTHÈQUE:
        {
            auto déclaration = noeud->comme_déclaration_bibliothèque();
            imprime_ident(enchaineuse, déclaration->ident);
            enchaineuse << " :: #bibliothèque ";
            imprime_lexème_chaine_littérale(enchaineuse, déclaration->lexème_nom_bibliothèque);
            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto structure = noeud->comme_type_structure();
            imprime_ident(enchaineuse, structure->ident);
            enchaineuse << " :: struct";
            imprime_paramètres_classe(enchaineuse, structure->bloc_constantes);
            imprime_directives(enchaineuse, état, structure->directives);
            if (!structure->bloc) {
                break;
            }
            enchaineuse << " ";
            if (structure->est_corps_texte) {
                enchaineuse << "#corps_texte ";
            }
            état.imprime_indent_avant_bloc = false;
            état.imprime_nouvelle_ligne_après_bloc = false;
            imprime_arbre(enchaineuse, état, structure->bloc);
            imprime_annotations(enchaineuse, structure->annotations);
            enchaineuse << "\n";
            break;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto structure = noeud->comme_type_union();
            imprime_ident(enchaineuse, structure->ident);
            enchaineuse << " :: union";
            if (structure->est_nonsure) {
                enchaineuse << " nonsûr";
            }
            imprime_paramètres_classe(enchaineuse, structure->bloc_constantes);
            imprime_directives(enchaineuse, état, structure->directives);
            if (!structure->bloc) {
                break;
            }
            enchaineuse << " ";
            if (structure->est_corps_texte) {
                enchaineuse << "#corps_texte ";
            }
            état.imprime_indent_avant_bloc = false;
            état.imprime_nouvelle_ligne_après_bloc = false;
            imprime_arbre(enchaineuse, état, structure->bloc);
            imprime_annotations(enchaineuse, structure->annotations);
            enchaineuse << "\n";
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto énumération = noeud->comme_type_énum();
            imprime_ident(enchaineuse, énumération->ident);
            if (énumération->est_type_enum_drapeau()) {
                enchaineuse << " :: énum_drapeau ";
            }
            else if (énumération->est_type_erreur()) {
                enchaineuse << " :: erreur ";
            }
            else {
                enchaineuse << " :: énum ";
            }
            if (énumération->expression_type) {
                imprime_arbre(enchaineuse, état, énumération->expression_type);
                enchaineuse << " ";
            }
            état.imprime_indent_avant_bloc = false;
            état.imprime_nouvelle_ligne_après_bloc = false;
            imprime_arbre(enchaineuse, état, énumération->bloc);
            imprime_annotations(enchaineuse, énumération->annotations);
            enchaineuse << "\n";
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto déclaration = noeud->comme_type_opaque();
            imprime_ident(enchaineuse, déclaration->ident);
            enchaineuse << " :: #opaque ";
            imprime_arbre(enchaineuse, état, déclaration->expression_type);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            auto entête = noeud->comme_entête_fonction();

            if (entête->est_opérateur_pour()) {
                enchaineuse << "opérateur pour";
            }
            else if (entête->est_opérateur) {
                enchaineuse << "opérateur ";
                if (entête->lexème->genre == GenreLexème::CROCHET_OUVRANT) {
                    enchaineuse << "[]";
                }
                else {
                    imprime_lexème_mot_clé(enchaineuse, entête, false);
                }
            }
            else {
                imprime_ident(enchaineuse, entête->ident);
            }
            enchaineuse << " :: fonc ";

            imprime_tableau_expression(enchaineuse, état, entête->params, "(", ")");

            if (!entête->param_sortie->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE)) {
                enchaineuse << " -> ";
                imprime_tableau_expression(enchaineuse, état, entête->params_sorties, "", "");
            }

            imprime_directives(enchaineuse, état, entête->directives);

            if (entête->possède_drapeau(DrapeauxNoeud::EST_EXTERNE)) {
                break;
            }

            enchaineuse << "\n";

            état.imprime_nouvelle_ligne_après_bloc = false;
            if (entête->possède_drapeau(DrapeauxNoeudFonction::BLOC_CORPS_EST_POUSSE_CONTEXTE)) {
                auto const bloc_corps = entête->corps->bloc;
                auto const expression = bloc_corps->expressions->a(0);
                imprime_arbre(enchaineuse, état, expression);
            }
            else {
                imprime_arbre(enchaineuse, état, entête->corps);
            }
            imprime_annotations(enchaineuse, entête->annotations);
            enchaineuse << "\n";
            break;
        }
        case GenreNoeud::DIRECTIVE_FONCTION:
        {
            auto directive = noeud->comme_directive_fonction();
            enchaineuse << " #";
            imprime_ident(enchaineuse, directive->ident);

            if (directive->ident == ID::externe && directive->opérandes.taille() != 0) {
                /* Cas spécial pour #externe. Nous ne devons pas séparer les opérandes par des
                 * virgules. */
                enchaineuse << " " << directive->opérandes[0]->chaine;
                if (directive->opérandes.taille() == 2) {
                    enchaineuse << " ";
                    imprime_lexème_chaine_littérale(enchaineuse, directive->opérandes[1]);
                }
                break;
            }

            auto virgule = " ";
            POUR (directive->opérandes) {
                enchaineuse << virgule << it->chaine;
                virgule = ",";
            }

            break;
        }
        case GenreNoeud::DÉCLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();

            if (corps->est_corps_texte) {
                enchaineuse << "#corps_texte ";
            }

            imprime_arbre(enchaineuse, état, corps->bloc);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSÉE:
        {
            auto bloc = noeud->comme_bloc();
            imprime_bloc(enchaineuse, état, bloc, false);
            break;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        {
            auto inst = noeud->comme_boucle();
            état.imprime_indent_avant_bloc = false;
            imprime_lexème_mot_clé(enchaineuse, "boucle", true);
            imprime_arbre(enchaineuse, état, inst->bloc);
            if (inst->bloc_inc) {
                enchaineuse << état.indent << "inc ";
                imprime_arbre(enchaineuse, état, inst->bloc_inc);
            }
            if (inst->bloc_sansarrêt) {
                enchaineuse << état.indent << "sansarrêt ";
                imprime_arbre(enchaineuse, état, inst->bloc_sansarrêt);
            }
            if (inst->bloc_sinon) {
                enchaineuse << état.indent << "sinon ";
                imprime_arbre(enchaineuse, état, inst->bloc_sinon);
            }
            break;
        }
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto inst = noeud->comme_tantque();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, donne_expression_sans_parenthèse(inst->condition));
            enchaineuse << " ";
            état.imprime_indent_avant_bloc = false;
            imprime_arbre(enchaineuse, état, inst->bloc);
            break;
        }
        case GenreNoeud::INSTRUCTION_RÉPÈTE:
        {
            auto inst = noeud->comme_répète();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            état.imprime_indent_avant_bloc = false;
            état.imprime_nouvelle_ligne_après_bloc = false;
            imprime_arbre(enchaineuse, état, inst->bloc);
            enchaineuse << " tantque ";
            imprime_arbre(enchaineuse, état, donne_expression_sans_parenthèse(inst->condition));
            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            auto inst = noeud->comme_pour();
            imprime_lexème_mot_clé(enchaineuse, "pour", true);

            /* Inférieur est la valeur par défaut, donc n'imprimons de lexème que si nous avons une
             * itération inverse. */
            if (inst->lexème_op == GenreLexème::SUPERIEUR) {
                enchaineuse << "> ";
            }

            if (inst->prend_pointeur) {
                enchaineuse << "* ";
            }
            else if (inst->prend_référence) {
                enchaineuse << "& ";
            }

            if (!inst->variable->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE)) {
                imprime_arbre(enchaineuse, état, inst->variable);
                enchaineuse << " dans ";
            }

            imprime_arbre(enchaineuse, état, inst->expression);
            enchaineuse << " ";

            état.imprime_indent_avant_bloc = false;
            imprime_arbre(enchaineuse, état, inst->bloc);

            if (inst->bloc_sansarrêt) {
                enchaineuse << état.indent << "sansarrêt ";
                imprime_arbre(enchaineuse, état, inst->bloc_sansarrêt);
            }

            if (inst->bloc_sinon) {
                enchaineuse << état.indent << "sinon ";
                imprime_arbre(enchaineuse, état, inst->bloc_sinon);
            }

            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE:
        {
            auto déclaration = noeud->comme_déclaration_variable();

            /* Pour les retours de fonctions sans nom. */
            if (déclaration->possède_drapeau(DrapeauxNoeud::IDENT_EST_DÉFAUT)) {
                imprime_arbre(enchaineuse, état, déclaration->expression_type);
                return;
            }

            imprime_ident(enchaineuse, déclaration->ident);

            auto séparation_expression = kuri::chaine_statique(" := ");

            if (déclaration->expression_type) {
                enchaineuse << ((déclaration->expression != nullptr) ? " : " : ": ");
                imprime_arbre(enchaineuse, état, déclaration->expression_type);
                séparation_expression = " = ";
            }
            else if (!déclaration->expression) {
                /* Les substitutions et autres déclarations implicites n'ont peut être ni
                 * expression de type, ni expression. */
                enchaineuse << ": " << chaine_type(déclaration->type);
            }

            if (déclaration->expression) {
                enchaineuse << séparation_expression;
                imprime_arbre(enchaineuse, état, déclaration->expression);
            }

            imprime_annotations(enchaineuse, déclaration->annotations);
            imprime_données_externes(
                enchaineuse, déclaration->données_externes, déclaration->ident);
            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE_MULTIPLE:
        {
            auto déclaration = noeud->comme_déclaration_variable_multiple();

            imprime_arbre(enchaineuse, état, déclaration->valeur);

            auto séparation_expression = kuri::chaine_statique(" := ");

            if (déclaration->expression_type) {
                enchaineuse << ((déclaration->expression != nullptr) ? " : " : ": ");
                imprime_arbre(enchaineuse, état, déclaration->expression_type);
                séparation_expression = " = ";
            }

            if (déclaration->expression) {
                enchaineuse << séparation_expression;
                imprime_arbre(enchaineuse, état, déclaration->expression);
            }

            imprime_annotations(enchaineuse, déclaration->annotations);
            break;
        }
        case GenreNoeud::DÉCLARATION_CONSTANTE:
        {
            auto déclaration = noeud->comme_déclaration_constante();

            if (déclaration->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
                enchaineuse << "$";
            }

            imprime_ident(enchaineuse, déclaration->ident);

            auto séparation_expression = kuri::chaine_statique(" :: ");

            if (déclaration->expression_type) {
                enchaineuse << ((déclaration->expression != nullptr) ? " : " : ": ");
                imprime_arbre(enchaineuse, état, déclaration->expression_type);
                séparation_expression = " : ";
            }

            if (déclaration->expression) {
                enchaineuse << séparation_expression;
                imprime_arbre(enchaineuse, état, déclaration->expression);
            }

            imprime_annotations(enchaineuse, déclaration->annotations);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto assignation = noeud->comme_assignation_variable();
            imprime_arbre(enchaineuse, état, assignation->assignée);
            enchaineuse << " = ";
            imprime_arbre(enchaineuse, état, assignation->expression);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:
        {
            auto assignation = noeud->comme_assignation_multiple();
            imprime_arbre(enchaineuse, état, assignation->assignées);
            enchaineuse << " = ";
            imprime_arbre(enchaineuse, état, assignation->expression);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_LOGIQUE:
        {
            auto expression = noeud->comme_assignation_logique();
            imprime_arbre(enchaineuse, état, expression->opérande_gauche);
            enchaineuse << " ";
            imprime_lexème_mot_clé(enchaineuse, expression, true);
            imprime_arbre(enchaineuse, état, expression->opérande_droite);
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            if (noeud->possède_drapeau(DrapeauxNoeud::IDENTIFIANT_EST_ACCENTUÉ_GRAVE)) {
                enchaineuse << "`";
            }
            if (noeud->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
                enchaineuse << "$";
            }
            auto référence = noeud->comme_référence_déclaration();
            if (référence->ident) {
                imprime_ident(enchaineuse, référence->ident);
            }
            else if (référence->déclaration_référée) {
                enchaineuse << nom_humainement_lisible(référence->déclaration_référée);
            }
            else {
                enchaineuse << noeud->lexème->chaine;
            }
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE_UNION:
        {
            auto membre = noeud->comme_référence_membre();
            imprime_arbre(enchaineuse, état, membre->accédée);
            enchaineuse << ".";
            imprime_ident(enchaineuse, noeud->ident);
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        {
            enchaineuse << noeud->lexème->chaine;
            break;
        }
        case GenreNoeud::EXPRESSION_APPEL:
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            auto appel = noeud->comme_appel();
            imprime_arbre(enchaineuse, état, appel->expression);
            /* Le deuxième cas est pour les fonctions d'initialisation. */
            if (état.préfére_substitution ||
                (appel->paramètres.taille() == 0 && appel->paramètres_résolus.taille() != 0)) {
                imprime_tableau_expression(enchaineuse, état, appel->paramètres_résolus, "(", ")");
            }
            else {
                imprime_tableau_expression(enchaineuse, état, appel->paramètres, "(", ")");
            }
            break;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto expression = noeud->comme_plage();
            imprime_arbre(enchaineuse, état, expression->début);
            enchaineuse << " ... ";
            imprime_arbre(enchaineuse, état, expression->fin);
            break;
        }
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        {
            auto expression = noeud->comme_parenthèse();
            enchaineuse << "(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:
        {
            auto expression = noeud->comme_négation_logique();
            enchaineuse << "!";
            imprime_arbre(enchaineuse, état, expression->opérande);
            break;
        }
        case GenreNoeud::EXPRESSION_MÉMOIRE:
        {
            auto expression = noeud->comme_mémoire();
            enchaineuse << "mémoire(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            auto inst = noeud->comme_empl();
            enchaineuse << "empl ";
            imprime_arbre(enchaineuse, état, inst->expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        {
            enchaineuse << "---";
            break;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            auto inst = noeud->comme_retiens();
            imprime_lexème_mot_clé(enchaineuse, "retiens", inst->expression != nullptr);
            imprime_arbre(enchaineuse, état, inst->expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = noeud->comme_retourne();
            imprime_lexème_mot_clé(enchaineuse, "retourne", inst->expression != nullptr);
            imprime_arbre(enchaineuse, état, inst->expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
        {
            auto inst = noeud->comme_retourne_multiple();
            imprime_lexème_mot_clé(enchaineuse, "retourne", true);
            imprime_arbre(enchaineuse, état, inst->expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_SI:
        case GenreNoeud::INSTRUCTION_SAUFSI:
        {
            auto inst = noeud->comme_si();
            if (inst->est_saufsi()) {
                imprime_lexème_mot_clé(enchaineuse, "saufsi", true);
            }
            else {
                imprime_lexème_mot_clé(enchaineuse, "si", true);
            }
            imprime_arbre(enchaineuse, état, donne_expression_sans_parenthèse(inst->condition));
            état.imprime_indent_avant_bloc = false;
            auto est_sur_même_ligne = le_noeud_est_sur_une_ligne(inst);
            état.imprime_nouvelle_ligne_après_bloc = !est_sur_même_ligne;
            enchaineuse << " ";
            imprime_arbre(enchaineuse, état, inst->bloc_si_vrai);
            if (inst->bloc_si_faux) {
                if (!est_sur_même_ligne) {
                    enchaineuse << état.indent;
                }
                else {
                    enchaineuse << " ";
                }
                enchaineuse << "sinon ";
                imprime_arbre(enchaineuse, état, inst->bloc_si_faux);
            }
            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();
            enchaineuse << "#";
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, donne_expression_sans_parenthèse(inst->condition));
            état.imprime_indent_avant_bloc = false;
            enchaineuse << " ";
            imprime_arbre(enchaineuse, état, inst->bloc_si_vrai);
            if (inst->bloc_si_faux) {
                enchaineuse << état.indent << "sinon ";
                imprime_arbre(enchaineuse, état, inst->bloc_si_faux);
            }
            break;
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ÉNUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto inst = noeud->comme_discr();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, inst->expression_discriminée);

            enchaineuse << " {\n";

            état.imprime_indent_avant_bloc = false;
            état.indent.v += 1;
            POUR (inst->paires_discr) {
                enchaineuse << état.indent;
                imprime_arbre(enchaineuse, état, it->expression);
                enchaineuse << " ";
                imprime_arbre(enchaineuse, état, it->bloc);
            }

            if (inst->bloc_sinon) {
                enchaineuse << état.indent << "sinon ";
                imprime_arbre(enchaineuse, état, inst->bloc_sinon);
            }

            état.indent.v -= 1;

            enchaineuse << état.indent << "}";
            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFÈRE:
        {
            auto inst = noeud->comme_diffère();
            imprime_lexème_mot_clé(enchaineuse, inst, true);

            /* Supprime les accolades pour s'il n'y a qu'une seule expression. */
            auto expression = inst->expression;
            if (expression->est_bloc()) {
                auto bloc = expression->comme_bloc();
                if (bloc->expressions->taille() == 1) {
                    expression = (*bloc->expressions.verrou_lecture())[0];
                }
            }

            état.imprime_indent_avant_bloc = false;
            imprime_arbre(enchaineuse, état, expression);
            break;
        }
        case GenreNoeud::INSTRUCTION_ARRÊTE:
        case GenreNoeud::INSTRUCTION_CONTINUE:
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = noeud->comme_controle_boucle();
            kuri::chaine_statique mot_clé;
            if (inst->est_arrête()) {
                mot_clé = "arrête";
            }
            else if (inst->est_continue()) {
                mot_clé = "continue";
            }
            else {
                mot_clé = "reprends";
            }
            imprime_lexème_mot_clé(enchaineuse, mot_clé, inst->expression != nullptr);
            if (inst->expression) {
                imprime_arbre(enchaineuse, état, inst->expression);
            }
            break;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto inst = noeud->comme_pousse_contexte();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, inst->expression);
            enchaineuse << " ";
            état.imprime_indent_avant_bloc = false;
            imprime_arbre(enchaineuse, état, inst->bloc);
            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = noeud->comme_tente();
            imprime_lexème_mot_clé(enchaineuse, inst, true);
            imprime_arbre(enchaineuse, état, inst->expression_appelée);
            enchaineuse << " piège ";
            if (inst->bloc) {
                imprime_arbre(enchaineuse, état, inst->expression_piégée);
                enchaineuse << " ";
                état.imprime_indent_avant_bloc = false;
                imprime_arbre(enchaineuse, état, inst->bloc);
            }
            else {
                enchaineuse << "nonatteignable";
            }
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        {
            auto lexème = noeud->lexème;
            /* Pour les subsitutition. */
            if (lexème->genre != GenreLexème::NOMBRE_ENTIER) {
                auto littérale = noeud->comme_littérale_entier();
                enchaineuse << littérale->valeur;
            }
            /* Vérifie la chaine du lexème car les substitutions n'ont pas de chaine. Pour les
             * lexèmes originaux, nous voulons préféserver le texte originel. */
            else if (lexème->chaine != "") {
                enchaineuse << lexème->chaine;
            }
            else {
                enchaineuse << lexème->valeur_entiere;
            }
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        {
            auto lexème = noeud->lexème;
            /* Pour les subsitutition. */
            if (lexème->genre != GenreLexème::NOMBRE_REEL) {
                auto littérale = noeud->comme_littérale_réel();
                enchaineuse << littérale->valeur;
            }
            /* Vérifie la chaine du lexème car les substitutions n'ont pas de chaine. Pour les
             * lexèmes originaux, nous voulons préféserver le texte originel. */
            else if (lexème->chaine != "") {
                enchaineuse << lexème->chaine;
            }
            else {
                enchaineuse << lexème->valeur_reelle;
            }
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        {
            auto lexème = noeud->lexème;
            /* Pour les subsitutition. */
            if (lexème->genre != GenreLexème::VRAI && lexème->genre != GenreLexème::FAUX) {
                auto littérale = noeud->comme_littérale_bool();
                enchaineuse << ((littérale->valeur) ? "vrai" : "faux");
            }
            /* Vérifie la chaine du lexème car les substitutions n'ont pas de chaine. Pour les
             * lexèmes originaux, nous voulons préféserver le texte originel. */
            else if (lexème->chaine != "") {
                enchaineuse << lexème->chaine;
            }
            else {
                enchaineuse << ((lexème->valeur_entiere) ? "vrai" : "faux");
            }
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        {
            enchaineuse << "nul";
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        {
            imprime_lexème_chaine_littérale(enchaineuse, noeud->lexème);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        {
            if (doit_ajouter_apostrophes(noeud->lexème->chaine)) {
                enchaineuse << "\"" << noeud->lexème->chaine << "\"";
            }
            else {
                enchaineuse << noeud->lexème->chaine;
            }
            break;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto expression = noeud->comme_virgule();
            imprime_tableau_expression(enchaineuse, état, expression->expressions, "", "");
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto expression = noeud->comme_prise_adresse();
            enchaineuse << "*";
            imprime_arbre(enchaineuse, état, expression->opérande);
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_RÉFÉRENCE:
        {
            auto expression = noeud->comme_prise_référence();
            enchaineuse << "&";
            imprime_arbre(enchaineuse, état, expression->opérande);
            break;
        }
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        {
            auto expression = noeud->comme_expression_unaire();
            imprime_lexème_mot_clé(enchaineuse, expression, false);
            imprime_arbre(enchaineuse, état, expression->opérande);
            break;
        }
        case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto expression = noeud->comme_expression_binaire();
            imprime_arbre(enchaineuse, état, expression->opérande_gauche);
            /* Pas d'espaces pour les contraintes polymorphiques. */
            auto const avec_espaces = !expression->opérande_gauche->possède_drapeau(
                DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE);
            if (avec_espaces) {
                enchaineuse << " ";
            }
            if (expression->op) {
                enchaineuse << donne_chaine_lexème_pour_op_binaire(expression->op->genre) << " ";
            }
            else {
                imprime_lexème_mot_clé(enchaineuse, expression, avec_espaces);
            }
            imprime_arbre(enchaineuse, état, expression->opérande_droite);
            break;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto expression = noeud->comme_expression_logique();
            imprime_arbre(enchaineuse, état, expression->opérande_gauche);
            enchaineuse << " ";
            imprime_lexème_mot_clé(enchaineuse, expression, true);
            imprime_arbre(enchaineuse, état, expression->opérande_droite);
            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expression = noeud->comme_comme();
            imprime_arbre(enchaineuse, état, expression->expression);
            if (!expression->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE)) {
                enchaineuse << " comme ";
                if (expression->expression_type) {
                    imprime_arbre(enchaineuse, état, expression->expression_type);
                }
                else if (expression->type) {
                    /* Pour l'impression des substitutions. */
                    enchaineuse << chaine_type(expression->type);
                }
            }
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto expression = noeud->comme_indexage();
            imprime_arbre(enchaineuse, état, expression->opérande_gauche);
            enchaineuse << "[";
            imprime_arbre(enchaineuse, état, expression->opérande_droite);
            enchaineuse << "]";
            break;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto expression = noeud->comme_args_variadiques();
            imprime_tableau_expression(enchaineuse, état, expression->expressions, "[", "]");
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto expression = noeud->comme_construction_tableau();
            enchaineuse << "[ ";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << " ]";
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU_TYPÉ:
        {
            auto expression = noeud->comme_construction_tableau_typé();
            imprime_arbre(enchaineuse, état, expression->expression_type);
            enchaineuse << ".[ ";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << " ]";
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expression = noeud->comme_taille_de();
            enchaineuse << "taille_de(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto expression = noeud->comme_info_de();
            enchaineuse << "info_de(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto expression = noeud->comme_init_de();
            enchaineuse << "init_de(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            auto expression = noeud->comme_type_de();
            enchaineuse << "type_de(";
            imprime_arbre(enchaineuse, état, expression->expression);
            enchaineuse << ")";
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        {
            auto expression = noeud->comme_expression_type_tableau_dynamique();
            enchaineuse << "[..]";
            imprime_arbre(enchaineuse, état, expression->expression_type);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        {
            auto expression = noeud->comme_expression_type_tableau_fixe();
            enchaineuse << "[";
            imprime_arbre(enchaineuse, état, expression->expression_taille);
            enchaineuse << "]";
            imprime_arbre(enchaineuse, état, expression->expression_type);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        {
            auto expression = noeud->comme_expression_type_tranche();
            enchaineuse << "[]";
            imprime_arbre(enchaineuse, état, expression->expression_type);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        {
            auto expression = noeud->comme_expression_type_fonction();
            enchaineuse << "fonc";
            imprime_tableau_expression(enchaineuse, état, expression->types_entrée, "(", ")");
            imprime_tableau_expression(enchaineuse, état, expression->types_sortie, "(", ")");
            break;
        }
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            auto expression = noeud->comme_expansion_variadique();
            enchaineuse << "...";
            imprime_arbre(enchaineuse, état, expression->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto directive = noeud->comme_ajoute_fini();
            enchaineuse << "#ajoute_fini ";
            imprime_arbre(enchaineuse, état, directive->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            auto directive = noeud->comme_ajoute_init();
            enchaineuse << "#ajoute_init ";
            imprime_arbre(enchaineuse, état, directive->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_PRÉ_EXÉCUTABLE:
        {
            auto directive = noeud->comme_pré_exécutable();
            enchaineuse << "#pré_exécutable ";
            imprime_arbre(enchaineuse, état, directive->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            auto directive = noeud->comme_directive_instrospection();
            enchaineuse << "#";
            imprime_ident(enchaineuse, directive->ident);
            break;
        }
        case GenreNoeud::DIRECTIVE_EXÉCUTE:
        {
            auto directive = noeud->comme_exécute();
            enchaineuse << "#";
            imprime_ident(enchaineuse, directive->ident);
            enchaineuse << " ";
            imprime_arbre(enchaineuse, état, directive->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto directive = noeud->comme_cuisine();
            enchaineuse << "#";
            imprime_ident(enchaineuse, directive->ident);
            enchaineuse << " ";
            imprime_arbre(enchaineuse, état, directive->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        {
            auto directive = noeud->comme_directive_corps_boucle();
            enchaineuse << "#";
            imprime_ident(enchaineuse, directive->ident);
            break;
        }
        case GenreNoeud::DIRECTIVE_DÉPENDANCE_BIBLIOTHÈQUE:
        {
            auto directive = noeud->comme_dépendance_bibliothèque();
            enchaineuse << "#";
            imprime_ident(enchaineuse, directive->ident);
            enchaineuse << " ";
            imprime_ident(enchaineuse, directive->bibliothèque_dépendante->ident);
            enchaineuse << " ";
            imprime_ident(enchaineuse, directive->bibliothèque_dépendue->ident);
            break;
        }
        CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX:
        {
            break;
        }
    }
}

void imprime_arbre_formatté_bloc_module(Enchaineuse &enchaineuse, NoeudBloc const *bloc)
{
    imprime_bloc(enchaineuse, ÉtatImpression{}, bloc, true);
}

void imprime_arbre_formatté(Enchaineuse &enchaineuse, NoeudExpression const *noeud)
{
    imprime_arbre(enchaineuse, ÉtatImpression{}, noeud);
}

void imprime_arbre_formatté(NoeudExpression const *noeud)
{
    Enchaineuse enchaineuse;
    imprime_arbre_formatté(enchaineuse, noeud);
    dbg() << enchaineuse.chaine();
}

void imprime_arbre_canonique_formatté(NoeudExpression const *noeud)
{
    Enchaineuse enchaineuse;
    auto état = ÉtatImpression{};
    état.préfére_substitution = true;
    imprime_arbre(enchaineuse, état, noeud);
    dbg() << enchaineuse.chaine();
}

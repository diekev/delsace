/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tacheronne.hh"

#include "parsage/lexeuse.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "compilatrice.hh"
#include "coulisse.hh"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "syntaxeuse.hh"

#include "representation_intermediaire/optimisations.hh"

const char *chaine_genre_tache(GenreTache genre)
{
#define ENUMERE_GENRE_TACHE(VERBE, ACTION, CHAINE, INDEX)                                         \
    case GenreTache::ACTION:                                                                      \
        return CHAINE;
    switch (genre) {
        ENUMERE_GENRES_TACHE(ENUMERE_GENRE_TACHE)
    }
#undef ENUMERE_GENRE_TACHE

    return "erreur";
}

std::ostream &operator<<(std::ostream &os, GenreTache genre)
{
    os << chaine_genre_tache(genre);
    return os;
}

std::ostream &operator<<(std::ostream &os, DrapeauxTacheronne drapeaux)
{
    const char *virgule = "";
#define ENUMERE_CAPACITE(VERBE, ACTION, CHAINE, INDEX)                                            \
    if (dls::outils::possede_drapeau(drapeaux, static_cast<DrapeauxTacheronne>(1 << INDEX))) {    \
        os << virgule << "PEUT_" #VERBE;                                                          \
        virgule = "|";                                                                            \
    }

    ENUMERE_TACHES_POSSIBLES(ENUMERE_CAPACITE)

#undef ENUMERE_CAPACITE
    return os;
}

Tache Tache::dors(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::DORS;
    t.espace = espace_;
    return t;
}

Tache Tache::compilation_terminee()
{
    Tache t;
    t.genre = GenreTache::COMPILATION_TERMINEE;
    return t;
}

Tache Tache::genere_fichier_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::GENERATION_CODE_MACHINE;
    t.espace = espace_;
    return t;
}

Tache Tache::liaison_objet(EspaceDeTravail *espace_)
{
    Tache t;
    t.genre = GenreTache::LIAISON_PROGRAMME;
    t.espace = espace_;
    return t;
}

static int file_pour_raison_d_etre(RaisonDEtre raison_d_etre)
{
    switch (raison_d_etre) {
        case RaisonDEtre::CHARGEMENT_FICHIER:
        {
            return OrdonnanceuseTache::FILE_CHARGEMENT;
        }
        case RaisonDEtre::LEXAGE_FICHIER:
        {
            return OrdonnanceuseTache::FILE_LEXAGE;
        }
        case RaisonDEtre::PARSAGE_FICHIER:
        {
            return OrdonnanceuseTache::FILE_PARSAGE;
        }
        case RaisonDEtre::TYPAGE:
        {
            return OrdonnanceuseTache::FILE_TYPAGE;
        }
        case RaisonDEtre::GENERATION_RI:
        case RaisonDEtre::GENERATION_RI_PRINCIPALE_MP:
        {
            return OrdonnanceuseTache::FILE_GENERATION_RI;
        }
        case RaisonDEtre::EXECUTION:
        {
            return OrdonnanceuseTache::FILE_EXECUTION;
        }
        case RaisonDEtre::GENERATION_CODE_MACHINE:
        {
            return OrdonnanceuseTache::FILE_GENERATION_CODE_MACHINE;
        }
        case RaisonDEtre::LIAISON_PROGRAMME:
        {
            return OrdonnanceuseTache::FILE_LIAISON_PROGRAMME;
        }
        case RaisonDEtre::ENVOIE_MESSAGE:
        {
            return OrdonnanceuseTache::FILE_ENVOIE_MESSAGE;
        }
        case RaisonDEtre::CONVERSION_NOEUD_CODE:
        {
            return OrdonnanceuseTache::FILE_CONVERSION_NOEUD_CODE;
        }
        case RaisonDEtre::CREATION_FONCTION_INIT_TYPE:
        {
            return OrdonnanceuseTache::FILE_CREATION_FONCTION_INIT_TYPE;
        }
        case RaisonDEtre::AUCUNE:
        {
            return -1;
        }
    }

    return -1;
}

OrdonnanceuseTache::OrdonnanceuseTache(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
{
}

void OrdonnanceuseTache::cree_tache_pour_unite(UniteCompilation *unite)
{
    assert(unite);
    assert(unite->espace);

    const auto index_file = file_pour_raison_d_etre(unite->raison_d_etre());

    auto tache = Tache{};
    tache.unite = unite;
    tache.espace = unite->espace;
    tache.genre = static_cast<GenreTache>(index_file + 2);

    taches[index_file].enfile(tache);

    pique_taille.taches[index_file] = std::max(pique_taille.taches[index_file],
                                               taches[index_file].taille());
}

long OrdonnanceuseTache::nombre_de_taches_en_attente() const
{
    auto resultat = 0l;
    POUR (taches) {
        resultat += it.taille();
    }
    return resultat;
}

Tache OrdonnanceuseTache::tache_suivante(Tache &tache_terminee, DrapeauxTacheronne drapeaux)
{
    using dls::outils::possede_drapeau;

    if (nombre_de_taches_en_attente() == 0) {
        m_compilatrice->gestionnaire_code->cree_taches(*this);
    }

    if (compilation_terminee) {
        return Tache::compilation_terminee();
    }

    auto unite = tache_terminee.unite;
    auto espace = EspaceDeTravail::nul();

    // unité peut-être nulle pour les tâches DORS du début de la compilation
    if (unite) {
        espace = unite->espace;
    }
    else {
        espace = tache_terminee.espace;
    }

    for (int i = 0; i < NOMBRE_FILES; ++i) {
        if (!possede_drapeau(drapeaux, static_cast<DrapeauxTacheronne>(1 << i))) {
            continue;
        }

        if (!taches[i].est_vide()) {
            return taches[i].defile();
        }
    }

    if (espace->possede_erreur) {
        /* Puisque l'espace possède une erreur, nous allons dormir sur l'espace par défaut de la
         * compilation. Ceci car la tâche sera retournée dans tache_suivante suivant sa complétion.
         */
        espace = m_compilatrice->espace_defaut_compilation();
    }

    return Tache::dors(espace);
}

long OrdonnanceuseTache::memoire_utilisee() const
{
    auto memoire = 0l;
    POUR (pique_taille.taches) {
        memoire += it * taille_de(Tache);
    }
    return memoire;
}

int OrdonnanceuseTache::enregistre_tacheronne(Badge<Tacheronne> /*badge*/)
{
    return nombre_de_tacheronnes++;
}

void OrdonnanceuseTache::supprime_toutes_les_taches()
{
    POUR (taches) {
        it.efface();
    }

    /* Il faut que toutes les tacheronnes soient notifiées de la fin de la compilation, donc enfile
     * un nombre égal de tâcheronnes de tâches de fin de compilation. */
    for (int i = 0; i < nombre_de_tacheronnes; ++i) {
        POUR (taches) {
            it.enfile(Tache::compilation_terminee());
        }
    }
}

void OrdonnanceuseTache::supprime_toutes_les_taches_pour_espace(const EspaceDeTravail *espace)
{
    auto predicat = [&](Tache const &tache) { return tache.espace == espace; };
    POUR (taches) {
        it.efface_si(predicat);
    }
}

void OrdonnanceuseTache::imprime_donnees_files(std::ostream &os)
{
    os << "Nombre de taches dans les files :\n";
#define IMPRIME_NOMBRE_DE_TACHES(VERBE, ACTION, CHAINE, INDEX)                                    \
    os << "-- " << CHAINE << " : " << taches[INDEX].taille() << '\n';

    ENUMERE_TACHES_POSSIBLES(IMPRIME_NOMBRE_DE_TACHES)

#undef IMPRIME_NOMBRE_DE_TACHES
}

Tacheronne::Tacheronne(Compilatrice &comp)
    : compilatrice(comp),
      assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", this->allocatrice_noeud)),
      id(compilatrice.ordonnanceuse->enregistre_tacheronne({}))
{
}

Tacheronne::~Tacheronne()
{
    memoire::deloge("AssembleuseArbre", assembleuse);
}

static Coulisse *selectionne_coulisse(Programme *programme)
{
    if (programme->pour_metaprogramme()) {
        return programme->coulisse();
    }

    return programme->espace()->coulisse;
}

static std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types_utilises, int drapeau)
{
    auto visites = kuri::ensemblon<Type *, 16>();
    auto pile = dls::pile<Type *>();

    pour_chaque_element(types_utilises, [&pile](auto &type) {
        pile.empile(type);
        return kuri::DecisionIteration::Continue;
    });

    while (!pile.est_vide()) {
        auto type_courant = pile.depile();

        /* Les types variadiques ou pointeur nul peuvent avoir des types déréférencés nuls. */
        if (!type_courant) {
            continue;
        }

        if (visites.possede(type_courant)) {
            continue;
        }

        visites.insere(type_courant);

        if ((type_courant->drapeaux & drapeau) == 0) {
            return Attente::sur_type(type_courant);
        }

        switch (type_courant->genre) {
            case GenreType::POLYMORPHIQUE:
            case GenreType::TUPLE:
            case GenreType::EINI:
            case GenreType::CHAINE:
            case GenreType::RIEN:
            case GenreType::BOOL:
            case GenreType::OCTET:
            case GenreType::TYPE_DE_DONNEES:
            case GenreType::REEL:
            case GenreType::ENTIER_CONSTANT:
            case GenreType::ENTIER_NATUREL:
            case GenreType::ENTIER_RELATIF:
            case GenreType::ENUM:
            case GenreType::ERREUR:
            {
                break;
            }
            case GenreType::FONCTION:
            {
                auto type_fonction = type_courant->comme_fonction();
                POUR (type_fonction->types_entrees) {
                    pile.empile(it);
                }
                pile.empile(type_fonction->type_sortie);
                break;
            }
            case GenreType::UNION:
            case GenreType::STRUCTURE:
            {
                auto type_compose = static_cast<TypeCompose *>(type_courant);
                POUR (type_compose->membres) {
                    pile.empile(it.type);
                }
                break;
            }
            case GenreType::REFERENCE:
            {
                pile.empile(type_courant->comme_reference()->type_pointe);
                break;
            }
            case GenreType::POINTEUR:
            {
                pile.empile(type_courant->comme_pointeur()->type_pointe);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                pile.empile(type_courant->comme_variadique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                pile.empile(type_courant->comme_tableau_dynamique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                pile.empile(type_courant->comme_tableau_fixe()->type_pointe);
                break;
            }
            case GenreType::OPAQUE:
            {
                pile.empile(type_courant->comme_opaque()->type_opacifie);
                break;
            }
        }
    }

    return {};
}

void Tacheronne::gere_tache()
{
    auto temps_debut = dls::chrono::compte_seconde();
    auto tache = Tache::dors(compilatrice.espace_de_travail_defaut);
    auto &ordonnanceuse = compilatrice.ordonnanceuse;

    while (true) {
        tache = ordonnanceuse->tache_suivante(tache, drapeaux);

        if (tache.genre != GenreTache::DORS) {
            nombre_dodos = 0;
        }

        switch (tache.genre) {
            case GenreTache::COMPILATION_TERMINEE:
            {
                temps_scene = temps_debut.temps() - temps_executable - temps_fichier_objet;
                return;
            }
            case GenreTache::DORS:
            {
                /* les tâches d'exécutions sont marquées comme terminées dès que leurs
                 * métaprogrammes sont ajoutés à la machine virtuelle, il se peut qu'il en reste à
                 * exécuter alors qu'il n'y a plus de tâches à exécuter */
                if (!mv.terminee()) {
                    nombre_dodos = 0;
                    execute_metaprogrammes();
                }
                else {
                    nombre_dodos += 1;
                    dls::chrono::dors_microsecondes(100 * nombre_dodos);
                    temps_passe_a_dormir += 0.1 * nombre_dodos;
                }

                break;
            }
            case GenreTache::CHARGEMENT:
            {
                auto fichier = tache.unite->fichier;
                auto donnees = fichier->donnees_constantes;

                if (!donnees->fut_charge) {
                    donnees->mutex.lock();

                    if (!donnees->fut_charge) {
                        auto debut_chargement = dls::chrono::compte_seconde();
                        auto texte = charge_contenu_fichier(dls::chaine(donnees->chemin));
                        temps_chargement += debut_chargement.temps();

                        auto debut_tampon = dls::chrono::compte_seconde();
                        donnees->charge_tampon(lng::tampon_source(std::move(texte)));
                        temps_tampons += debut_tampon.temps();
                    }

                    donnees->mutex.unlock();
                }

                if (donnees->fut_charge) {
                    compilatrice.gestionnaire_code->chargement_fichier_termine(tache.unite);
                }
                else {
                    compilatrice.gestionnaire_code->mets_en_attente(tache.unite,
                                                                    Attente::sur_lexage(fichier));
                }

                break;
            }
            case GenreTache::LEXAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_LEXER));
                auto unite = tache.unite;
                auto fichier = unite->fichier;
                auto donnees = fichier->donnees_constantes;

                if (!donnees->fut_lexe) {
                    donnees->mutex.lock();

                    if (!donnees->en_lexage) {
                        donnees->en_lexage = true;
                        auto debut_lexage = dls::chrono::compte_seconde();
                        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(), donnees);
                        lexeuse.performe_lexage();
                        temps_lexage += debut_lexage.temps();
                        donnees->en_lexage = false;
                    }

                    donnees->mutex.unlock();
                }

                if (donnees->fut_lexe) {
                    compilatrice.gestionnaire_code->lexage_fichier_termine(tache.unite);
                }
                else {
                    compilatrice.gestionnaire_code->mets_en_attente(
                        tache.unite, Attente::sur_chargement(fichier));
                }

                break;
            }
            case GenreTache::PARSAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_PARSER));
                auto unite = tache.unite;
                auto debut_parsage = dls::chrono::compte_seconde();
                auto syntaxeuse = Syntaxeuse(*this, unite);
                syntaxeuse.analyse();
                unite->fichier->fut_parse = true;
                compilatrice.gestionnaire_code->parsage_fichier_termine(tache.unite);
                temps_parsage += debut_parsage.temps();
                break;
            }
            case GenreTache::TYPAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_TYPER));
                auto unite = tache.unite;

                if (unite->est_bloquee()) {
                    mv.stop = true;
                    unite->rapporte_erreur();
                    break;
                }

                auto debut_validation = dls::chrono::compte_seconde();
                gere_unite_pour_typage(unite);
                temps_validation += debut_validation.temps();
                break;
            }
            case GenreTache::GENERATION_RI:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI));
                auto debut_generation = dls::chrono::compte_seconde();
                if (gere_unite_pour_ri(tache.unite)) {
                    compilatrice.gestionnaire_code->generation_ri_terminee(tache.unite);
                }
                constructrice_ri.temps_generation += debut_generation.temps();
                break;
            }
            case GenreTache::OPTIMISATION:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_OPTIMISER));
                auto debut_generation = dls::chrono::compte_seconde();
                temps_optimisation += debut_generation.temps();
                break;
            }
            case GenreTache::EXECUTION:
            {
                assert(dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER));
                gere_unite_pour_execution(tache.unite);
                break;
            }
            case GenreTache::GENERATION_CODE_MACHINE:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                auto programme = tache.unite->programme;
                auto coulisse = selectionne_coulisse(programme);
                if (coulisse->cree_fichier_objet(
                        compilatrice, *tache.unite->espace, programme, constructrice_ri)) {
                    compilatrice.gestionnaire_code->generation_code_machine_terminee(tache.unite);
                }
                temps_generation_code += tache.espace->coulisse->temps_generation_code;
                temps_fichier_objet += tache.espace->coulisse->temps_fichier_objet;
                break;
            }
            case GenreTache::LIAISON_PROGRAMME:
            {
                assert(
                    dls::outils::possede_drapeau(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                auto programme = tache.unite->programme;
                auto coulisse = selectionne_coulisse(programme);
                if (coulisse->cree_executable(compilatrice, *tache.espace, programme)) {
                    compilatrice.gestionnaire_code->liaison_programme_terminee(tache.unite);
                }
                temps_executable += tache.espace->coulisse->temps_executable;
                break;
            }
            case GenreTache::CONVERSION_NOEUD_CODE:
            {
                assert(dls::outils::possede_drapeau(
                    drapeaux, DrapeauxTacheronne::PEUT_CONVERTIR_NOEUD_CODE));
                auto espace = tache.unite->espace;
                auto noeud = tache.unite->noeud;

                auto types_utilises = kuri::ensemblon<Type *, 16>();
                visite_noeud(
                    noeud, PreferenceVisiteNoeud::ORIGINAL, [&](NoeudExpression const *racine) {
                        auto type = racine->type;
                        if (type) {
                            types_utilises.insere(type);
                        }

                        if (racine->est_entete_fonction()) {
                            auto entete = racine->comme_entete_fonction();

                            POUR ((*entete->bloc_constantes->membres.verrou_ecriture())) {
                                if (it->type) {
                                    types_utilises.insere(it->type);
                                }
                            }

                            return DecisionVisiteNoeud::IGNORE_ENFANTS;
                        }

                        return DecisionVisiteNoeud::CONTINUE;
                    });

                auto attente_possible = attente_sur_type_si_drapeau_manquant(types_utilises,
                                                                             TYPE_FUT_VALIDE);
                if (attente_possible) {
                    compilatrice.gestionnaire_code->mets_en_attente(tache.unite,
                                                                    attente_possible.value());
                    break;
                }

                // À FAIRE(noeuds codes) : ne convertis pas les corps s'ils n'ont pas encore été
                // validés
                // À FAIRE(noeuds codes) : ne convertis pas les déclarations référées
                // (créer un NoeudCode prématurément peut se faire)
                convertisseuse_noeud_code.convertis_noeud_syntaxique(espace, noeud);
                compilatrice.gestionnaire_code->conversion_noeud_code_terminee(tache.unite);
                break;
            }
            case GenreTache::ENVOIE_MESSAGE:
            {
                assert(dls::outils::possede_drapeau(drapeaux,
                                                    DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE));
                compilatrice.messagere->envoie_message(tache.unite->message);
                break;
            }
            case GenreTache::CREATION_FONCTION_INIT_TYPE:
            {
                assert(dls::outils::possede_drapeau(
                    drapeaux, DrapeauxTacheronne::PEUT_CREER_FONCTION_INIT_TYPE));

                auto unite = tache.unite;
                auto espace = unite->espace;
                auto type = unite->type;
                assert(type);

                cree_noeud_initialisation_type(espace, type, this->assembleuse);
                compilatrice.gestionnaire_code->fonction_initialisation_type_creee(unite);
                break;
            }
        }
    }

    temps_scene = temps_debut.temps() - temps_executable - temps_fichier_objet;
}

void Tacheronne::gere_unite_pour_typage(UniteCompilation *unite)
{
    auto contexte = ContexteValidationCode(compilatrice, *this, *unite);
    auto resultat = contexte.valide();
    if (est_erreur(resultat)) {
        return;
    }
    if (est_attente(resultat)) {
        compilatrice.gestionnaire_code->mets_en_attente(unite, std::get<Attente>(resultat));
        return;
    }
    /* Pour les imports et chargements. */
    temps_validation -= contexte.temps_chargement;
    compilatrice.gestionnaire_code->typage_termine(unite);
}

static NoeudDeclarationEnteteFonction *entete_fonction(NoeudExpression *noeud)
{
    if (noeud->est_entete_fonction()) {
        return noeud->comme_entete_fonction();
    }

    if (noeud->est_corps_fonction()) {
        return noeud->comme_corps_fonction()->entete;
    }

    return nullptr;
}

bool Tacheronne::gere_unite_pour_ri(UniteCompilation *unite)
{
    auto noeud = unite->noeud;

    if (noeud->type == nullptr) {
        unite->espace->rapporte_erreur(
            noeud, "Erreur interne: type nul sur une déclaration avant la génération de RI");
        return false;
    }

    auto entete_possible = entete_fonction(noeud);
    if (entete_possible && !entete_possible->est_initialisation_type) {
        auto types_utilises = kuri::ensemblon<Type *, 16>();
        visite_noeud(noeud, PreferenceVisiteNoeud::ORIGINAL, [&](NoeudExpression const *racine) {
            auto type = racine->type;
            if (type && !est_type_polymorphique(type)) {
                types_utilises.insere(type);
            }

            if (racine->est_entete_fonction()) {
                auto entete = racine->comme_entete_fonction();

                if (entete->bloc_constantes) {
                    POUR ((*entete->bloc_constantes->membres.verrou_ecriture())) {
                        if (it->type && !est_type_polymorphique(it->type)) {
                            types_utilises.insere(it->type);
                        }
                    }
                }

                return DecisionVisiteNoeud::IGNORE_ENFANTS;
            }

            if (noeud->est_declaration_variable()) {
                auto declaration = noeud->comme_declaration_variable();

                POUR (declaration->donnees_decl.plage()) {
                    for (auto &var : it.variables.plage()) {
                        if (!est_type_polymorphique(var->type)) {
                            types_utilises.insere(var->type);
                        }
                    }
                }
            }

            return DecisionVisiteNoeud::CONTINUE;
        });

        auto attente_possible = attente_sur_type_si_drapeau_manquant(
            types_utilises, INITIALISATION_TYPE_FUT_CREEE);
        if (attente_possible) {
            compilatrice.gestionnaire_code->mets_en_attente(unite, attente_possible.value());
            return false;
        }
    }

    if (unite->est_pour_generation_ri_principale_mp()) {
        constructrice_ri.genere_ri_pour_fonction_metaprogramme(unite->espace,
                                                               noeud->comme_entete_fonction());
    }
    else {
        constructrice_ri.genere_ri_pour_noeud(unite->espace, noeud);
    }

    noeud->drapeaux |= RI_FUT_GENEREE;
    return true;
}

void Tacheronne::gere_unite_pour_optimisation(UniteCompilation *unite)
{
    auto noeud = unite->noeud;
    auto entete = entete_fonction(noeud);
    assert(entete);

    if (entete->est_externe) {
        return;
    }

    /* n'optimise pas cette fonction car le manque de retour fait supprimer tout le code */
    if (entete == unite->espace->interface_kuri->decl_creation_contexte) {
        return;
    }

    optimise_code(constructrice_ri, static_cast<AtomeFonction *>(entete->atome));
}

void Tacheronne::gere_unite_pour_execution(UniteCompilation *unite)
{
    auto metaprogramme = unite->metaprogramme;
    assert(metaprogramme->fonction->drapeaux & RI_FUT_GENEREE);

    metaprogramme->donnees_execution = mv.loge_donnees_execution();
    mv.ajoute_metaprogramme(metaprogramme);

    execute_metaprogrammes();
}

void Tacheronne::execute_metaprogrammes()
{
    mv.execute_metaprogrammes_courants();

    POUR (mv.metaprogrammes_termines()) {
        auto espace = it->unite->espace;

        // À FAIRE : précision des messages d'erreurs
        if (it->resultat == MetaProgramme::ResultatExecution::ERREUR) {
            espace->rapporte_erreur(it->directive, "Erreur lors de l'exécution du métaprogramme");
        }
        else {
            if (it->directive && it->directive->ident == ID::assert_) {
                auto resultat = *reinterpret_cast<bool *>(it->donnees_execution->pointeur_pile);

                if (!resultat) {
                    espace->rapporte_erreur(it->directive, "Échec de l'assertion");
                }
            }
            else if (it->directive && it->directive->ident == ID::execute) {
                auto type = it->directive->type;
                auto pointeur = it->donnees_execution->pointeur_pile;

                // Les directives pour des expressions dans des fonctions n'ont pas d'unités
                if (!it->directive->unite) {
                    it->directive->substitution = noeud_syntaxique_depuis_resultat(
                        espace, it->directive, it->directive->lexeme, type, pointeur);
                }
            }
            else if (it->corps_texte) {
                auto resultat = *reinterpret_cast<kuri::chaine_statique *>(
                    it->donnees_execution->pointeur_pile);

                if (resultat.taille() == 0) {
                    espace->rapporte_erreur(it->corps_texte,
                                            "Le corps-texte a retourné une chaine vide");
                }

                auto tampon = dls::chaine(resultat.pointeur(), resultat.taille());

                if (*tampon.fin() != '\n') {
                    tampon.ajoute('\n');
                }

                /* Remplis les sources du fichier. */
                auto fichier = it->fichier;
                assert(it->fichier);

                auto donnees_fichier = fichier->donnees_constantes;
                donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

                compilatrice.chaines_ajoutees_a_la_compilation->ajoute(resultat);
                compilatrice.gestionnaire_code->requiers_lexage(espace, fichier);
            }
        }

        /* attend d'avoir générer le résultat des opérations avant de marquer comme exécuté */
        it->fut_execute = true;

        mv.deloge_donnees_execution(it->donnees_execution);

        compilatrice.gestionnaire_code->execution_terminee(it->unite);
    }
}

NoeudExpression *Tacheronne::noeud_syntaxique_depuis_resultat(EspaceDeTravail *espace,
                                                              NoeudDirectiveExecute *directive,
                                                              Lexeme const *lexeme,
                                                              Type *type,
                                                              octet_t *pointeur)
{
    switch (type->genre) {
        case GenreType::EINI:
        case GenreType::POINTEUR:
        case GenreType::POLYMORPHIQUE:
        case GenreType::REFERENCE:
        case GenreType::VARIADIQUE:
        {
            espace
                ->rapporte_erreur(directive,
                                  "Type non pris en charge pour le résutat des exécutions !")
                .ajoute_message("Le type est : ", chaine_type(type), "\n");
            break;
        }
        case GenreType::RIEN:
        {
            break;
        }
        case GenreType::TUPLE:
        {
            // pour les tuples de retours, nous les convertissons en expression-virgule
            auto tuple = type->comme_tuple();
            auto virgule = assembleuse->cree_virgule(lexeme);

            POUR (tuple->membres) {
                auto pointeur_membre = pointeur + it.decalage;
                virgule->expressions.ajoute(noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, it.type, pointeur_membre));
            }

            return virgule;
        }
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_RELATIF:
        {
            unsigned long valeur = 0;

            if (type->taille_octet == 1) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<char *>(pointeur));
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<short *>(pointeur));
            }
            else if (type->taille_octet == 4 || type->taille_octet == 0) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<int *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<long *>(pointeur));
            }

            return assembleuse->cree_litterale_entier(lexeme, type, valeur);
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        case GenreType::ENTIER_NATUREL:
        {
            unsigned long valeur = 0;
            if (type->taille_octet == 1) {
                valeur = static_cast<unsigned long>(*pointeur);
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<unsigned short *>(pointeur));
            }
            else if (type->taille_octet == 4) {
                valeur = static_cast<unsigned long>(*reinterpret_cast<unsigned int *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<unsigned long *>(pointeur);
            }

            return assembleuse->cree_litterale_entier(lexeme, type, valeur);
        }
        case GenreType::BOOL:
        {
            auto valeur = *reinterpret_cast<bool *>(pointeur);
            auto noeud_syntaxique = assembleuse->cree_litterale_bool(lexeme);
            noeud_syntaxique->valeur = valeur;
            noeud_syntaxique->type = type;
            return noeud_syntaxique;
        }
        case GenreType::REEL:
        {
            double valeur = 0.0;

            // À FAIRE(r16)
            if (type->taille_octet == 4) {
                valeur = static_cast<double>(*reinterpret_cast<float *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<double *>(pointeur);
            }

            return assembleuse->cree_litterale_reel(lexeme, type, valeur);
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = type->comme_structure();

            auto construction_structure = assembleuse->cree_construction_structure(lexeme,
                                                                                   type_structure);

            POUR (type_structure->membres) {
                if (it.drapeaux & TypeCompose::Membre::EST_CONSTANT) {
                    continue;
                }

                auto pointeur_membre = pointeur + it.decalage;
                auto noeud_membre = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, it.type, pointeur_membre);
                construction_structure->parametres_resolus.ajoute(noeud_membre);
            }

            return construction_structure;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();
            auto construction_union = assembleuse->cree_construction_structure(lexeme, type_union);

            if (type_union->est_nonsure) {
                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_union->type_le_plus_grand, pointeur);
                construction_union->parametres_resolus.ajoute(expr);
            }
            else {
                auto pointeur_donnees = pointeur;
                auto pointeur_discriminant = *reinterpret_cast<int *>(pointeur +
                                                                      type_union->decalage_index);
                auto index_membre = pointeur_discriminant - 1;

                auto type_donnees = type_union->membres[index_membre].type;

                for (auto i = 0; i < index_membre; ++i) {
                    construction_union->parametres_resolus.ajoute(nullptr);
                }

                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_donnees, pointeur_donnees);
                construction_union->parametres_resolus.ajoute(expr);
            }

            return construction_union;
        }
        case GenreType::CHAINE:
        {
            auto valeur_pointeur = pointeur;
            auto valeur_chaine = *reinterpret_cast<long *>(pointeur + 8);

            kuri::chaine_statique chaine = {*reinterpret_cast<char **>(valeur_pointeur),
                                            valeur_chaine};

            auto lit_chaine = assembleuse->cree_litterale_chaine(lexeme);
            lit_chaine->valeur = compilatrice.gerante_chaine->ajoute_chaine(chaine);
            lit_chaine->type = type;
            return lit_chaine;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            auto type_de_donnees = *reinterpret_cast<Type **>(pointeur);
            type_de_donnees = espace->typeuse.type_type_de_donnees(type_de_donnees);
            return assembleuse->cree_reference_type(lexeme, type_de_donnees);
        }
        case GenreType::FONCTION:
        {
            auto fonction = *reinterpret_cast<AtomeFonction **>(pointeur);

            if (!fonction->decl) {
                espace->rapporte_erreur(directive,
                                        "La fonction retournée n'a pas de déclaration !\n");
            }

            return assembleuse->cree_reference_declaration(
                lexeme, const_cast<NoeudDeclarationEnteteFonction *>(fonction->decl));
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto expr = noeud_syntaxique_depuis_resultat(
                espace, directive, lexeme, type_opaque->type_opacifie, pointeur);

            /* comme dans la simplification de l'arbre, ceci doit être un transtypage vers le type
             * opaque */
            auto comme = assembleuse->cree_comme(lexeme);
            comme->type = type_opaque;
            comme->expression = expr;
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_opaque};
            comme->drapeaux |= TRANSTYPAGE_IMPLICITE;
            return comme;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();

            auto virgule = assembleuse->cree_virgule(lexeme);
            virgule->expressions.reserve(type_tableau->taille);

            for (auto i = 0; i < type_tableau->taille; ++i) {
                auto pointeur_valeur = pointeur + type_tableau->type_pointe->taille_octet *
                                                      static_cast<unsigned>(i);
                auto expr = noeud_syntaxique_depuis_resultat(
                    espace, directive, lexeme, type_tableau->type_pointe, pointeur_valeur);
                virgule->expressions.ajoute(expr);
            }

            auto construction = assembleuse->cree_construction_tableau(lexeme);
            construction->type = type_tableau;
            construction->expression = virgule;
            return construction;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_tableau_dynamique();

            auto pointeur_donnees = *reinterpret_cast<octet_t **>(pointeur);
            auto taille_donnees = *reinterpret_cast<long *>(pointeur + 8);

            if (taille_donnees == 0) {
                espace->rapporte_erreur(directive, "Retour d'un tableau dynamique de taille 0 !");
            }

            /* crée un tableau fixe */
            auto type_tableau_fixe = espace->typeuse.type_tableau_fixe(
                type_tableau->type_pointe, static_cast<int>(taille_donnees));
            auto construction = noeud_syntaxique_depuis_resultat(
                espace, directive, lexeme, type_tableau_fixe, pointeur_donnees);

            /* convertis vers un tableau dynamique */
            auto comme = assembleuse->cree_comme(lexeme);
            comme->type = type_tableau;
            comme->expression = construction;
            comme->transformation = {TypeTransformation::CONVERTI_TABLEAU, type_tableau};
            comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

            return comme;
        }
    }

    return nullptr;
}

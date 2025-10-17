/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "code_binaire.hh"

#include <iomanip>

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/bibliotheque.hh"
#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/ipa.hh"
#include "compilation/metaprogramme.hh"
#include "compilation/programme.hh"

#include "structures/enchaineuse.hh"

#include "utilitaires/log.hh"
#include "utilitaires/logeuse_memoire.hh"
#include "utilitaires/macros.hh"

#include "impression.hh"
#include "instructions.hh"

kuri::chaine_statique chaine_code_operation(octet_t code_operation)
{
    switch (code_operation) {
#define ENUMERE_CODE_OPERATION_EX(code)                                                           \
    case code:                                                                                    \
        return #code;
        ENUMERE_CODES_OPERATION
#undef ENUMERE_CODE_OPERATION_EX
    }

    return "ERREUR";
}

/* ************************************************************************** */

Chunk::~Chunk()
{
    détruit();
}

NoeudExpression const *Chunk::donne_site_pour_adresse(octet_t *adresse) const
{
    assert_rappel(adresse >= code && adresse < (code + compte), [&]() {
        dbg() << "code " << static_cast<void *>(code) << ", " << "adresse "
              << static_cast<void *>(adresse) << ", " << "code + compte "
              << static_cast<void *>(code + compte);
    });

    if (m_sites_source.est_vide()) {
        return nullptr;
    }

    auto décalage = static_cast<int>(adresse - code);
    if (décalage < 0 || décalage >= compte) {
        return nullptr;
    }

    for (int i = 0; i < m_sites_source.taille() - 1; i++) {
        if (décalage >= m_sites_source[i].décalage && décalage < m_sites_source[i + 1].décalage) {
            return m_sites_source[i].site;
        }
    }

    return m_sites_source.dernier_élément().site;
}

void Chunk::ajoute_site_source(NoeudExpression const *site)
{
    if (!site) {
        return;
    }

    if (m_sites_source.est_vide()) {
        m_sites_source.ajoute({static_cast<int>(compte), site});
        return;
    }

    if (m_sites_source.dernier_élément().site == site) {
        return;
    }

    m_sites_source.ajoute({static_cast<int>(compte), site});
}

void Chunk::initialise()
{
    code = nullptr;
    compte = 0;
    capacité = 0;
    m_sites_source.efface();
}

void Chunk::détruit()
{
    mémoire::deloge_tableau("Chunk::code", code, capacité);
    initialise();
}

void Chunk::rétrécis_capacité_sur_taille()
{
    if (compte == capacité) {
        return;
    }

    mémoire::reloge_tableau("Chunk::code", code, capacité, compte);
    capacité = compte;
}

int64_t Chunk::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += capacité;
    résultat += locales.taille_mémoire();
    résultat += m_sites_source.taille_mémoire();
    return résultat;
}

void Chunk::émets(octet_t o)
{
    agrandis_si_nécessaire(1);
    code[compte] = o;
    compte += 1;
}

void Chunk::agrandis_si_nécessaire(int64_t taille)
{
    auto taille_requise = compte + taille;
    if (capacité < taille_requise) {
        auto nouvelle_capacité = taille_requise < 8 ? 8 : taille_requise;
        if (nouvelle_capacité < capacité * 2) {
            nouvelle_capacité = capacité * 2;
        }
        mémoire::reloge_tableau("Chunk::code", code, capacité, nouvelle_capacité);
        capacité = nouvelle_capacité;
    }
}

void Chunk::émets_entête_op(octet_t op, const NoeudExpression *site)
{
    ajoute_site_source(site);

    if (émets_stats_ops) {
        émets(OP_STAT_INSTRUCTION);
    }

    émets(op);
}

void Chunk::émets_logue_instruction(int32_t décalage)
{
    émets_entête_op(OP_LOGUE_INSTRUCTION, nullptr);
    émets(décalage);
}

void Chunk::émets_logue_appel(AtomeFonction const *atome)
{
    émets_entête_op(OP_LOGUE_APPEL, nullptr);
    émets(atome);
}

void Chunk::émets_logue_entrées(AtomeFonction const *atome, unsigned taille_arguments)
{
    émets_entête_op(OP_LOGUE_ENTRÉES, nullptr);
    émets(atome);
    émets(taille_arguments);
}

void Chunk::émets_logue_sorties()
{
    émets_entête_op(OP_LOGUE_SORTIES, nullptr);
}

void Chunk::émets_logue_retour()
{
    émets_entête_op(OP_LOGUE_RETOUR, nullptr);
}

void Chunk::émets_notifie_empilage(NoeudExpression const *site, uint32_t taille)
{
    if (!émets_notifications_empilage) {
        return;
    }
    émets_entête_op(OP_NOTIFIE_EMPILAGE_VALEUR, site);
    émets(taille);
}

void Chunk::émets_notifie_dépilage(NoeudExpression const *site, uint32_t taille)
{
    if (!émets_notifications_empilage) {
        return;
    }
    émets_entête_op(OP_NOTIFIE_DÉPILAGE_VALEUR, site);
    émets(taille);
}

void Chunk::émets_dépilage_paramètres_appel(NoeudExpression const *site,
                                            InstructionAppel const *inst)
{
    if (!émets_notifications_empilage) {
        return;
    }
    auto nombre_d_arguments = inst->args.taille();
    for (auto i = nombre_d_arguments - 1; i >= 0; --i) {
        auto type = donne_type_primitif(inst->args[i]->type);
        émets_notifie_dépilage(site, type->taille_octet);
    }
}

void Chunk::émets_empilage_retour_appel(NoeudExpression const *site, InstructionAppel const *inst)
{
    if (!émets_notifications_empilage) {
        return;
    }
    auto type_retourné = inst->type;
    if (!type_retourné->est_type_rien()) {
        émets_notifie_empilage(site, type_retourné->taille_octet);
    }
}

void Chunk::émets_profile_débute_appel()
{
    if (!émets_profilage) {
        return;
    }
    émets_entête_op(OP_PROFILE_DÉBUTE_APPEL, nullptr);
}

void Chunk::émets_profile_termine_appel()
{
    if (!émets_profilage) {
        return;
    }
    émets_entête_op(OP_PROFILE_TERMINE_APPEL, nullptr);
}

int Chunk::ajoute_locale(InstructionAllocation const *alloc)
{
    auto type_alloué = alloc->donne_type_alloué();
    type_alloué = donne_type_primitif(type_alloué);
    auto taille_octet = type_alloué->taille_octet;
    assert(taille_octet);

    auto index = locales.taille();
    locales.ajoute({alloc->ident, type_alloué, taille_allouée});
    taille_allouée += static_cast<int>(taille_octet);
    return index;
}

int Chunk::émets_structure_constante(uint32_t taille_structure)
{
    émets_entête_op(OP_STRUCTURE_CONSTANTE, nullptr);
    émets(taille_structure);

    auto décalage = compte;
    agrandis_si_nécessaire(taille_structure);
    compte += taille_structure;
    émets_notifie_empilage(nullptr, taille_structure);
    return int32_t(décalage);
}

void Chunk::émets_retour(NoeudExpression const *site, Atome const *valeur)
{
    if (valeur) {
        /* Puisque nous avons des fonctions externes (incluant les intrinsèques et l'IPA de la
         * compilatrice) qui n'ont pas de retour via une instruction du code binaire, nous
         * « dépilons » la valeur de retour afin qu'elle ne pollue pas la pile; la notification de
         * l'empilage d'une se fera via l'instruction d'appel. */
        émets_notifie_dépilage(site, valeur->type->taille_octet);
    }
    émets_entête_op(OP_RETOURNE, site);
}

void Chunk::émets_assignation(ContexteGénérationCodeBinaire contexte,
                              NoeudExpression const *site,
                              Type const *type)
{
#if 0  // ndef CMAKE_BUILD_TYPE_PROFILE
    assert_rappel(type->taille_octet, [&]() {
        dbg() << "Le type est " << chaine_type(type) << '\n';

        auto fonction = contexte.fonction;
        if (fonction) {
            dbg() << "La fonction est " << nom_humainement_lisible(fonction) << '\n';
            dbg() << *fonction << '\n';
        }

        erreur::imprime_site(*contexte.espace, site);
    });
#endif

    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_ADRESSAGE_ASSIGNE, site);
        émets(type->taille_octet);
    }

    émets_notifie_dépilage(site, 8); /* adresse */
    émets_notifie_dépilage(site, type->taille_octet);
    émets_entête_op(OP_ASSIGNE, site);
    émets(type->taille_octet);
}

void Chunk::émets_assignation_locale(NoeudExpression const *site, int pointeur, Type const *type)
{
#if 0  // ndef CMAKE_BUILD_TYPE_PROFILE
    assert_rappel(type->taille_octet, [&]() {
        dbg() << "Le type est " << chaine_type(type) << '\n';

        auto fonction = contexte.fonction;
        if (fonction) {
            dbg() << "La fonction est " << nom_humainement_lisible(fonction) << '\n';
            dbg() << *fonction << '\n';
        }

        erreur::imprime_site(*contexte.espace, site);
    });
#endif

    émets_notifie_dépilage(site, type->taille_octet);
    émets_entête_op(OP_ASSIGNE_LOCALE, site);
    émets(pointeur);
    émets(type->taille_octet);
}

void Chunk::émets_init_locale_zéro(const NoeudExpression *site, int pointeur, const Type *type)
{
    émets_entête_op(OP_INIT_LOCALE_ZÉRO, site);
    émets(pointeur);
    émets(type->taille_octet);
}

void Chunk::émets_copie_locale(NoeudExpression const *site,
                               Type const *type,
                               int pointeur_source,
                               int pointeur_destination)
{
    émets_entête_op(OP_COPIE_LOCALE, site);
    émets(type->taille_octet);
    émets(pointeur_source);
    émets(pointeur_destination);
}

void Chunk::émets_charge(NoeudExpression const *site, Type const *type)
{
    assert(type->taille_octet);

    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_ADRESSAGE_CHARGE, site);
        émets(type->taille_octet);
    }

    émets_notifie_dépilage(site, 8); /* adresse */
    émets_entête_op(OP_CHARGE, site);
    émets(type->taille_octet);
    émets_notifie_empilage(site, type->taille_octet);
}

void Chunk::émets_charge_locale(NoeudExpression const *site, int pointeur, Type const *type)
{
    auto type_primitif = donne_type_primitif(type);
    auto taille_octet = type_primitif->taille_octet;
    émets_entête_op(OP_CHARGE_LOCALE, site);
    émets(pointeur);
    émets(taille_octet);
    émets_notifie_empilage(site, taille_octet);
}

void Chunk::émets_référence_globale(NoeudExpression const *site, int pointeur)
{
    émets_entête_op(OP_REFERENCE_GLOBALE, site);
    émets(pointeur);
    émets_notifie_empilage(site, 8); /* adresse */
}

void Chunk::émets_référence_globale_externe(const NoeudExpression *site, const void *adresse)
{
    émets_entête_op(OP_REFERENCE_GLOBALE_EXTERNE, site);
    émets(adresse);
    émets_notifie_empilage(site, 8); /* adresse */
}

void Chunk::émets_référence_locale(NoeudExpression const *site, int pointeur)
{
    émets_entête_op(OP_RÉFÉRENCE_LOCALE, site);
    émets(pointeur);
    émets_notifie_empilage(site, 8); /* adresse */
}

void Chunk::émets_référence_rubrique(NoeudExpression const *site, unsigned décalage)
{
    émets_notifie_dépilage(site, 8); /* adresse */
    émets_entête_op(OP_REFERENCE_RUBRIQUE, site);
    émets(décalage);
    émets_notifie_empilage(site, 8); /* adresse rubrique */
}

void Chunk::émets_référence_rubrique_locale(const NoeudExpression *site,
                                            int pointeur,
                                            uint32_t décalage)
{
    émets_entête_op(OP_RÉFÉRENCE_RUBRIQUE_LOCALE, site);
    émets(pointeur);
    émets(décalage);
    émets_notifie_empilage(site, 8); /* adresse rubrique */
}

void Chunk::émets_appel(NoeudExpression const *site,
                        AtomeFonction const *fonction,
                        InstructionAppel const *inst_appel,
                        unsigned taille_arguments)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_dépilage_paramètres_appel(site, inst_appel);
    émets_profile_débute_appel();

    émets_entête_op(OP_APPEL, site);
    émets(fonction);
    émets(taille_arguments);

    émets_profile_termine_appel();
    émets_empilage_retour_appel(site, inst_appel);
}

void Chunk::émets_appel_externe(NoeudExpression const *site,
                                AtomeFonction const *fonction,
                                unsigned taille_arguments,
                                InstructionAppel const *inst_appel)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_dépilage_paramètres_appel(site, inst_appel);
    émets_profile_débute_appel();

    émets_entête_op(OP_APPEL_EXTERNE, site);
    émets(fonction);
    émets(taille_arguments);
    émets(inst_appel);

    émets_profile_termine_appel();
    émets_empilage_retour_appel(site, inst_appel);
}

void Chunk::émets_appel_compilatrice(const NoeudExpression *site,
                                     const AtomeFonction *fonction,
                                     InstructionAppel const *inst_appel)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(false); /* est pointeur */
        émets(fonction);
    }

    émets_dépilage_paramètres_appel(site, inst_appel);
    émets_profile_débute_appel();

    émets_entête_op(OP_APPEL_COMPILATRICE, site);
    émets(fonction);

    émets_profile_termine_appel();
    émets_empilage_retour_appel(site, inst_appel);
}

void Chunk::émets_appel_intrinsèque(NoeudExpression const *site,
                                    AtomeFonction const *fonction,
                                    InstructionAppel const *inst_appel)
{
    émets_dépilage_paramètres_appel(site, inst_appel);
    émets_profile_débute_appel();

    émets_entête_op(OP_APPEL_INTRINSÈQUE, site);
    émets(fonction);

    émets_profile_termine_appel();
    émets_empilage_retour_appel(site, inst_appel);
}

void Chunk::émets_appel_pointeur(NoeudExpression const *site,
                                 unsigned taille_arguments,
                                 InstructionAppel const *inst_appel)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_APPEL, site);
        émets(true); /* est pointeur */
    }

    émets_notifie_dépilage(site, 8); /* adresse. */
    émets_dépilage_paramètres_appel(site, inst_appel);
    émets_profile_débute_appel();

    émets_entête_op(OP_APPEL_POINTEUR, site);
    émets(taille_arguments);
    émets(inst_appel);

    émets_profile_termine_appel();
    émets_empilage_retour_appel(site, inst_appel);
}

void Chunk::émets_accès_index(NoeudExpression const *site, Type const *type)
{
    assert(type->taille_octet);
    émets_notifie_dépilage(site, 8); /* adresse */
    émets_notifie_dépilage(site, 8); /* index */
    émets_entête_op(OP_ACCÈDE_INDICE, site);
    émets(type->taille_octet);
    émets_notifie_empilage(site, 8); /* nouvelle_adresse */
}

void Chunk::émets_branche(NoeudExpression const *site,
                          kuri::tableau<PatchLabel> &patchs_labels,
                          int index)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_BRANCHE, nullptr);
        émets(0);
        patchs_labels.ajoute({index, static_cast<int>(compte - 4)});
    }

    émets_entête_op(OP_BRANCHE, site);
    émets(0);
    patchs_labels.ajoute({index, static_cast<int>(compte - 4)});
}

void Chunk::émets_branche_condition(NoeudExpression const *site,
                                    kuri::tableau<PatchLabel> &patchs_labels,
                                    int indice_label_si_vrai,
                                    int indice_label_si_faux)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION, nullptr);
        émets(0);
        patchs_labels.ajoute({indice_label_si_vrai, static_cast<int>(compte - 4)});
        émets(0);
        patchs_labels.ajoute({indice_label_si_faux, static_cast<int>(compte - 4)});
    }

    émets_notifie_dépilage(site, 1);
    émets_entête_op(OP_BRANCHE_CONDITION, site);
    émets(0);
    patchs_labels.ajoute({indice_label_si_vrai, static_cast<int>(compte - 4)});
    émets(0);
    patchs_labels.ajoute({indice_label_si_faux, static_cast<int>(compte - 4)});
}

void Chunk::émets_branche_si_zéro(NoeudExpression const *site,
                                  kuri::tableau<PatchLabel> &patchs_labels,
                                  uint32_t taille_opérande,
                                  int indice_label_si_vrai,
                                  int indice_label_si_faux)
{
    if (émets_vérification_branches) {
        émets_entête_op(OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION, nullptr);
        émets(0);
        patchs_labels.ajoute({indice_label_si_vrai, static_cast<int>(compte - 4)});
        émets(0);
        patchs_labels.ajoute({indice_label_si_faux, static_cast<int>(compte - 4)});
    }

    émets_notifie_dépilage(site, taille_opérande);
    émets_entête_op(OP_BRANCHE_SI_ZÉRO, site);
    émets(taille_opérande);
    émets(0);
    patchs_labels.ajoute({indice_label_si_vrai, static_cast<int>(compte - 4)});
    émets(0);
    patchs_labels.ajoute({indice_label_si_faux, static_cast<int>(compte - 4)});
}

void Chunk::émets_operation_unaire(NoeudExpression const *site,
                                   OpérateurUnaire::Genre op,
                                   Type const *type)
{
    auto type_primitif = donne_type_primitif(type);
    auto taille_type = type_primitif->taille_octet;
    émets_notifie_dépilage(site, taille_type);

    if (op == OpérateurUnaire::Genre::Négation) {
        if (type->est_type_réel()) {
            émets_entête_op(OP_COMPLEMENT_REEL, site);
        }
        else {
            émets_entête_op(OP_COMPLEMENT_ENTIER, site);
        }
    }
    else if (op == OpérateurUnaire::Genre::Négation_Binaire) {
        émets_entête_op(OP_NON_BINAIRE, site);
    }

    émets(taille_type);
    émets_notifie_empilage(site, taille_type);
}

static octet_t converti_op_binaire(OpérateurBinaire::Genre genre)
{
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code)                                            \
    case OpérateurBinaire::Genre::genre:                                                          \
        return op_code;
    switch (genre) {
        ENUMERE_OPERATEURS_BINAIRE
    }
#undef ENUMERE_GENRE_OPBINAIRE_EX
    return static_cast<octet_t>(-1);
}

static std::optional<octet_t> converti_type_transtypage(TypeTranstypage genre)
{
    switch (genre) {
        case TypeTranstypage::BITS:
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
        {
            break;
        }
        case TypeTranstypage::REEL_VERS_ENTIER_RELATIF:
        {
            return OP_REEL_VERS_RELATIF;
        }
        case TypeTranstypage::REEL_VERS_ENTIER_NATUREL:
        {
            return OP_REEL_VERS_NATUREL;
        }
        case TypeTranstypage::ENTIER_RELATIF_VERS_REEL:
        {
            return OP_RELATIF_VERS_REEL;
        }
        case TypeTranstypage::ENTIER_NATUREL_VERS_REEL:
        {
            return OP_NATUREL_VERS_REEL;
        }
        case TypeTranstypage::AUGMENTE_REEL:
        {
            return OP_AUGMENTE_REEL;
        }
        case TypeTranstypage::AUGMENTE_NATUREL:
        case TypeTranstypage::AUGMENTE_NATUREL_VERS_RELATIF:
        {
            return OP_AUGMENTE_NATUREL;
        }
        case TypeTranstypage::AUGMENTE_RELATIF:
        case TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL:
        {
            return OP_AUGMENTE_RELATIF;
        }
        case TypeTranstypage::DIMINUE_REEL:
        {
            return OP_DIMINUE_REEL;
        }
        case TypeTranstypage::DIMINUE_NATUREL:
        case TypeTranstypage::DIMINUE_NATUREL_VERS_RELATIF:
        {
            return OP_DIMINUE_NATUREL;
        }
        case TypeTranstypage::DIMINUE_RELATIF:
        case TypeTranstypage::DIMINUE_RELATIF_VERS_NATUREL:
        {
            return OP_DIMINUE_RELATIF;
        }
    }

    return {};
}

void Chunk::émets_operation_binaire(NoeudExpression const *site,
                                    OpérateurBinaire::Genre op,
                                    Type const *type_résultat,
                                    Type const *type_gauche,
                                    Type const *type_droite)
{
    auto type_primitif_gauche = donne_type_primitif(type_gauche);
    auto type_primitif_droite = donne_type_primitif(type_droite);
    auto taille_octet = std::max(type_primitif_gauche->taille_octet,
                                 type_primitif_droite->taille_octet);
    assert(taille_octet != 0);

    émets_notifie_dépilage(site, taille_octet);
    émets_notifie_dépilage(site, taille_octet);

    auto op_comp = converti_op_binaire(op);
    émets_entête_op(op_comp, site);
    émets(taille_octet);

    émets_notifie_empilage(site, type_résultat->taille_octet);
}

void Chunk::émets_incrémente(const NoeudExpression *site, const Type *type)
{
    auto type_primitif = donne_type_primitif(type);
    émets_entête_op(OP_INCRÉMENTE, site);
    émets(type_primitif->taille_octet);
}

void Chunk::émets_incrémente_locale(const NoeudExpression *site, const Type *type, int pointeur)
{
    auto type_primitif = donne_type_primitif(type);
    émets_entête_op(OP_INCRÉMENTE_LOCALE, site);
    émets(type_primitif->taille_octet);
    émets(pointeur);
}

void Chunk::émets_décrémente(const NoeudExpression *site, const Type *type)
{
    auto type_primitif = donne_type_primitif(type);
    émets_entête_op(OP_DÉCRÉMENTE, site);
    émets(type_primitif->taille_octet);
}

void Chunk::émets_transtype(const NoeudExpression *site,
                            uint8_t op,
                            uint32_t taille_source,
                            uint32_t taille_dest)
{
    émets_notifie_dépilage(site, taille_source);
    émets_entête_op(op, site);
    émets(taille_source);
    émets(taille_dest);
    émets_notifie_empilage(site, taille_dest);
}

void Chunk::émets_inatteignable(const NoeudExpression *site)
{
    émets_entête_op(OP_INATTEIGNABLE, site);
}

void Chunk::émets_sélection(const NoeudExpression *site, const Type *type)
{
    émets_notifie_dépilage(site, 1);
    émets_notifie_dépilage(site, type->taille_octet);
    émets_entête_op(OP_SÉLECTION, site);
    émets(type->taille_octet);
}

void Chunk::émets_rembourrage(uint32_t rembourrage)
{
    émets_entête_op(OP_REMBOURRAGE, nullptr);
    émets(rembourrage);
}

/* ************************************************************************** */

template <typename T>
struct DésassembleuseValeur {
    static T donne_valeur(Chunk const &chunk, int64_t &décalage)
    {
        auto résultat = *reinterpret_cast<T *>(&chunk.code[décalage]);
        décalage += int64_t(sizeof(T));
        return résultat;
    }
};

template <>
struct DésassembleuseValeur<AtomeFonction *> {
    static kuri::chaine_statique donne_valeur(Chunk const &chunk, int64_t &décalage)
    {
        auto fonction = *reinterpret_cast<AtomeFonction **>(&chunk.code[décalage]);
        décalage += int64_t(sizeof(AtomeFonction *));
        return fonction->nom;
    }
};

static int64_t instruction_simple(int64_t décalage, Enchaineuse &os)
{
    os << '\n';
    return décalage + 1;
}

template <typename T>
static int64_t instruction_1d(Chunk const &chunk, int64_t décalage, Enchaineuse &os)
{
    décalage += 1;
    os << ' ' << DésassembleuseValeur<T>::donne_valeur(chunk, décalage) << '\n';
    return décalage;
}

template <typename T1, typename T2>
static int64_t instruction_2d(Chunk const &chunk, int64_t décalage, Enchaineuse &os)
{
    décalage += 1;
    os << ' ' << DésassembleuseValeur<T1>::donne_valeur(chunk, décalage) << ", ";
    os << DésassembleuseValeur<T2>::donne_valeur(chunk, décalage) << '\n';
    return décalage;
}

template <typename T1, typename T2, typename T3>
static int64_t instruction_3d(Chunk const &chunk, int64_t décalage, Enchaineuse &os)
{
    décalage += 1;
    os << ' ' << DésassembleuseValeur<T1>::donne_valeur(chunk, décalage) << ", ";
    os << DésassembleuseValeur<T2>::donne_valeur(chunk, décalage) << ", ";
    os << DésassembleuseValeur<T3>::donne_valeur(chunk, décalage) << '\n';
    return décalage;
}

int64_t désassemble_instruction(Chunk const &chunk, int64_t décalage, Enchaineuse &os)
{
    os << std::setfill('0') << std::setw(4) << décalage << ' ';

    auto instruction = chunk.code[décalage];

    os << chaine_code_operation(instruction);

    switch (instruction) {
        case OP_LOGUE_RETOUR:
        case OP_LOGUE_SORTIES:
        case OP_RETOURNE:
        case OP_PROFILE_DÉBUTE_APPEL:
        case OP_PROFILE_TERMINE_APPEL:
        case OP_INATTEIGNABLE:
        {
            return instruction_simple(décalage, os);
        }
        case OP_CONSTANTE:
        {
            décalage += 1;
            auto drapeaux = chunk.code[décalage];
            décalage += 1;
            os << ' ';

#define LIS_CONSTANTE(type)                                                                       \
    type v = *(reinterpret_cast<type *>(&chunk.code[décalage]));                                  \
    os << v;                                                                                      \
    décalage += (drapeaux >> 3);

            switch (drapeaux) {
                case CONSTANTE_ENTIER_RELATIF | BITS_8:
                {
                    LIS_CONSTANTE(char);
                    os << " z8";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_16:
                {
                    LIS_CONSTANTE(short);
                    os << " z16";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_32:
                {
                    LIS_CONSTANTE(int);
                    os << " z32";
                    break;
                }
                case CONSTANTE_ENTIER_RELATIF | BITS_64:
                {
                    LIS_CONSTANTE(int64_t);
                    os << " z64";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_8:
                {
                    // erreur de compilation pour transtype inutile avec drapeaux stricts
                    os << static_cast<int64_t>(chunk.code[décalage]);
                    décalage += 1;
                    os << " n8";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_16:
                {
                    LIS_CONSTANTE(unsigned short);
                    os << " n16";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_32:
                {
                    LIS_CONSTANTE(uint32_t);
                    os << " n32";
                    break;
                }
                case CONSTANTE_ENTIER_NATUREL | BITS_64:
                {
                    LIS_CONSTANTE(uint64_t);
                    os << " n64";
                    break;
                }
                case CONSTANTE_NOMBRE_REEL | BITS_32:
                {
                    LIS_CONSTANTE(float);
                    os << " r32";
                    break;
                }
                case CONSTANTE_NOMBRE_REEL | BITS_64:
                {
                    LIS_CONSTANTE(double);
                    os << " r64";
                    break;
                }
            }

#undef LIS_CONSTANTE
            os << '\n';
            return décalage;
        }
        case OP_STRUCTURE_CONSTANTE:
        {
            os << "\n";
            décalage += 1;
            auto taille_structure = *reinterpret_cast<int *>(&chunk.code[décalage]);
            return décalage + taille_structure + 4;
        }
        case OP_AJOUTE:
        case OP_AJOUTE_REEL:
        case OP_SOUSTRAIT:
        case OP_SOUSTRAIT_REEL:
        case OP_MULTIPLIE:
        case OP_MULTIPLIE_REEL:
        case OP_DIVISE:
        case OP_DIVISE_RELATIF:
        case OP_DIVISE_REEL:
        case OP_RESTE_NATUREL:
        case OP_RESTE_RELATIF:
        case OP_COMP_EGAL:
        case OP_COMP_INEGAL:
        case OP_COMP_INF:
        case OP_COMP_INF_EGAL:
        case OP_COMP_SUP:
        case OP_COMP_SUP_EGAL:
        case OP_COMP_INF_NATUREL:
        case OP_COMP_INF_EGAL_NATUREL:
        case OP_COMP_SUP_NATUREL:
        case OP_COMP_SUP_EGAL_NATUREL:
        case OP_COMP_EGAL_REEL:
        case OP_COMP_INEGAL_REEL:
        case OP_COMP_INF_REEL:
        case OP_COMP_INF_EGAL_REEL:
        case OP_COMP_SUP_REEL:
        case OP_COMP_SUP_EGAL_REEL:
        case OP_ET_BINAIRE:
        case OP_OU_BINAIRE:
        case OP_OU_EXCLUSIF:
        case OP_DEC_GAUCHE:
        case OP_DEC_DROITE_ARITHM:
        case OP_DEC_DROITE_LOGIQUE:
        case OP_BRANCHE:
        case OP_ASSIGNE:
        case OP_CHARGE:
        case OP_REFERENCE_GLOBALE:
        case OP_RÉFÉRENCE_LOCALE:
        case OP_REFERENCE_RUBRIQUE:
        case OP_ACCÈDE_INDICE:
        case OP_COMPLEMENT_REEL:
        case OP_COMPLEMENT_ENTIER:
        case OP_NON_BINAIRE:
        case OP_VÉRIFIE_ADRESSAGE_ASSIGNE:
        case OP_VÉRIFIE_ADRESSAGE_CHARGE:
        case OP_LOGUE_INSTRUCTION:
        case OP_INCRÉMENTE:
        case OP_DÉCRÉMENTE:
        case OP_REMBOURRAGE:
        case OP_NOTIFIE_DÉPILAGE_VALEUR:
        case OP_NOTIFIE_EMPILAGE_VALEUR:
        case OP_SÉLECTION:
        case OP_VÉRIFIE_CIBLE_BRANCHE:
        {
            return instruction_1d<int>(chunk, décalage, os);
        }
        case OP_VÉRIFIE_CIBLE_BRANCHE_CONDITION:
        {
            return instruction_2d<int, int>(chunk, décalage, os);
        }
        case OP_APPEL_POINTEUR:
        {
            return instruction_2d<int, void *>(chunk, décalage, os);
        }
        case OP_STAT_INSTRUCTION:
        {
            décalage += 1;
            auto op = chunk.code[décalage];
            os << ' ' << chaine_code_operation(op) << '\n';
            return décalage + 1;
        }
        case OP_ASSIGNE_LOCALE:
        case OP_CHARGE_LOCALE:
        case OP_BRANCHE_CONDITION:
        case OP_INCRÉMENTE_LOCALE:
        case OP_RÉFÉRENCE_RUBRIQUE_LOCALE:
        case OP_INIT_LOCALE_ZÉRO:
        {
            return instruction_2d<int, int>(chunk, décalage, os);
        }
        case OP_BRANCHE_SI_ZÉRO:
        {
            return instruction_3d<int, int, int>(chunk, décalage, os);
        }
        case OP_VÉRIFIE_CIBLE_APPEL:
        {
            auto décalage_est_pointeur = décalage + 1;
            auto est_pointeur = DésassembleuseValeur<bool>::donne_valeur(chunk,
                                                                         décalage_est_pointeur);
            if (est_pointeur) {
                return instruction_1d<bool>(chunk, décalage, os);
            }

            return instruction_2d<bool, int64_t>(chunk, décalage, os);
        }
        case OP_APPEL:
        {
            return instruction_2d<AtomeFonction *, int>(chunk, décalage, os);
        }
        case OP_APPEL_EXTERNE:
        {
            return instruction_3d<AtomeFonction *, int, void *>(chunk, décalage, os);
        }
        case OP_APPEL_COMPILATRICE:
        case OP_APPEL_INTRINSÈQUE:
        {
            return instruction_1d<AtomeFonction *>(chunk, décalage, os);
        }
        case OP_COPIE_LOCALE:
        {
            return instruction_3d<int, int, int>(chunk, décalage, os);
        }
        case OP_AUGMENTE_NATUREL:
        case OP_DIMINUE_NATUREL:
        case OP_AUGMENTE_RELATIF:
        case OP_DIMINUE_RELATIF:
        case OP_AUGMENTE_REEL:
        case OP_DIMINUE_REEL:
        case OP_NATUREL_VERS_REEL:
        case OP_RELATIF_VERS_REEL:
        case OP_REEL_VERS_NATUREL:
        case OP_REEL_VERS_RELATIF:
        {
            return instruction_2d<int, int>(chunk, décalage, os);
        }
        case OP_LOGUE_APPEL:
        case OP_REFERENCE_GLOBALE_EXTERNE:
        {
            return instruction_1d<void *>(chunk, décalage, os);
        }
        case OP_LOGUE_ENTRÉES:
        {
            return instruction_2d<void *, uint32_t>(chunk, décalage, os);
        }
        default:
        {
            os << "Code Opération inconnu : " << instruction << '\n';
            return décalage + 1;
        }
    }
}

static void désassemble(const Chunk &chunk, kuri::chaine_statique nom, Enchaineuse &os)
{
    os << "== " << nom << " ==\n";
    for (auto décalage = int64_t(0); décalage < chunk.compte;) {
        décalage = désassemble_instruction(chunk, décalage, os);
    }
}

kuri::chaine désassemble(const Chunk &chunk, kuri::chaine_statique nom)
{
    Enchaineuse enchaineuse;
    désassemble(chunk, nom, enchaineuse);
    return enchaineuse.chaine();
}

ffi_type *converti_type_ffi(Type const *type)
{
    switch (type->genre) {
        case GenreNoeud::POLYMORPHIQUE:
        {
            assert_rappel(false, [&]() { dbg() << "Type polymorphique dans la conversion FFI"; });
            return static_cast<ffi_type *>(nullptr);
        }
        case GenreNoeud::TUPLE:
        {
            assert_rappel(false, [&]() { dbg() << "Type tuple dans la conversion FFI"; });
            return static_cast<ffi_type *>(nullptr);
        }
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        {
            return &ffi_type_uint8;
        }
        case GenreNoeud::CHAINE:
        {
            static ffi_type *types_elements[] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};

            static ffi_type type_ffi_chaine = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_chaine;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            static ffi_type *types_elements[] = {
                &ffi_type_pointer, &ffi_type_sint64, &ffi_type_sint64, nullptr};

            static ffi_type type_ffi_tableau = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_tableau;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            static ffi_type *types_éléments[] = {&ffi_type_pointer, &ffi_type_sint64, nullptr};
            static ffi_type type_ffi_tranche = {0, 0, FFI_TYPE_STRUCT, types_éléments};
            return &type_ffi_tranche;
        }
        case GenreNoeud::EINI:
        {
            static ffi_type *types_elements[] = {&ffi_type_pointer, &ffi_type_pointer, nullptr};

            static ffi_type type_ffi_eini = {0, 0, FFI_TYPE_STRUCT, types_elements};

            return &type_ffi_eini;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            return &ffi_type_sint32;
        }
        case GenreNoeud::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                return &ffi_type_uint8;
            }

            if (type->taille_octet == 2) {
                return &ffi_type_uint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_uint32;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_uint64;
            }

            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                return &ffi_type_sint8;
            }

            if (type->taille_octet == 2) {
                return &ffi_type_sint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_sint32;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_sint64;
            }

            break;
        }
        case GenreNoeud::RÉEL:
        {
            if (type->taille_octet == 2) {
                return &ffi_type_uint16;
            }

            if (type->taille_octet == 4) {
                return &ffi_type_float;
            }

            if (type->taille_octet == 8) {
                return &ffi_type_double;
            }

            break;
        }
        case GenreNoeud::RIEN:
        {
            return &ffi_type_void;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::FONCTION:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            return &ffi_type_pointer;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            // non supporté pour le moment, nous devrions uniquement passer des pointeurs
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            return converti_type_ffi(type_opaque->type_opacifié);
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            if (type_union->est_nonsure) {
                return converti_type_ffi(type_union->type_le_plus_grand);
            }

            // non supporté
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            return converti_type_ffi(static_cast<TypeEnum const *>(type)->type_sous_jacent);
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            return &ffi_type_sint64;
        }
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TABLEAU_FIXE:
        {
            // ces types là ne sont pas supporté dans FFI
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    return static_cast<ffi_type *>(nullptr);
}

/* ************************************************************************** */

CompilatriceCodeBinaire::CompilatriceCodeBinaire(EspaceDeTravail *espace_,
                                                 MetaProgramme *metaprogramme_)
    : espace(espace_), données_exécutions(&espace_->données_constantes_exécutions),
      métaprogramme(metaprogramme_)
{
    vérifie_adresses = espace->compilatrice().arguments.debogue_execution;
    notifie_empilage = espace->compilatrice().arguments.debogue_execution;
    émets_stats_ops = espace->compilatrice().arguments.émets_stats_ops_exécution;
    émets_profilage = espace->compilatrice().arguments.profile_metaprogrammes;
}

bool CompilatriceCodeBinaire::génère_code(ProgrammeRepreInter const &repr_inter)
{
    POUR (repr_inter.donne_globales_info_types()) {
        if (it->index != -1) {
            continue;
        }

        if (!ajoute_globale(it)) {
            return false;
        }
    }

    kuri::tableau<AtomeGlobale *> globales_requérant_génération_code;
    POUR (repr_inter.donne_globales_non_info_types()) {
        if (it->index != -1) {
            continue;
        }

        if (!ajoute_globale(it)) {
            return false;
        }

        globales_requérant_génération_code.ajoute(it);
    }

    POUR (globales_requérant_génération_code) {
        génère_code_pour_globale(it);
    }

    POUR (repr_inter.donne_fonctions()) {
        /* Évite de recréer le code binaire. */
        if (it->données_exécution) {
            continue;
        }

        it->données_exécution = mémoire::loge<DonnéesExécutionFonction>(
            "DonnéesExécutionFonction");
        m_atome_fonction_courante = it;
        fonction_courante = it->decl;

        if (!génère_code_pour_fonction(it)) {
            return false;
        }

        /* Les fonction d'initialisation de globales n'ont pas de déclarations. */
        if (it->decl) {
            if (it->decl->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_CODE_BINAIRE_FUT_REQUIS)) {
                dbg() << désassemble(it->données_exécution->chunk, it->nom);
            }
        }
    }

    return true;
}

bool CompilatriceCodeBinaire::génère_code_pour_fonction(AtomeFonction const *fonction)
{
    auto données_exécution = fonction->données_exécution;

    /* Certains AtomeFonction créés par la compilatrice n'ont pas de déclaration. */
    if (fonction->decl && fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        if (fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
            return true;
        }

        auto &donnees_externe = données_exécution->données_externe;
        auto decl = fonction->decl;

        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
            donnees_externe.ptr_fonction = fonction_compilatrice_pour_ident(decl->ident);
        }
        else {
            /* Nous ne pouvons appeler une fonction prenant un pointeur de fonction car le pointeur
             * pourrait être une fonction interne dont l'adresse ne sera pas celle d'une fonction
             * exécutable (pour le système d'exploitation) mais l'adresse de l'AtomeFonction
             * correspondant qui est utilisée dans la machine virtuelle. */
            POUR (decl->params) {
                if (it->type->est_type_fonction()) {
                    espace->rapporte_erreur(fonction->decl,
                                            "Impossible d'appeler dans un métaprogramme une "
                                            "fonction externe utilisant un pointeur de fonction");
                    return false;
                }
            }

            if (!decl->données_externes->symbole->charge(
                    espace, decl, RaisonRechercheSymbole::EXÉCUTION_MÉTAPROGRAMME)) {
                return false;
            }

            donnees_externe.ptr_fonction =
                decl->données_externes->symbole->donne_adresse_fonction_pour_exécution();
        }

        if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_VARIADIQUE)) {
            /* Les fonctions variadiques doivent être préparées pour chaque appel. */
            return true;
        }

        auto type_fonction = fonction->type->comme_type_fonction();
        donnees_externe.types_entrées.réserve(type_fonction->types_entrées.taille());

        POUR (type_fonction->types_entrées) {
            donnees_externe.types_entrées.ajoute(converti_type_ffi(it));
        }

        auto type_ffi_sortie = converti_type_ffi(type_fonction->type_sortie);
        auto nombre_arguments = static_cast<unsigned>(donnees_externe.types_entrées.taille());
        auto ptr_types_entrees = donnees_externe.types_entrées.données();

        auto status = ffi_prep_cif(&donnees_externe.cif,
                                   FFI_DEFAULT_ABI,
                                   nombre_arguments,
                                   type_ffi_sortie,
                                   ptr_types_entrees);

        if (status != FFI_OK) {
            espace->rapporte_erreur(
                decl, "Impossible de préparer l'interface d'appel forrain pour la fonction");
            return false;
        }

        return true;
    }

    auto &chunk = données_exécution->chunk;
    chunk.m_typeuse = &espace->typeuse;
    chunk.émets_stats_ops = émets_stats_ops;
    chunk.émets_vérification_branches = vérifie_adresses;
    chunk.émets_notifications_empilage = notifie_empilage;
    chunk.émets_profilage = émets_profilage;

    m_indice_locales.redimensionne(fonction->nombre_d_instructions_avec_entrées_sorties());
    fonction->numérote_instructions();

    POUR (fonction->params_entrée) {
        m_indice_locales[it->numero] = chunk.ajoute_locale(it);
    }

    /* crée une variable local pour la valeur de sortie */
    if (fonction->param_sortie) {
        auto alloc = fonction->param_sortie;
        auto type_pointe = alloc->donne_type_alloué();

        if (!type_pointe->est_type_rien()) {
            m_indice_locales[alloc->numero] = chunk.ajoute_locale(alloc);
        }
    }

    POUR (fonction->instructions) {
        if (it->est_alloc()) {
            m_indice_locales[it->numero] = chunk.ajoute_locale(it->comme_alloc());
        }
    }

    POUR (fonction->instructions) {
        if (!instruction_est_racine(it)) {
            continue;
        }

        génère_code_pour_instruction(it, chunk, false);
    }

    POUR (patchs_labels) {
        auto décalage = décalages_labels[it.indice_label];
        *reinterpret_cast<int *>(&chunk.code[it.adresse]) = décalage;
    }

    /* Réinitialise à la fin pour ne pas polluer les données pour les autres fonctions. */
    décalages_labels.efface();
    patchs_labels.efface();
    m_indice_locales.efface();

    chunk.rétrécis_capacité_sur_taille();

    return true;
}

void CompilatriceCodeBinaire::génère_code_pour_instruction(Instruction const *instruction,
                                                           Chunk &chunk,
                                                           bool pour_operande)
{
    switch (instruction->genre) {
        case GenreInstruction::LABEL:
        {
            auto label = instruction->comme_label();
            if (décalages_labels.taille() <= label->id) {
                décalages_labels.redimensionne(label->id + 1);
            }

            décalages_labels[label->id] = static_cast<int>(chunk.compte);
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto branche = instruction->comme_branche();
            chunk.émets_branche(branche->site, patchs_labels, branche->label->id);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto branche = instruction->comme_branche_cond();

            /* À FAIRE : évite de générer une branche si nous avons une constante. */
            if (branche->condition->est_instruction()) {
                if (auto atome = est_comparaison_égal_zéro_ou_nul(
                        branche->condition->comme_instruction())) {
                    génère_code_pour_atome(atome, chunk);
                    chunk.émets_branche_si_zéro(branche->site,
                                                patchs_labels,
                                                atome->type->taille_octet,
                                                branche->label_si_vrai->id,
                                                branche->label_si_faux->id);

                    break;
                }

                if (auto atome = est_comparaison_inégal_zéro_ou_nul(
                        branche->condition->comme_instruction())) {
                    génère_code_pour_atome(atome, chunk);
                    /* Utilise branche_si_zéro, mais inverse les labels. */
                    chunk.émets_branche_si_zéro(branche->site,
                                                patchs_labels,
                                                atome->type->taille_octet,
                                                branche->label_si_faux->id,
                                                branche->label_si_vrai->id);

                    break;
                }
            }

            génère_code_pour_atome(branche->condition, chunk);
            chunk.émets_branche_condition(branche->site,
                                          patchs_labels,
                                          branche->label_si_vrai->id,
                                          branche->label_si_faux->id);
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            auto alloc = instruction->comme_alloc();
            assert(pour_operande);
            chunk.émets_référence_locale(alloc->site, donne_indice_locale(alloc));
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto charge = instruction->comme_charge();

            if (est_allocation(charge->chargée)) {
                auto alloc = charge->chargée->comme_instruction()->comme_alloc();
                chunk.émets_charge_locale(charge->site, donne_indice_locale(alloc), charge->type);
                break;
            }

            génère_code_pour_atome(charge->chargée, chunk);
            chunk.émets_charge(charge->site, charge->type);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto stocke = instruction->comme_stocke_mem();

            if (est_stocke_alloc_incrémente(stocke)) {
                auto alloc_destination = static_cast<InstructionAllocation const *>(
                    stocke->destination);
                chunk.émets_incrémente_locale(
                    stocke->site, stocke->source->type, donne_indice_locale(alloc_destination));
                break;
            }

            if (auto alloc_source = est_stocke_alloc_depuis_charge_alloc(stocke)) {
                auto alloc_destination = static_cast<InstructionAllocation const *>(
                    stocke->destination);
                chunk.émets_copie_locale(stocke->site,
                                         stocke->source->type,
                                         donne_indice_locale(alloc_source),
                                         donne_indice_locale(alloc_destination));
                break;
            }

            if (est_allocation(stocke->destination) &&
                est_constante_entière_zéro(stocke->source)) {
                auto alloc_destination = static_cast<InstructionAllocation const *>(
                    stocke->destination);
                chunk.émets_init_locale_zéro(
                    stocke->site, donne_indice_locale(alloc_destination), stocke->source->type);
                break;
            }

            génère_code_pour_atome(stocke->source, chunk);

            if (est_allocation(stocke->destination)) {
                auto alloc = stocke->destination->comme_instruction()->comme_alloc();
                chunk.émets_assignation_locale(
                    stocke->site, donne_indice_locale(alloc), stocke->source->type);
                break;
            }

            // l'adresse de la valeur doit être au sommet de la pile lors de l'assignation
            génère_code_pour_atome(stocke->destination, chunk);
            chunk.émets_assignation(contexte(), stocke->site, stocke->source->type);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto appel = instruction->comme_appel();

            /* Évite de générer deux fois le code pour les appels : une fois dans la boucle sur les
             * instructions, une fois pour l'opérande. Les fonctions retournant « rien » ne peuvent
             * être opérandes. */
            if (!appel->type->est_type_rien() && !pour_operande) {
                return;
            }

            auto appelee = appel->appelé;
            auto taille_arguments = 0u;

            POUR (appel->args) {
                génère_code_pour_atome(it, chunk);
                auto type_primitif = donne_type_primitif(it->type);
                taille_arguments += type_primitif->taille_octet;
            }

            if (appelee->genre_atome == Atome::Genre::FONCTION) {
                auto atome_appelee = appelee->comme_fonction();

                if (atome_appelee->decl &&
                    atome_appelee->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
                    chunk.émets_appel_intrinsèque(appel->site, atome_appelee, appel);
                }
                else if (atome_appelee->decl && atome_appelee->decl->possède_drapeau(
                                                    DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
                    chunk.émets_appel_compilatrice(appel->site, atome_appelee, appel);
                }
                else if (atome_appelee->est_externe) {
                    chunk.émets_appel_externe(appel->site, atome_appelee, taille_arguments, appel);
                }
                else {
                    chunk.émets_appel(appel->site, atome_appelee, appel, taille_arguments);
                }
            }
            else {
                génère_code_pour_atome(appelee, chunk);
                chunk.émets_appel_pointeur(appel->site, taille_arguments, appel);
            }

            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = instruction->comme_retour();

            if (retour->valeur) {
                génère_code_pour_atome(retour->valeur, chunk);
            }

            chunk.émets_retour(retour->site, retour->valeur);
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto transtype = instruction->comme_transtype();
            auto valeur = transtype->valeur;

            génère_code_pour_atome(valeur, chunk);

            auto opt_op_transtype = converti_type_transtypage(transtype->op);
            if (opt_op_transtype.has_value()) {
                auto op_transtype = opt_op_transtype.value();
                if (op_transtype == OP_AUGMENTE_REEL) {
                    chunk.émets_transtype(transtype->site, op_transtype, 4, 8);
                }
                else if (op_transtype == OP_DIMINUE_REEL) {
                    chunk.émets_transtype(transtype->site, op_transtype, 8, 4);
                }
                else {
                    chunk.émets_transtype(transtype->site,
                                          op_transtype,
                                          valeur->type->taille_octet,
                                          transtype->type->taille_octet);
                }
            }

            break;
        }
        case GenreInstruction::ACCÈDE_INDICE:
        {
            auto index = instruction->comme_acces_index();
            auto type_pointeur = index->type->comme_type_pointeur();
            génère_code_pour_atome(index->index, chunk);
            génère_code_pour_atome(index->accédé, chunk);

            if (index->accédé->genre_atome == Atome::Genre::INSTRUCTION) {
                auto type_accede = index->donne_type_accédé();

                // l'accédé est le pointeur vers le pointeur, donc déréférence-le
                if (type_accede->est_type_pointeur()) {
                    chunk.émets_charge(index->site, type_pointeur);
                }
            }

            chunk.émets_accès_index(index->site, type_pointeur->type_pointé);
            break;
        }
        case GenreInstruction::ACCEDE_RUBRIQUE:
        {
            auto rubrique = instruction->comme_acces_rubrique();
            auto accès_fusionné = fusionne_accès_rubriques(rubrique);

            if (est_allocation(accès_fusionné.accédé)) {
                auto alloc = accès_fusionné.accédé->comme_instruction()->comme_alloc();
                chunk.émets_référence_rubrique_locale(
                    rubrique->site, donne_indice_locale(alloc), accès_fusionné.décalage);
                break;
            }

            génère_code_pour_atome(accès_fusionné.accédé, chunk);
            chunk.émets_référence_rubrique(rubrique->site, accès_fusionné.décalage);
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto op_unaire = instruction->comme_op_unaire();
            auto type = op_unaire->valeur->type;
            génère_code_pour_atome(op_unaire->valeur, chunk);
            chunk.émets_operation_unaire(op_unaire->site, op_unaire->op, type);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto op_binaire = instruction->comme_op_binaire();
            auto type_gauche = op_binaire->valeur_gauche->type;

            génère_code_pour_atome(op_binaire->valeur_gauche, chunk);

            if (op_binaire->op == OpérateurBinaire::Genre::Addition &&
                est_constante_entière_un(op_binaire->valeur_droite)) {
                chunk.émets_incrémente(op_binaire->site, type_gauche);
                break;
            }
            if (op_binaire->op == OpérateurBinaire::Genre::Soustraction &&
                est_constante_entière_un(op_binaire->valeur_droite)) {
                chunk.émets_décrémente(op_binaire->site, type_gauche);
                break;
            }

            génère_code_pour_atome(op_binaire->valeur_droite, chunk);

            auto type_droite = op_binaire->valeur_droite->type;
            chunk.émets_operation_binaire(
                op_binaire->site, op_binaire->op, op_binaire->type, type_gauche, type_droite);

            break;
        }
        case GenreInstruction::INATTEIGNABLE:
        {
            auto inst_inatteignable = instruction->comme_inatteignable();
            chunk.émets_inatteignable(inst_inatteignable->site);
            break;
        }
        case GenreInstruction::SÉLECTION:
        {
            auto sélection = instruction->comme_sélection();
            génère_code_pour_atome(sélection->si_faux, chunk);
            génère_code_pour_atome(sélection->si_vrai, chunk);
            génère_code_pour_atome(sélection->condition, chunk);
            chunk.émets_sélection(sélection->site, sélection->type);
            break;
        }
    }
}

void CompilatriceCodeBinaire::génère_code_pour_atome(Atome const *atome, Chunk &chunk)
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto atome_globale = atome->comme_globale();
            auto globale = données_exécutions->globales[atome_globale->index];
            if (globale.adresse_pour_exécution) {
                chunk.émets_référence_globale_externe(nullptr, globale.adresse_pour_exécution);
            }
            else {
                chunk.émets_référence_globale(nullptr, atome_globale->index);
            }
            break;
        }
        case Atome::Genre::FONCTION:
        {
            /* L'adresse pour les pointeurs de fonctions. */
            chunk.émets_constante(reinterpret_cast<int64_t>(atome));
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            génère_code_pour_instruction(atome->comme_instruction(), chunk, true);
            break;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = atome->comme_transtype_constant();
            génère_code_pour_atome(transtype->valeur, chunk);
            break;
        }
        case Atome::Genre::ACCÈS_INDICE_CONSTANT:
        {
            auto indice_constant = atome->comme_accès_indice_constant();
            auto type_pointeur = indice_constant->type->comme_type_pointeur();
            chunk.émets_constante(indice_constant->index);
            génère_code_pour_atome(indice_constant->accédé, chunk);
            chunk.émets_accès_index(nullptr, type_pointeur->type_pointé);
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            chunk.émets_constante(int64_t(0));
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            auto type = atome->comme_constante_type()->donne_type();
            assert_rappel(type->atome_info_type,
                          [&]() { dbg() << "Pas d'atome_info_type pour " << chaine_type(type); });
            chunk.émets_référence_globale(nullptr, type->atome_info_type->index);
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = atome->comme_taille_de()->type_de_données;
            chunk.émets_constante(type->taille_octet);
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto type = constante_réelle->type;

            if (type->taille_octet == 4) {
                chunk.émets_constante(static_cast<float>(constante_réelle->valeur));
            }
            else {
                chunk.émets_constante(constante_réelle->valeur);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            auto valeur_entiere = constante_entière->valeur;
            auto type = type_entier_sous_jacent(constante_entière->type);

            if (type->est_type_entier_naturel()) {
                if (type->taille_octet == 1) {
                    chunk.émets_constante(static_cast<unsigned char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.émets_constante(static_cast<unsigned short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.émets_constante(static_cast<uint32_t>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.émets_constante(valeur_entiere);
                }
            }
            else if (type->est_type_entier_relatif()) {
                if (type->taille_octet == 1) {
                    chunk.émets_constante(static_cast<char>(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    chunk.émets_constante(static_cast<short>(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    chunk.émets_constante(static_cast<int>(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    chunk.émets_constante(static_cast<int64_t>(valeur_entiere));
                }
            }

            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            chunk.émets_constante(constante_booléenne->valeur);
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            chunk.émets_constante(static_cast<char>(caractère->valeur));
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = atome->comme_constante_structure();
            auto type = atome->type;
            auto tableau_valeur = structure->donne_atomes_rubriques();

            auto décalage = chunk.émets_structure_constante(type->taille_octet);
            auto destination = chunk.code + décalage;

            auto type_composé = type->comme_type_composé();

            auto adressage_destination = AdresseDonnéesExécution{
                CODE_FONCTION, 0, m_atome_fonction_courante};

            POUR_INDICE (type_composé->donne_rubriques_pour_code_machine()) {
                auto destination_rubrique = destination + it.decalage;
                auto décalage_rubrique = décalage + int(it.decalage);

                génère_code_atome_constant(tableau_valeur[indice_it],
                                           adressage_destination,
                                           destination_rubrique,
                                           décalage_rubrique);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau = atome->comme_constante_tableau();
            auto type = tableau->type;
            auto type_élément = type->comme_type_tableau_fixe()->type_pointé;
            auto éléments = tableau->donne_atomes_éléments();

            auto décalage = chunk.émets_structure_constante(type->taille_octet);
            auto destination = chunk.code + décalage;

            auto adressage_destination = AdresseDonnéesExécution{
                CODE_FONCTION, 0, m_atome_fonction_courante};

            auto destination_rubrique = destination;
            auto décalage_rubrique = décalage;
            POUR (éléments) {
                génère_code_atome_constant(
                    it, adressage_destination, destination_rubrique, décalage_rubrique);
                destination_rubrique += type_élément->taille_octet;
                décalage_rubrique += int(type_élément->taille_octet);
            }

            break;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto init_tableau = atome->comme_initialisation_tableau();
            auto type_tableau = init_tableau->type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointé;

            auto décalage = chunk.émets_structure_constante(type_tableau->taille_octet);
            auto destination = chunk.code + décalage;

            auto adressage_destination = AdresseDonnéesExécution{
                CODE_FONCTION, 0, m_atome_fonction_courante};

            for (auto i = 0; i < type_tableau->taille; i++) {
                génère_code_atome_constant(
                    init_tableau->valeur, adressage_destination, destination, décalage);
                destination += type_élément->taille_octet;
                décalage += int(type_élément->taille_octet);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_INDICE_TABLE_TYPE:
        {
            /* Nous utilisons directement InfoType* créer pour les NoeudCode. Nous ne devions pas
             * être ici. */
            assert_rappel(false,
                          []() { dbg() << "CONSTANTE_INDICE_TABLE_TYPE dans le code binaire."; });
            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            break;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            break;
        }
    }
}

template <typename T>
static inline void assigne(octet_t *tampon, T valeur)
{
    *reinterpret_cast<T *>(tampon) = valeur;
}

void CompilatriceCodeBinaire::génère_code_atome_constant(
    AtomeConstante const *atome,
    AdresseDonnéesExécution const &adressage_destination,
    octet_t *destination,
    int décalage) const
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto atome_globale = atome->comme_globale();
            auto globale = données_exécutions->globales[atome_globale->index];
            if (globale.adresse_pour_exécution) {
                assigne(destination, globale.adresse_pour_exécution);
            }
            else {
                assert(!atome_globale->est_externe);
                ajoute_réadressage_pour_globale(globale, adressage_destination, décalage);
            }
            break;
        }
        case Atome::Genre::FONCTION:
        {
            /* L'adresse pour les pointeurs de fonctions. */
            assigne(destination, atome);
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            assert(false);
            break;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype = atome->comme_transtype_constant();
            génère_code_atome_constant(
                transtype->valeur, adressage_destination, destination, décalage);
            break;
        }
        case Atome::Genre::ACCÈS_INDICE_CONSTANT:
        {
            auto indexage = atome->comme_accès_indice_constant();
            auto indexée = indexage->accédé->comme_globale();

            if (!indexée->initialisateur) {
                auto globale = données_exécutions->globales[indexée->index];
                if (globale.adresse_pour_exécution) {
                    assigne(destination, globale.adresse_pour_exécution);
                }
                else {
                    assert(!indexée->est_externe);
                    ajoute_réadressage_pour_globale(globale, adressage_destination, décalage);
                }
                return;
            }

            if (indexée->initialisateur->est_données_constantes()) {
                auto tableau = indexée->initialisateur->comme_données_constantes();
                auto données_tableau = tableau->donne_données();
                assigne(destination,
                        static_cast<void *>(données_tableau.begin() + indexage->index));
            }
            else if (indexée->initialisateur->est_constante_tableau()) {
                assert(indexage->index == 0);
                auto globale = données_exécutions->globales[indexée->index];
                if (globale.adresse_pour_exécution) {
                    assigne(destination, globale.adresse_pour_exécution);
                }
                else {
                    assert(!indexée->est_externe);
                    ajoute_réadressage_pour_globale(globale, adressage_destination, décalage);
                }
            }
            else {
                assert_rappel(false, [&]() {
                    dbg() << "Indexage constant non-implémenté dans le code binaire pour le genre "
                             "d'atome "
                          << indexée->initialisateur->genre_atome;
                });
            }
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            *reinterpret_cast<int64_t *>(destination) = int64_t(0);
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            auto type = atome->comme_constante_type()->donne_type();
            assert_rappel(type->atome_info_type,
                          [&]() { dbg() << "Pas d'atome_info_type pour " << chaine_type(type); });
            auto globale = données_exécutions->globales[type->atome_info_type->index];
            ajoute_réadressage_pour_globale(globale, adressage_destination, décalage);
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = atome->comme_taille_de()->type_de_données;
            assigne(destination, type->taille_octet);
            break;
        }
        case Atome::Genre::CONSTANTE_INDICE_TABLE_TYPE:
        {
            /* Nous utilisons directement InfoType* créer pour les NoeudCode. Nous ne devions pas
             * être ici. */
            assert_rappel(false,
                          []() { dbg() << "CONSTANTE_INDICE_TABLE_TYPE dans le code binaire."; });
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto type = constante_réelle->type;

            if (type->taille_octet == 4) {
                assigne(destination, static_cast<float>(constante_réelle->valeur));
            }
            else {
                assigne(destination, constante_réelle->valeur);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            auto valeur_entiere = constante_entière->valeur;
            auto type = type_entier_sous_jacent(constante_entière->type);

            if (type->est_type_entier_naturel()) {
                if (type->taille_octet == 1) {
                    assigne(destination, uint8_t(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    assigne(destination, uint16_t(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    assigne(destination, uint32_t(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    assigne(destination, valeur_entiere);
                }
            }
            else if (type->est_type_entier_relatif()) {
                if (type->taille_octet == 1) {
                    assigne(destination, int8_t(valeur_entiere));
                }
                else if (type->taille_octet == 2) {
                    assigne(destination, int16_t(valeur_entiere));
                }
                else if (type->taille_octet == 4) {
                    assigne(destination, int32_t(valeur_entiere));
                }
                else if (type->taille_octet == 8) {
                    assigne(destination, int64_t(valeur_entiere));
                }
            }

            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            assigne(destination, constante_booléenne->valeur);
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            assigne(destination, char(caractère->valeur));
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = atome->comme_constante_structure();
            auto type = atome->type;
            auto tableau_valeur = structure->donne_atomes_rubriques();
            auto type_composé = type->comme_type_composé();

            POUR_INDICE (type_composé->donne_rubriques_pour_code_machine()) {
                auto destination_rubrique = destination + it.decalage;
                auto décalage_rubrique = décalage + int(it.decalage);
                génère_code_atome_constant(tableau_valeur[indice_it],
                                           adressage_destination,
                                           destination_rubrique,
                                           décalage_rubrique);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau_fixe = atome->comme_constante_tableau();
            auto type_tableau = atome->type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointé;

            auto destination_élément = destination;
            auto décalage_élément = décalage;
            POUR (tableau_fixe->donne_atomes_éléments()) {
                génère_code_atome_constant(
                    it, adressage_destination, destination_élément, décalage_élément);
                destination_élément += type_élément->taille_octet;
                décalage_élément += int(type_élément->taille_octet);
            }
            break;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto init_tableau = atome->comme_initialisation_tableau();
            auto type_tableau = init_tableau->type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointé;

            auto décalage_élément = décalage;
            auto destination_élément = destination;
            for (auto i = 0; i < type_tableau->taille; i++) {
                génère_code_atome_constant(init_tableau->valeur,
                                           adressage_destination,
                                           destination_élément,
                                           décalage_élément);
                destination_élément += type_élément->taille_octet;
                décalage_élément += int(type_élément->taille_octet);
            }

            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            assert(false);
            break;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            break;
        }
    }
}

bool CompilatriceCodeBinaire::ajoute_globale(AtomeGlobale *globale) const
{
    if (globale->index != -1) {
        /* Déjà généré par un autre métaprogramme. */
        return true;
    }

    void *adresse_pour_exécution = nullptr;
    if (globale->est_info_type_de) {
        adresse_pour_exécution = globale->est_info_type_de->info_type;
    }
    else if (globale->decl && globale->decl->données_externes) {
        auto decl = globale->decl;
        if (!decl->données_externes->symbole->charge(
                espace, decl, RaisonRechercheSymbole::EXÉCUTION_MÉTAPROGRAMME)) {
            return false;
        }

        adresse_pour_exécution =
            decl->données_externes->symbole->donne_adresse_objet_pour_exécution();
    }

    auto type_globale = globale->donne_type_alloué();
    auto index = données_exécutions->ajoute_globale(
        type_globale, globale->ident, adresse_pour_exécution);
    globale->index = index;
    return true;
}

void CompilatriceCodeBinaire::génère_code_pour_globale(AtomeGlobale const *atome_globale) const
{
    if (!atome_globale->initialisateur) {
        return;
    }

    auto index = atome_globale->index;
    auto adressage_destination = AdresseDonnéesExécution{DONNÉES_GLOBALES, 0};
    auto globale = données_exécutions->globales[index];
    auto destination = données_exécutions->données_globales.données() + globale.adresse;
    auto initialisateur = atome_globale->initialisateur;
    génère_code_atome_constant(
        initialisateur, adressage_destination, destination, globale.adresse);
}

int CompilatriceCodeBinaire::donne_indice_locale(const InstructionAllocation *alloc) const
{
    return m_indice_locales[alloc->numero];
}

ContexteGénérationCodeBinaire CompilatriceCodeBinaire::contexte() const
{
    return {espace, fonction_courante};
}

void CompilatriceCodeBinaire::ajoute_réadressage_pour_globale(
    Globale const &globale,
    AdresseDonnéesExécution const &adressage_destination,
    int décalage) const
{
    auto patch = PatchDonnéesConstantes{};
    patch.destination = adressage_destination;
    patch.destination.décalage = décalage;
    patch.source = {ADRESSE_GLOBALE, globale.adresse};
    données_exécutions->patchs_données_constantes.ajoute(patch);
}

int64_t DonnéesExécutionFonction::mémoire_utilisée() const
{
    int64_t résultat = 0;
    résultat += chunk.mémoire_utilisée();
    résultat += données_externe.types_entrées.taille_mémoire();
    return résultat;
}

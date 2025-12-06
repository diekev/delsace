/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#include "fichier_elf.hh"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "programme.hh"

#include "representation_intermediaire/instructions.hh"

#include "structures/chemin_systeme.hh"

#include "utilitaires/log.hh"
#include "utilitaires/logeuse_memoire.hh"

static constexpr uint64_t TAILLE_PAGE = 16 * 1024;

struct FichierPagé {
    struct Page {
        uint8_t *données = nullptr;
        uint64_t occupé = 0;
    };

  private:
    kuri::tableau<Page> m_pages{};

  public:
    FichierPagé() = default;

    ~FichierPagé()
    {
        POUR (m_pages) {
            mémoire::déloge_tableau("uint8_t", it.données, int64_t(TAILLE_PAGE));
        }
    }

    uint64_t donne_taille() const
    {
        auto résultat = uint64_t(0);

        POUR (m_pages) {
            résultat += it.occupé;
        }

        return résultat;
    }

    template <typename T>
    T *loge_en_un_seul_morceau()
    {
        auto page = donne_page_courante();
        auto résultat = reinterpret_cast<T *>(page->données + page->occupé);
        page->occupé += sizeof(T);
        new (résultat) T;
        return résultat;
    }

    void écris(kuri::chaine_statique chaine, bool avec_terminateur)
    {
        auto taille_chaine = uint64_t(chaine.taille());

        auto page = donne_page_courante();
        auto source = chaine.pointeur();
        while (taille_chaine != 0) {
            auto taille_restante = TAILLE_PAGE - page->occupé;
            if (taille_restante == 0) {
                ajoute_page();
                page = donne_page_courante();
                taille_restante = TAILLE_PAGE;
            }

            auto taille_à_écrire = std::min(taille_chaine, taille_restante);
            memcpy(page->données + page->occupé, source, taille_à_écrire);

            page->occupé += taille_à_écrire;
            source += taille_à_écrire;
            taille_chaine -= taille_à_écrire;
        }

        if (avec_terminateur) {
            écris_octet(0x0);
        }
    }

    void écris_octet(uint8_t octet)
    {
        auto page = donne_page_courante();
        if (page->occupé == TAILLE_PAGE) {
            ajoute_page();
            page = donne_page_courante();
        }

        page->données[page->occupé] = octet;
        page->occupé += 1;
    }

    void écris(kuri::tableau_statique<char> octets)
    {
        auto taille_octets = uint64_t(octets.taille());

        auto page = donne_page_courante();
        auto source = octets.begin();
        while (taille_octets != 0) {
            auto taille_restante = TAILLE_PAGE - page->occupé;
            if (taille_restante == 0) {
                ajoute_page();
                page = donne_page_courante();
                taille_restante = TAILLE_PAGE;
            }

            auto taille_à_écrire = std::min(taille_octets, taille_restante);
            memcpy(page->données + page->occupé, source, taille_à_écrire);

            page->occupé += taille_à_écrire;
            source += taille_à_écrire;
            taille_octets -= taille_à_écrire;
        }
    }

    kuri::tableau_statique<Page> pages() const
    {
        return m_pages;
    }

  private:
    Page *donne_page_courante()
    {
        if (m_pages.taille() == 0 || m_pages.dernier_élément().occupé == TAILLE_PAGE) {
            ajoute_page();
        }

        return &m_pages.dernier_élément();
    }

    void ajoute_page()
    {
        auto page = Page{};
        page.données = mémoire::loge_tableau<uint8_t>("uint8_t", int64_t(TAILLE_PAGE));
        page.occupé = 0;
        m_pages.ajoute(page);
    }
};

struct TableDeChaines {
  private:
    kuri::tableau<kuri::chaine_statique> m_chaines{};
    uint64_t taille_totale = 0;

  public:
    [[nodiscard]] uint64_t ajoute_chaine(kuri::chaine_statique chaine)
    {
        m_chaines.ajoute(chaine);
        auto résultat = taille_totale;
        /* +1 pour le terminateur nul. */
        taille_totale += uint64_t(chaine.taille()) + 1;
        return résultat;
    }

    kuri::tableau_statique<kuri::chaine_statique> chaines() const
    {
        return m_chaines;
    }

    uint64_t donne_taille() const
    {
        return taille_totale;
    }
};

FichierELF *FichierELF::crée_fichier_objet()
{
    auto résultat = mémoire::loge<FichierELF>("FichierELF");
    résultat->crée_section("", SHT_NULL);
    résultat->crée_section(".text", SHT_PROGBITS);
    résultat->crée_section(".data", SHT_PROGBITS);
    résultat->crée_section(".bss", SHT_NOBITS);
    résultat->crée_section(".tbss", SHT_NOBITS);
    résultat->crée_section(".rodata", SHT_PROGBITS);
    résultat->crée_section(".data.rel.ro.local", SHT_PROGBITS);
    résultat->crée_section(".rela.data.rel.ro.local", SHT_RELA);
    résultat->crée_section(".comment", SHT_PROGBITS);
    résultat->crée_section(".note.GNU-stack", SHT_PROGBITS);
    résultat->crée_section(".note.gnu.property", SHT_NOTE);
    résultat->crée_section(".symtab", SHT_SYMTAB);
    résultat->crée_section(".strtab", SHT_STRTAB);
    résultat->crée_section(".shstrtab", SHT_STRTAB);
    return résultat;
}

SectionELF *FichierELF::crée_section(kuri::chaine_statique nom, uint32_t type)
{
    auto résultat = mémoire::loge<SectionELF>("SectionELF");
    résultat->nom = nom;
    résultat->type = type;

    m_sections.ajoute(résultat);

    return résultat;
}

SectionELF *FichierELF::donne_section(kuri::chaine_statique nom) const
{
    SectionELF *résultat = nullptr;

    POUR (m_sections) {
        if (it->nom == nom) {
            résultat = it;
            break;
        }
    }

    return résultat;
}

uint16_t FichierELF::donne_indice_section(kuri::chaine_statique nom) const
{
    uint16_t résultat = 0;
    bool trouvé = false;

    POUR (m_sections) {
        if (it->nom == nom) {
            trouvé = true;
            break;
        }
        résultat += 1;
    }

    assert(trouvé);

    return résultat;
}

void FichierELF::écris_données_constantes(DonnéesConstantes const *données)
{
    m_données_constantes = données;
}

void FichierELF::écris_vers(kuri::chaine_statique chemin) const
{
    auto fichier = FichierPagé{};
    auto table_noms_entêtes_sections = TableDeChaines{};

    /* Entête fichier. */
    auto entête_fichier = fichier.loge_en_un_seul_morceau<Elf64_Ehdr>();

    entête_fichier->e_ident[EI_MAG0] = ELFMAG0;
    entête_fichier->e_ident[EI_MAG1] = ELFMAG1;
    entête_fichier->e_ident[EI_MAG2] = ELFMAG2;
    entête_fichier->e_ident[EI_MAG3] = ELFMAG3;
    entête_fichier->e_ident[EI_CLASS] = ELFCLASS64;
    entête_fichier->e_ident[EI_DATA] = ELFDATA2LSB;
    entête_fichier->e_ident[EI_VERSION] = EV_CURRENT;
    entête_fichier->e_ident[EI_OSABI] = ELFOSABI_SYSV;
    entête_fichier->e_ident[EI_ABIVERSION] = 0;
    for (int i = EI_PAD; i < EI_NIDENT; i++) {
        entête_fichier->e_ident[i] = 0;
    }

    // À FAIRE : détermine ceci lors de la création.
    entête_fichier->e_type = ET_REL;
    entête_fichier->e_machine = EM_X86_64;
    entête_fichier->e_version = EV_CURRENT;
    entête_fichier->e_entry = 0;
    // À FAIRE : détermine ceci.
    entête_fichier->e_phoff = 0;
    entête_fichier->e_flags = 0;
    entête_fichier->e_ehsize = sizeof(Elf64_Ehdr);
    // À FAIRE : détermine ceci.
    entête_fichier->e_phentsize = 0;
    // À FAIRE : détermine ceci.
    entête_fichier->e_phnum = 0;
    entête_fichier->e_shentsize = sizeof(Elf64_Shdr);
    entête_fichier->e_shnum = Elf64_Half(m_sections.taille());
    entête_fichier->e_shstrndx = donne_indice_section(".shstrtab");

    /* Entêtes sections. */
    entête_fichier->e_shoff = fichier.donne_taille();

    POUR (m_sections) {
        auto entête_section = fichier.loge_en_un_seul_morceau<Elf64_Shdr>();
        entête_section->sh_name = Elf64_Word(table_noms_entêtes_sections.ajoute_chaine(it->nom));
        entête_section->sh_type = it->type;
        entête_section->sh_flags = 0;
        entête_section->sh_addr = 0;
        entête_section->sh_offset = 0;
        entête_section->sh_size = 0;
        entête_section->sh_link = 0;
        entête_section->sh_info = 0;
        entête_section->sh_addralign = 0;
        entête_section->sh_entsize = 0;

        it->entête = entête_section;
    }

    auto section_shstrtab = donne_section(".shstrtab");
    section_shstrtab->entête->sh_size = table_noms_entêtes_sections.donne_taille();
    section_shstrtab->entête->sh_addralign = 1;

    if (m_données_constantes) {
        auto section_rodata = donne_section(".rodata");
        section_rodata->entête->sh_size = Elf64_Xword(
            m_données_constantes->taille_données_tableaux_constants);
        section_rodata->entête->sh_addralign = m_données_constantes->alignement_désiré;
    }

    auto décalage_section = fichier.donne_taille();

    /* Sections. */
    POUR (m_sections) {
        it->entête->sh_offset = décalage_section;
        décalage_section += it->entête->sh_size;

        dbg() << it->nom << ", taille " << it->entête->sh_size << ", décalage "
              << it->entête->sh_offset;

        if (it->nom == ".shstrtab") {
            POUR_NOMME (chaine, table_noms_entêtes_sections.chaines()) {
                fichier.écris(chaine, true);
            }
        }
        else if (it->nom == ".rodata" && m_données_constantes) {
            POUR_NOMME (tableau_constant, m_données_constantes->tableaux_constants) {
                for (auto i = 0; i < tableau_constant.rembourrage; i++) {
                    fichier.écris_octet(0x0);
                }

                auto tableau = tableau_constant.tableau->donne_données();
                fichier.écris(tableau);
            }
        }
    }

    /* Écriture du fichier. */
    auto std_path = vers_std_path(chemin);
    auto fd = open(std_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd != -1) {
        POUR (fichier.pages()) {
            auto octets_écris = write(fd, it.données, it.occupé);
            assert(octets_écris == it.occupé);
            dbg() << "Écris " << octets_écris << " octets";
        }

        close(fd);
    }
    else {
        perror("open");
    }
}

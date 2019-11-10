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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "decoupeuse.h"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/unicode.hh"

#include "erreur.h"

/* ************************************************************************** */

constexpr bool est_espace_blanc(char c)
{
	return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

/* ************************************************************************** */

/* espaces */
static constexpr auto ESPACE_INSECABLE             = 0x00A0;
static constexpr auto ESPACE_D_OGAM                = 0x1680;
static constexpr auto SEPARATEUR_VOYELLES_MONGOL   = 0x180E;
static constexpr auto DEMI_CADRATIN                = 0x2000;
static constexpr auto CADRATIN                     = 0x2001;
static constexpr auto ESPACE_DEMI_CADRATIN         = 0x2002;
static constexpr auto ESPACE_CADRATIN              = 0x2003;
static constexpr auto TIERS_DE_CADRATIN            = 0x2004;
static constexpr auto QUART_DE_CADRATIN            = 0x2005;
static constexpr auto SIXIEME_DE_CADRATIN          = 0x2006;
static constexpr auto ESPACE_TABULAIRE             = 0x2007;
static constexpr auto ESPACE_PONCTUATION           = 0x2008;
static constexpr auto ESPACE_FINE                  = 0x2009;
static constexpr auto ESPACE_ULTRAFINE             = 0x200A;
static constexpr auto ESPACE_SANS_CHASSE           = 0x200B;
static constexpr auto ESPACE_INSECABLE_ETROITE     = 0x202F;
static constexpr auto ESPACE_MOYENNE_MATHEMATIQUE  = 0x205F;
static constexpr auto ESPACE_IDEOGRAPHIQUE         = 0x3000;
static constexpr auto ESPACE_INSECABLE_SANS_CHASSE = 0xFEFF;

/* guillemets */
static constexpr auto GUILLEMET_OUVRANT = 0x00AB;  /* « */
static constexpr auto GUILLEMET_FERMANT = 0x00BB;  /* » */

constexpr auto converti_utf32(const char *sequence, int n)
{
	auto const s0 = static_cast<unsigned char>(sequence[0]);

	if (n == 1) {
		return static_cast<int>(s0) & 0b01111111;
	}

	auto const s1 = static_cast<unsigned char>(sequence[1]);

	if (n == 2) {
		auto valeur = (s0 & 0b00011111) << 6 | (s1 & 0b00111111);
		return valeur;
	}

	auto const s2 = static_cast<unsigned char>(sequence[2]);

	if (n == 3) {
		auto valeur = (s0 & 0b00001111) << 12 | (s1 & 0b00111111) << 6 | (s2 & 0b00111111);
		return valeur;
	}

	auto const s3 = static_cast<unsigned char>(sequence[3]);

	if (n == 4) {
		auto valeur = (s0 & 0b00000111) << 18 | (s1 & 0b00111111) << 12 | (s2 & 0b00111111) << 6 | (s3 & 0b00111111);
		return valeur;
	}

	return 0;
}

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(DonneesModule *module, int drapeaux)
	: m_module(module)
	, m_debut_mot(module->tampon.debut())
	, m_debut(module->tampon.debut())
	, m_fin(module->tampon.fin())
	, m_drapeaux(drapeaux)
{
	construit_tables_caractere_speciaux();
}

void decoupeuse_texte::genere_morceaux()
{
	m_taille_mot_courant = 0;

	while (!this->fini()) {
		auto nombre_octet = lng::nombre_octets(m_debut);

		switch (nombre_octet) {
			case 1:
			{
				analyse_caractere_simple();
				break;
			}
			case 2:
			case 3:
			case 4:
			{
				if (m_taille_mot_courant == 0) {
					this->enregistre_pos_mot();
				}

				auto c = converti_utf32(m_debut, nombre_octet);

				switch (c) {
					case ESPACE_INSECABLE:
					case ESPACE_D_OGAM:
					case SEPARATEUR_VOYELLES_MONGOL:
					case DEMI_CADRATIN:
					case CADRATIN:
					case ESPACE_DEMI_CADRATIN:
					case ESPACE_CADRATIN:
					case TIERS_DE_CADRATIN:
					case QUART_DE_CADRATIN:
					case SIXIEME_DE_CADRATIN:
					case ESPACE_TABULAIRE:
					case ESPACE_PONCTUATION:
					case ESPACE_FINE:
					case ESPACE_ULTRAFINE:
					case ESPACE_SANS_CHASSE:
					case ESPACE_INSECABLE_ETROITE:
					case ESPACE_MOYENNE_MATHEMATIQUE:
					case ESPACE_IDEOGRAPHIQUE:
					case ESPACE_INSECABLE_SANS_CHASSE:
					{
						if (m_taille_mot_courant != 0) {
							this->pousse_mot(id_chaine(this->mot_courant()));
						}

						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->enregistre_pos_mot();
							this->avance(nombre_octet);
							this->pousse_mot(id_morceau::CARACTERE_BLANC);
						}
						else {
							this->avance(nombre_octet);
						}

						break;
					}
					case GUILLEMET_OUVRANT:
					{
						if (m_taille_mot_courant != 0) {
							this->pousse_mot(id_chaine(this->mot_courant()));
						}

						/* Saute le premier guillemet si nécessaire. */
						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->enregistre_pos_mot();
							this->pousse_caractere(nombre_octet);
							this->avance(nombre_octet);
						}
						else {
							this->avance(nombre_octet);
							this->enregistre_pos_mot();
						}

						while (!this->fini()) {
							nombre_octet = lng::nombre_octets(m_debut);
							c = converti_utf32(m_debut, nombre_octet);

							if (c == GUILLEMET_FERMANT) {
								break;
							}

							m_taille_mot_courant += nombre_octet;
							this->avance(nombre_octet);
						}

						/* Saute le dernier guillemet si nécessaire. */
						if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
							this->pousse_caractere(nombre_octet);
						}

						this->avance(nombre_octet);

						this->pousse_mot(id_morceau::CHAINE_LITTERALE);
						break;
					}
					default:
					{
						m_taille_mot_courant += nombre_octet;
						this->avance(nombre_octet);
						break;
					}
				}

				break;
			}
			default:
			{
				/* Le caractère (octet) courant est invalide dans le codec unicode. */
				lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
			}
		}
	}

	if (m_taille_mot_courant != 0) {
		lance_erreur("Des caractères en trop se trouvent à la fin du texte !");
	}
}

size_t decoupeuse_texte::memoire_morceaux() const
{
	return static_cast<size_t>(m_module->morceaux.taille()) * sizeof(DonneesMorceau);
}

void decoupeuse_texte::imprime_morceaux(std::ostream &os)
{
	for (auto const &morceau : m_module->morceaux) {
		os << chaine_identifiant(morceau.identifiant) << '\n';
	}
}

bool decoupeuse_texte::fini() const
{
	return m_debut >= m_fin;
}

void decoupeuse_texte::avance(int n)
{
	for (int i = 0; i < n; ++i) {
		if (this->caractere_courant() == '\n') {
			++m_compte_ligne;
			m_position_ligne = 0;
		}
		else {
			++m_position_ligne;
		}

		++m_debut;
	}
}

char decoupeuse_texte::caractere_courant() const
{
	return *m_debut;
}

char decoupeuse_texte::caractere_voisin(int n) const
{
	return *(m_debut + n);
}

dls::vue_chaine_compacte decoupeuse_texte::mot_courant() const
{
	return dls::vue_chaine_compacte(m_debut_mot, m_taille_mot_courant);
}

void decoupeuse_texte::lance_erreur(const dls::chaine &quoi) const
{
	auto ligne_courante = m_module->tampon[m_compte_ligne];

	dls::flux_chaine ss;
	ss << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
	ss << ligne_courante;

	/* La position ligne est en octet, il faut donc compter le nombre d'octets
	 * de chaque point de code pour bien formater l'erreur. */
	for (auto i = 0l; i < m_position_ligne;) {
		if (ligne_courante[i] == '\t') {
			ss << '\t';
		}
		else {
			ss << ' ';
		}

		i += lng::decalage_pour_caractere(ligne_courante, i);
	}

	ss << "^~~~\n";
	ss << quoi;

	throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::DECOUPAGE);
}

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe trois point
//        sinon si caractere est '"':
//            decoupe chaine caractere littérale
//        sinon si caractere est '#':
//            decoupe commentaire
//        sinon si caractere est '\'':
//            decoupe caractere
//        sinon:
//        	ajoute mot caractere simple
// sinon si nombre et mot est vide:
//    decoupe nombre
// sinon:
//    ajoute caractere mot courant
void decoupeuse_texte::analyse_caractere_simple()
{
	auto idc = id_morceau::INCONNU;

	if (est_espace_blanc(this->caractere_courant())) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
			this->enregistre_pos_mot();
			this->pousse_caractere();
			this->pousse_mot(id_morceau::CARACTERE_BLANC);
		}

		this->avance();
	}
	else if (est_caractere_special(this->caractere_courant(), idc)) {
		if (m_taille_mot_courant != 0) {
			this->pousse_mot(id_chaine(this->mot_courant()));
		}

		this->enregistre_pos_mot();

		auto id = id_trigraphe(dls::vue_chaine_compacte(m_debut, 3));

		if (id != id_morceau::INCONNU) {
			this->pousse_caractere(3);
			this->pousse_mot(id);
			this->avance(3);
			return;
		}

		id = id_digraphe(dls::vue_chaine_compacte(m_debut, 2));

		if (id != id_morceau::INCONNU) {
			this->pousse_caractere(2);
			this->pousse_mot(id);
			this->avance(2);
			return;
		}

		switch (this->caractere_courant()) {
			case '.':
			{
				this->pousse_caractere();
				this->pousse_mot(id_morceau::POINT);
				this->avance();
				break;
			}
			case '"':
			{
				/* Saute le premier guillemet si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->enregistre_pos_mot();
					this->pousse_caractere();
					this->avance();
				}
				else {
					this->avance();
					this->enregistre_pos_mot();
				}

				while (!this->fini()) {
					if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
						break;
					}

					this->pousse_caractere();
					this->avance();
				}

				/* Saute le dernier guillemet si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->pousse_caractere();
				}

				this->avance();

				this->pousse_mot(id_morceau::CHAINE_LITTERALE);
				break;
			}
			case '\'':
			{
				/* Saute la première apostrophe si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->enregistre_pos_mot();
					this->pousse_caractere();
					this->avance();
				}
				else {
					this->avance();
					this->enregistre_pos_mot();
				}

				if (this->caractere_courant() == '\\') {
					this->pousse_caractere();
					this->avance();
				}

				this->pousse_caractere();
				this->avance();				

				/* Saute la dernière apostrophe si nécessaire. */
				if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
					this->pousse_caractere();
				}

				if (this->caractere_courant() != '\'') {
					lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
				}

				this->avance();
				this->pousse_mot(id_morceau::CARACTERE);
				break;
			}
			case '#':
			{
				if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
					this->enregistre_pos_mot();
				}

				/* ignore commentaire */
				while (this->caractere_courant() != '\n') {
					this->avance();
					this->pousse_caractere();
				}

				if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
					this->pousse_mot(id_morceau::COMMENTAIRE);
				}

				/* Lorsqu'on inclus pas les commentaires, il faut ignorer les
				 * caractères poussées. */
				m_taille_mot_courant = 0;

				break;
			}
			default:
			{
				this->pousse_caractere();
				this->pousse_mot(idc);
				this->avance();
				break;
			}
		}
	}
	else if (m_taille_mot_courant == 0 && lng::est_nombre_decimal(this->caractere_courant())) {
		this->enregistre_pos_mot();

		using denombreuse = lng::decoupeuse_nombre<id_morceau>;
		id_morceau id_nombre;

		/* NOTE : on utilise une variable temporaire pour stocker le compte au
		 * lieu d'utiliser m_taille_mot_courant directement, car
		 * m_taille_mot_courant est remis à 0 dans pousse_mot() donc on ne peut
		 * l'utiliser en paramètre de avance() (ce qui causerait une boucle
		 * infinie. */
		auto const compte = denombreuse::extrait_nombre(m_debut, m_fin, id_nombre);
		m_taille_mot_courant = static_cast<long>(compte);

		this->pousse_mot(id_nombre);
		this->avance(static_cast<int>(compte));
	}
	else {
		if (m_taille_mot_courant == 0) {
			this->enregistre_pos_mot();
		}

		this->pousse_caractere();
		this->avance();
	}
}

void decoupeuse_texte::pousse_caractere(int n)
{
	m_taille_mot_courant += n;
}

void decoupeuse_texte::pousse_mot(id_morceau identifiant)
{
	m_module->morceaux.pousse({ mot_courant(), identifiant, static_cast<int>(m_module->id) });
	m_taille_mot_courant = 0;
}

void decoupeuse_texte::enregistre_pos_mot()
{
	m_pos_mot = m_position_ligne;
	m_debut_mot = m_debut;
}

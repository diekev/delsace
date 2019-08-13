
#include "gestionnaire_fichier.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"

/**
 * À FAIRE : considère avoir un cache par poignée pour stocker des données, par
 * exemple une archive Alembic ouverte.
 */

PoigneeFichier::PoigneeFichier(dls::chaine const &chemin)
	: m_chemin(chemin)
{}

void PoigneeFichier::lecture(type_fonction_lecture &&fonction, const std::any &donnees)
{
	m_mutex.lock();
	{
		std::ifstream is;
		is.open(m_chemin.c_str());
		fonction(is, donnees);
		is.close();
	}
	m_mutex.unlock();
}

void PoigneeFichier::lecture_chemin(type_fonction_lecture_chemin &&fonction, const std::any &donnees)
{
	m_mutex.lock();
	{
		fonction(m_chemin.c_str(), donnees);
	}
	m_mutex.unlock();
}

void PoigneeFichier::ecriture(type_fonction_ecriture &&fonction, const std::any &donnees)
{
	m_mutex.lock();
	{
		std::ofstream os;
		os.open(m_chemin.c_str());
		fonction(os, donnees);
		os.close();
	}
	m_mutex.unlock();
}

void PoigneeFichier::ecriture_chemin(type_fonction_lecture_chemin &&fonction, const std::any &donnees)
{
	m_mutex.lock();
	{
		fonction(m_chemin.c_str(), donnees);
	}
	m_mutex.unlock();
}

GestionnaireFichier::~GestionnaireFichier()
{
	for (auto paire : m_table) {
		memoire::deloge("PoigneeFichier", paire.second);
	}
}

PoigneeFichier *GestionnaireFichier::poignee_fichier(const dls::chaine &chemin)
{
	auto poignee = static_cast<PoigneeFichier *>(nullptr);

	m_mutex.lock();
	{
		auto iter = m_table.trouve(chemin);

		if (iter != m_table.fin()) {
			poignee = iter->second;
		}

		poignee = memoire::loge<PoigneeFichier>("PoigneeFichier", chemin);

		m_table.insere({chemin, poignee});
	}
	m_mutex.unlock();

	return poignee;
}

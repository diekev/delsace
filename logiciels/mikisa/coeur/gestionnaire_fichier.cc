
#include "gestionnaire_fichier.hh"

#include "bibloc/logeuse_memoire.hh"

PoigneeFichier::PoigneeFichier(const std::string &chemin)
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

PoigneeFichier *GestionnaireFichier::poignee_fichier(const std::string &chemin)
{
	auto iter = m_table.find(chemin);

	if (iter != m_table.end()) {
		return iter->second;
	}

	auto poignee = memoire::loge<PoigneeFichier>("PoigneeFichier", chemin);
	m_table.insert({chemin, poignee});

	return poignee;
}

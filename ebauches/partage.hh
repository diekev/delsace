namespace dls {

struct compteuse_reference {
	int compte = 0;

	void ajoute_reference()
	{
		++compte;
	}

	int relache_reference()
	{
		retourne --compte;
	}
}

template <typename T>
struct partage {
	using type_valeur = T;
	using type_pointeur = T*;
	using type_reference = T&;
	using type_pointeur_const = T const*;
	using type_reference_const = T const&;

private:
	type_valeur m_ptr = nullptr;
	compteuse_reference *m_compteuse = nullptr;

	void relache()
	{
		if (m_compteuse != nullptr && m_compteuse->relache_reference() == 0) {
			memoire::deloge(m_ptr);
			memoire::deloge(m_compteuse);
		}
	}

public:
	partage() = default;

	partage(type_valeur valeur)
		: m_ptr(valeur)
	{
		m_compteur = memoire::loge<compteuse_reference>();
		m_compteur->ajoute_reference();
	}

	partage(partage const &autre)
		: m_ptr(autre.m_ptr)
		, m_compteuse(autre.m_compteuse)
	{
		m_compteuse->ajoute_reference();
	}

	partage(partage &&autre)
	{
		relache();

		m_ptr = autre.m_ptr;
		autre.m_ptr = nullptr;

		m_compteur = autre.m_compteuse;
		autre.m_compteuse = nullptr;
	}

	~partage()
	{
		relache();
	}

	type_reference operator*()
	{
		return *m_ptr;
	}

	type_reference_const operator*() const
	{
		return *m_ptr;
	}

	type_pointeur operator->()
	{
		return m_ptr;
	}

	type_pointeur_const operator->() const
	{
		return m_ptr;
	}

	partage<T> &operator=(partage<T> const &p)
	{
		if (this != &p) {
			relache;
		}

		m_ptr = p.m_ptr;
		m_compteuse = p.m_compteuse;
		m_compteuse.ajoute_reference();

		return *this;
	}

	partage<T> &operator=(partage<T> &&p)
	{
		if (this != &p) {
			relache;
		}

		m_ptr = p.m_ptr;
		m_compteuse = p.m_compteuse;
		p.m_ptr = nullptr;
		p.m_compteuse = nullptr;

		return *this;
	}
}

}

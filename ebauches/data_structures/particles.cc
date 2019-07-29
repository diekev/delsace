#include <cstdlib>
#include <vector>
#include <queue>

class Particle {
	float m_pos[3];
	float m_radius;
	bool m_escaped;

//	size_t m_id;

public:
	Particle() = default;
	~Particle() = default;

#if 0
	void id(size_t id)
	{
		this->m_id = id;
	}

	size_t id() const
	{
		return m_id;
	}
#endif

	void position(float x, float y, float z)
	{
		m_pos[0] = x;
		m_pos[1] = y;
		m_pos[2] = z;
	}

	void getPosition(float &x, float &y, float &z) const
	{
		x = m_pos[0];
		y = m_pos[1];
		z = m_pos[2];
	}

	void radius(float rad)
	{
		this->m_radius = rad;
	}

	float radius() const
	{
		return m_radius;
	}

	bool hasEscaped() const
	{
		return m_escaped;
	}

	void setEscaped(bool escaped)
	{
		m_escaped = escaped;
	}
};

class SemiCompactParticle {
	unsigned short m_pos[3];
	unsigned short m_radius; // also stores escape property

public:
	SemiCompactParticle() = default;
	~SemiCompactParticle() = default;

	void position(float x, float y, float z)
	{
		m_pos[0] = x;
		m_pos[1] = y;
		m_pos[2] = z;
	}

	void getPosition(float &x, float &y, float &z) const
	{
		x = m_pos[0];
		y = m_pos[1];
		z = m_pos[2];
	}

	void radius(float rad)
	{
		this->m_radius = std::ceil((rad + 0.5f) * 65534);
	}

	float radius() const
	{
		return static_cast<float>(m_radius & 65534) / 65534.0f - 0.5f;
	}

	bool hasEscaped() const
	{
		return (m_radius & ~1) != 0;
	}

	void setEscaped(bool escaped)
	{
		if (escaped) {
			m_radius |= escaped;
		}
		else {
			m_radius &= 65534;
		}
	}
};

class CompactParticle {
	unsigned char m_pos[3];
	unsigned char m_radius; // also stores escape property

public:
	CompactParticle() = default;
	~CompactParticle() = default;

	void position(float x, float y, float z)
	{
		m_pos[0] = std::floor(255.0f * (x + 1.0f) / 3.0f);
		m_pos[1] = std::floor(255.0f * (y + 1.0f) / 3.0f);
		m_pos[2] = std::floor(255.0f * (z + 1.0f) / 3.0f);
	}

	void getPosition(float &x, float &y, float &z) const
	{
		x = static_cast<float>(3 * m_pos[0]) / 255.0f - 1.0f;
		y = static_cast<float>(3 * m_pos[1]) / 255.0f - 1.0f;
		z = static_cast<float>(3 * m_pos[2]) / 255.0f - 1.0f;
	}

	void radius(float rad)
	{
		this->m_radius = std::ceil((rad + 0.5f) * 254);
	}

	float radius() const
	{
		return static_cast<float>(m_radius & 254) / 254.0f - 0.5f;
	}

	bool hasEscaped() const
	{
		return (m_radius & ~1) != 0;
	}

	void setEscaped(bool escaped)
	{
		if (escaped) {
			m_radius |= escaped;
		}
		else {
			m_radius &= 254;
		}
	}
};

class ParticleField {
	using GarbageList = std::priority_queue<size_t>;
	using ParticleVec = std::vector<Particle *>;

	ParticleVec m_particles;
	GarbageList m_garbage_list;

	class IDHandler {
		GarbageList *m_garbage_list;
		ParticleVec *m_particles;

	public:
		IDHandler(ParticleVec &particles, GarbageList &list)
		    : m_garbage_list(&list)
		    , m_particles(&particles)
		{}

		~IDHandler() = default;

		size_t getID()
		{
			if (m_garbage_list->empty()) {
				return m_particles->size();
			}

			const auto id = m_garbage_list->top();
			m_garbage_list->pop();
			return id;
		}

		void reserveID(size_t id)
		{
			m_garbage_list->push(id);
		}
	};

	IDHandler m_idhandler;

public:
	ParticleField() = default;

	~ParticleField()
	{
		for (auto &particle : m_particles) {
			delete particle;
		}
	}

	void createParticle()
	{
		const auto id = m_idhandler.getID();
		if (id < m_particles.size()) {
			//			Particle *p = m_particles[id];
		}
		else {
			Particle *p = new Particle();
			m_particles.push_back(p);
		}
	}

	void removeParticle(Particle *p)
	{
		m_idhandler.reserveID(p->id());
	}
};

class ParticleContainer {
	// volume structure properties
	float m_voxel_size = 0.1f;

public:
	void addParticle(const glm::ivec3 &cell, Particle *p);

	void moveParticle(const glm::ivec3 &from, const glm::vec3 &to);

	void removeParticle(const glm::ivec3 &cell, const int &id);

	void clearParticles(const glm::ivec3 &cell);

	size_t particleCount(const glm::ivec3 &cell) const;

	size_t particleCount() const;

	void clear();

	void save(const std::string &filename);
	void load(const std::string &filename);

	void particleLocalToWorld();
	void particleWorldToLocal();
};

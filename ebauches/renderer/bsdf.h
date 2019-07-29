#include <glm/glm.hpp>

class BSDF {
public:
	BSDF();
	virtual ~BSDF();

	void evaluate(glm::vec3 &dir, glm::vec3 &nor, float weight, float cos_angle, glm::vec3 &col);
	void calculatePDFW(glm::vec3 &dir, glm::vec3 &nor, float probability);
	void sample(glm::vec3 &dir, glm::vec3 &nor, int rng, glm::vec3 &r_dir, float &weight, float fwd_prob, float &cos_angle);

	/**
	 * Return true if the BSDF's pdf is a Dirac delta function, false otherwise.
	 */
	bool isDelta();

	/**
	 * Probability of ending a ray path at this BSDF.
	 *
	 * @param dir direction
	 * @param nor normal
	 */
	float continuationProbablity(const glm::vec3 &dir, const glm::vec3 &nor);
};

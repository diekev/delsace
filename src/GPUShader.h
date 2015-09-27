#include <GL/glew.h>
#include <map>
#include <string>

class GPUShader {
	enum ShadeType {
		VERTEX_SHADER,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER
	};

	GLuint m_program;
	int m_total_shaders;
	GLuint m_shaders[3];
	std::map<std::string, GLuint> m_attrib_list;
	std::map<std::string, GLuint> m_uniform_loc_list;

public:
	GPUShader();
	~GPUShader();

	void loadFromString(GLenum whichShader, const std::string &source);
	void loadFromFile(GLenum whichShader, const std::string &source);
	void createAndLinkProgram();
	void use();
	void unUse();
	void addAttribute(const std::string &attribute);
	void addUniform(const std::string &uniform);

	GLuint operator[](const std::string &attribute);
	GLuint operator()(const std::string &uniform);
};

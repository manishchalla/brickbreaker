

#include "glazy_program.h"


namespace glazy {


	namespace uniform {
		template<> void SetUniformV(GLint loc, size_t c, GLfloat* ptr) { glUniform1fv(loc, c, ptr); }
		template<> void SetUniformV(GLint loc, size_t c, glm::vec2* ptr) { glUniform2fv(loc, c, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::vec3* ptr) { glUniform3fv(loc, c, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::vec4* ptr) { glUniform4fv(loc, c, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, GLint* ptr) { glUniform1iv(loc, c, ptr); }
		template<> void SetUniformV(GLint loc, size_t c, glm::ivec2* ptr) { glUniform2iv(loc, c, reinterpret_cast<GLint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::ivec3* ptr) { glUniform3iv(loc, c, reinterpret_cast<GLint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::ivec4* ptr) { glUniform4iv(loc, c, reinterpret_cast<GLint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, GLuint* ptr) { glUniform1uiv(loc, c, ptr); }
		template<> void SetUniformV(GLint loc, size_t c, glm::uvec2* ptr) { glUniform2uiv(loc, c, reinterpret_cast<GLuint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::uvec3* ptr) { glUniform3uiv(loc, c, reinterpret_cast<GLuint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::uvec4* ptr) { glUniform4uiv(loc, c, reinterpret_cast<GLuint*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat2* ptr) { glUniformMatrix2fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat3* ptr) { glUniformMatrix3fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat4* ptr) { glUniformMatrix4fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat2x3* ptr) { glUniformMatrix2x3fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat3x2* ptr) { glUniformMatrix3x2fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat2x4* ptr) { glUniformMatrix2x4fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat4x2* ptr) { glUniformMatrix4x2fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat3x4* ptr) { glUniformMatrix3x4fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
		template<> void SetUniformV(GLint loc, size_t c, glm::mat4x3* ptr) { glUniformMatrix4x3fv(loc, c, false, reinterpret_cast<GLfloat*>(ptr)); }
	}


	GPUAccessor::GPUAccessor(GPUProgram& prog, std::string name)
		: prog(prog)
		, name(name)
	{}


	void GPUProgram::check_linking() {
		safety::entry_guard("GPUProgram::check_linking");
		GLint status;
		glGetProgramiv(id, GL_LINK_STATUS, &status);
		if (!status) {
			GLint log_length;
			glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
			std::string log_text((size_t)log_length, '\0');
			GLsizei written;
			glGetProgramInfoLog(id, log_length, &written, reinterpret_cast<GLchar*>((char*)log_text.data()));
			std::string message = "GPU Program linking failed. Error log:\n\n\"\"\"\n";
			message += log_text.c_str();
			message += "\n\"\"\"\n";
			throw std::runtime_error(message);
		}
		safety::exit_guard("GPUProgram::check_linking");
	}



	void GPUProgram::attach(GLuint shader_id) {
		safety::entry_guard("GPUProgram::attach");
		glAttachShader(id,shader_id);
		safety::exit_guard("GPUProgram::attach");
	}

	GPUProgram::GPUProgram(
		Shader<GL_VERTEX_SHADER>          vertex,
		Shader<GL_TESS_CONTROL_SHADER>    tess_cont,
		Shader<GL_TESS_EVALUATION_SHADER> tess_eval,
		Shader<GL_GEOMETRY_SHADER>        geometry,
		Shader<GL_FRAGMENT_SHADER>        fragment
	) {
		safety::entry_guard("GPUProgram::GPUProgram(vertex,tess_cont,tess_eval,geometry,fragment)");
		if (vertex.is_empty()) {
			throw std::runtime_error("Render pipeline must have a vertex shader.");
		}
		else if (fragment.is_empty()) {
			throw std::runtime_error("Render pipeline must have a fragment shader.");
		}
		else if (tess_cont.is_empty() != tess_eval.is_empty()) {
			throw std::runtime_error("Render pipeline must have both tesselation shader stages or neither.");
		}
		id = glCreateProgram();
		if (!id) {
			throw std::runtime_error("Failed to allocate id for GPU program");
		}
		attach(vertex);
		if (!tess_cont.is_empty()) {
			attach(tess_cont);
			attach(tess_eval);
		}
		if (!geometry.is_empty()) {
			attach(geometry);
		}
		attach(fragment);
		glLinkProgram(id);
		check_linking();
		safety::exit_guard("GPUProgram::GPUProgram(vertex,tess_cont,tess_eval,geometry,fragment)");
	}

	GPUProgram::operator GLuint() {
		return id;
	}

	GPUAccessor GPUProgram::operator[](std::string name) {
		return GPUAccessor(*this, name);
	}

	GPUProgram::BindGuard::BindGuard(GPUProgram& program) : program(program) {
		safety::entry_guard("GPUProgram::BindGuard::bind");
		if (bind_stack.empty() || (bind_stack.top() != program.id)) {
			glUseProgram(program.id);
		}
		bind_stack.push(program.id);
		safety::exit_guard("GPUProgram::BindGuard::bind");
	}

	GPUProgram::BindGuard::~BindGuard() {
		if (bind_stack.empty()) {
			std::cerr << "ERROR: BindGuard has run more destructors than constructors! "
						 "Do not manually invoke the destructor of a BindGuard!\n";
			return;
		}
		bind_stack.pop();
		if ((!bind_stack.empty()) && (bind_stack.top() != program.id)) {
			glUseProgram(bind_stack.top());
		}
	}


	GLint GPUProgram::attribute_index (std::string name) {
		safety::entry_guard("GPUProgram::attribute_index");
		GLint index = glGetAttribLocation(id, name.c_str());
		if (index < 0) {
			std::string message = "Attribute '";
			message += name + "' does not exist.";
			throw std::runtime_error(message);
		}
		safety::exit_guard("GPUProgram::attribute_index");
		return index;
	}


	std::stack<GLuint> GPUProgram::BindGuard::bind_stack;

}




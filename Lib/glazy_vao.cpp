
#include "glazy_vao.h"

namespace glazy {



	VAO::VAO() {
		safety::entry_guard("VAO::VAO()");
		id = 0;
		glGenVertexArrays(1, &id);
		if (id == 0) {
			throw std::runtime_error("Failed to allocate id for VAO.");
		}
		safety::exit_guard("VAO::VAO()");
	}


	VAO::VAO(VAO&& other)
		: id(other.id)
	{
		safety::entry_guard("VAO::VAO(VAO&&)");
		other.id = 0;
		safety::exit_guard("VAO::VAO(VAO&&)");
	}

	VAO::~VAO() {
		if (glIsVertexArray(id)) {
			glDeleteVertexArrays(1, &id);
		}
	}

	VAO::operator GLuint() const {
		return id;
	}

	VAO::BindGuard::BindGuard(VAO& vao) : vao(vao) {
		safety::entry_guard("VAO::BindGuard::BindGuard");
		if (bind_stack.empty() || (bind_stack.top() != vao.id)) {
			glBindVertexArray(vao.id);
		}
		bind_stack.push(vao.id);
		safety::exit_guard("VAO::BindGuard::BindGuard");
	}

	VAO::BindGuard::~BindGuard() {
		if (bind_stack.empty()) {
			std::cerr << "ERROR: BindGuard has run more destructors than constructors! "
				"Do not manually invoke the destructor of a BindGuard!\n";
			return;
		}
		bind_stack.pop();
		if ((!bind_stack.empty()) && (bind_stack.top() != vao.id)) {
			glBindVertexArray(bind_stack.top());
		}
	}

	void VAO::bind() {
		safety::entry_guard("VAO::bind");
		glBindVertexArray(id);
		safety::exit_guard("VAO::bind");
	}


	VAO::Attribute::Attribute(VAO& vao, size_t index)
		: vao(vao)
		, index(index)
	{}


	VAO::Attribute& VAO::Attribute::enable() {
		safety::entry_guard("AttributeAccessor::enable");
		{
			BindGuard bind_guard(vao);
			glEnableVertexAttribArray(index);
		}
		safety::exit_guard("AttributeAccessor::enable");
		return *this;
	}

	VAO::Attribute& VAO::Attribute::disable() {
		safety::entry_guard("AttributeAccessor::disable");
		{
			BindGuard bind_guard(vao);
			glDisableVertexAttribArray(index);
		}
		safety::exit_guard("AttributeAccessor::disable");
		return *this;
	}


	VAO::Attribute VAO::operator[] (size_t index) {
		return Attribute(*this, index);
	}

	std::stack<GLuint> VAO::BindGuard::bind_stack;


	SharedVAO::SharedVAO() : vao(new VAO) {}

	SharedVAO::operator GLuint() {
		return (*vao);
	}

	SharedVAO::operator VAO& () {
		return (*vao);
	}

	VAO::Attribute SharedVAO::operator[] (size_t index) {
		return (*vao)[index];
	}



	VAO::BindGuard::BindGuard(SharedVAO& svao)
		: vao(svao)
	{
		safety::entry_guard("VAO::BindGuard::BindGuard");
		if (bind_stack.empty() || (bind_stack.top() != vao.id)) {
			glBindVertexArray(vao.id);
		}
		bind_stack.push(vao.id);
		safety::exit_guard("VAO::BindGuard::BindGuard");
	}

}

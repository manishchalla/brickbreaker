#include "glazy_buffer.h"

namespace glazy {
	namespace compat {

		// macOS does not support OpenGL 4.5, and so does not have compat::named_buffer_data.
		// To fix this, we have the janky solution of rapidly swapping the original
		// GL_ARRAY_BUFFER out, binding our buffer briefly to perform the operation,
		// and then swapping the original back in. This is less inefficient, but it
		// is the price that must be paid for ergonomic data movement without
		// introducing nasty, hard-to-debug side effects.

		void named_buffer_data(GLuint id, size_t size, void* data, GLenum usage) {
			safety::entry_guard("compat::named_buffer_data()");
			GLuint old;
			// Why would OpenGL base all object identifiers around unsigned integers,
			// but not provide a get routine for unsigned integers?
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&old));
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glBufferData(GL_ARRAY_BUFFER, size, data, usage);
			glBindBuffer(GL_ARRAY_BUFFER, old);
			safety::exit_guard("compat::named_buffer_data()");
		}

		void* map_named_buffer(GLuint id, GLenum access) {
			safety::entry_guard("compat::map_named_buffer()");
			void* result;
			GLuint old;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&old));
			glBindBuffer(GL_ARRAY_BUFFER, id);
			result = glMapBuffer(GL_ARRAY_BUFFER, access);
			glBindBuffer(GL_ARRAY_BUFFER, old);
			safety::exit_guard("compat::map_named_buffer()");
			return result;
		}

		void unmap_named_buffer(GLuint id) {
			safety::entry_guard("compat::unmap_named_buffer()");
			GLuint old;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&old));
			glBindBuffer(GL_ARRAY_BUFFER, id);
			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer(GL_ARRAY_BUFFER, old);
			safety::exit_guard("compat::unmap_named_buffer()");
		}

	}
}


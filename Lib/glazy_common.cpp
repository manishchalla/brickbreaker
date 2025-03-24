

#include "glazy_common.h"


namespace glazy {

	namespace flags {
		extern bool debug = true;
	}

	namespace file {
		size_t file_size(std::ifstream& file) {
			file.seekg(0, std::ios::end);
			size_t result = static_cast<size_t>(file.tellg());
			file.seekg(0, std::ios::beg);
			return result;
		}

		std::string read_file_to_string(std::string file_path) {
			std::ifstream file(file_path);
			size_t size = file_size(file);
			std::string result;
			result.resize(size);
			result.assign((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());
			return result;
		}
	}


	namespace safety {
		void auto_throw(std::string context) {
			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				std::string message = context + " - OpenGL Error Codes: ";
				bool first = true;
				while (err != GL_NO_ERROR) {
					if (!first) {
						message += ", ";
					}
					message += std::to_string(err);
					err = glGetError();
				}
				throw std::runtime_error(message);
			}
		}

		void entry_guard(std::string fn_name) {
			if (true) { //flags::debug) {
				std::string context = "Encountered OpenGL error from before '";
				context += fn_name + "' was called.";
				auto_throw(context);
			}
		}

		void exit_guard(std::string fn_name) {
			if (flags::debug) {
				std::string context = "Encountered OpenGL error during evaluation of '";
				context += fn_name + "'.";
				auto_throw(context);
			}
		}
	}



	namespace context {

		void error_callback(int error_code, char const* desc) {
			throw std::runtime_error(desc);
		}

		GLFWwindow* setup(glm::ivec2 position, glm::ivec2 dimensions, const char* title, std::vector<WindowHint> hints) {
			if (glfwInit() == GLFW_FALSE) {
				throw std::runtime_error("Failed to initialize GLFW.");
			}

			glfwSetErrorCallback(error_callback);

			std::vector<WindowHint> platform_hints;

			#ifdef __APPLE__
			// Hints derived from GLFW documentation: https://www.glfw.org/faq.html#41__how_do_i_create_an_opengl_30_context
			// Versions updated to 4.1, since that seemed to work for students in previous terms
			platform_hints = {
				{GLFW_CONTEXT_VERSION_MAJOR, 4},
				{GLFW_CONTEXT_VERSION_MINOR, 1},
				{GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE},
				{GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE}
			};
			#else
			platform_hints = {
				{GLFW_CONTEXT_VERSION_MAJOR, 4},
				{GLFW_CONTEXT_VERSION_MINOR, 1},
				{GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE},
			};
			#endif

			for (auto&& hint : platform_hints) {
				glfwWindowHint(hint.hint, hint.value);
			}

			for (auto&& hint : hints) {
				glfwWindowHint(hint.hint, hint.value);
			}

			GLFWwindow* result = glfwCreateWindow(dimensions.x, dimensions.y, title, nullptr, nullptr);

			if (result == nullptr) {
				glfwTerminate();
				throw std::runtime_error("Failed to create window.");
			}
			
			glfwSetWindowPos(result, position.x, position.y);

			glfwMakeContextCurrent(result);
			glfwSwapInterval(1);
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				throw std::runtime_error("Failed to initialize OpenGL context.");
			}
			return result;
		}
	}

}



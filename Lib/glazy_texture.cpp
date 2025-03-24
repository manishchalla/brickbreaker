
#include "glazy_texture.h"

namespace glazy {

	Texture::Texture(std::vector<Texture::RGB8> data, size_t width, size_t height, bool mipmap) {
		safety::entry_guard("Texture::Texture");
		id = 0;
		glGenTextures(1, &id);
		if (id == 0) {
			throw std::runtime_error("Failed to allocate texture id.");
		}
		glBindTexture(GL_TEXTURE_2D, id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
		if (mipmap) {
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		safety::exit_guard("Texture::Texture");
	}

	Texture::~Texture() {
		glDeleteTextures(1, &id);
	}

	Texture::operator GLuint() {
		return id;
	}


	Sampler::Sampler() {
		glGenSamplers(1, &id);
	}

	Sampler::~Sampler() {
		glDeleteSamplers(1, &id);
	}

}

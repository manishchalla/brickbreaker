#include "shape.h"


#include <cmath>


namespace glazy {

	namespace shape {

		float const M_PI = 3.1415926f;


		std::vector<glm::vec2> uv_grid(size_t w, size_t h) {
			std::vector<glm::vec2> result;
			result.resize(w*h*3*2);
			float x_delta = 1.0f / w;
			float y_delta = 1.0f / h;
			for (size_t x = 0; x < w; x++) {
				for (size_t y = 0; y < h; y++) {
					size_t offset = (x * w + y) * 3 * 2;
					result[offset + 0] = glm::vec2{ (x / (float)w), ((y+1) / (float)h) };
					result[offset + 1] = result[offset + 0] + glm::vec2{ x_delta,0.0f };
					result[offset + 2] = result[offset + 0] + glm::vec2{ x_delta,-y_delta };
					result[offset + 3] = result[offset + 0] + glm::vec2{ 0.0f, 0.0f };
					result[offset + 4] = result[offset + 0] + glm::vec2{ x_delta,-y_delta };
					result[offset + 5] = result[offset + 0] + glm::vec2{ 0.0f,-y_delta };
				}
			}
			return result;
		}


		std::vector<glm::vec3> quad() {
			std::vector<glm::vec3> result = {
				{-1,-1, 0}, { 1,-1,-1}, { 1, 1,-1},
				{-1,-1,-1}, { 1, 1,-1}, {-1, 1,-1}
			};
			return result;
		}


		std::vector<glm::vec3> box(float w, float h, float d) {
			std::vector<glm::vec3> result = {
				{0,0,0}, {1,0,0}, {1,1,0}, {0,0,0}, {1,1,0}, {0,1,0},
				{0,0,0}, {0,1,0}, {0,1,1}, {0,0,0}, {0,1,1}, {0,0,1},
				{0,0,0}, {0,0,1}, {1,0,1}, {0,0,0}, {1,0,1}, {1,0,0},

				{1,1,1}, {0,1,1}, {0,0,1}, {1,1,1}, {0,0,1}, {1,0,1},
				{1,1,1}, {1,0,1}, {1,0,0}, {1,1,1}, {1,0,0}, {1,1,0},
				{1,1,1}, {1,1,0}, {0,1,0}, {1,1,1}, {0,1,0}, {0,1,1},
			};
			return result;
		}


		std::vector<glm::vec3> sphere(size_t wedges, size_t layers, float radius) {
			std::vector<glm::vec3> result;
			float phi_step = (1.0f / layers) * M_PI;
			float theta_step = (1.0f / layers) * M_PI * 2.0f;
			// resize to number of points in the sphere model
			result.resize(wedges * layers * 3 * 2);
			for (size_t w = 0; w < wedges; w++) {
				for (size_t l = 0; l < layers; l++) {
					// offset of triangles for this patch
					size_t offset = (layers * w + l) * 3 * 2;
					float upper_phi = l * phi_step;
					float lower_phi = upper_phi + phi_step;
					float left_theta = w * theta_step;
					float right_theta = left_theta + theta_step;

					glm::vec3 bot_left = glm::vec3{
						sin(lower_phi) * cos(left_theta),
						cos(lower_phi),
						sin(lower_phi) * sin(left_theta)
					} *radius;

					glm::vec3 bot_right = glm::vec3{
						sin(lower_phi) * cos(right_theta),
						cos(lower_phi),
						sin(lower_phi) * sin(right_theta)
					} *radius;

					glm::vec3 top_left = glm::vec3{
						sin(upper_phi) * cos(left_theta),
						cos(upper_phi),
						sin(upper_phi) * sin(left_theta)
					} *radius;

					glm::vec3 top_right = glm::vec3{
						sin(upper_phi) * cos(right_theta),
						cos(upper_phi),
						sin(upper_phi) * sin(right_theta)
					} *radius;

					result[offset + 0] = bot_left;
					result[offset + 1] = bot_right;
					result[offset + 2] = top_right;
					result[offset + 3] = bot_left;
					result[offset + 4] = top_right;
					result[offset + 5] = top_left;
				}
			}
			return result;
		}

	}
}


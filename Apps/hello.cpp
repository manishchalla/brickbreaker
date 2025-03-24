#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>

void renderText(const char* text, float x, float y, FT_Face face, unsigned int fontSize) {
    // Set the font size
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    // Set up OpenGL to render text
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

    // Set text color
    glColor3f(1.0f, 1.0f, 1.0f); // White color

    // Set text position
    glRasterPos2f(x, y);

    // Render each character
    for (const char* c = text; *c; ++c) {
        // Load glyph
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER)) {
            continue;
        }

        // Render glyph
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );

        // Set character size and position
        float xPos = x + face->glyph->bitmap_left;
        float yPos = y - (face->glyph->bitmap.rows - face->glyph->bitmap_top);

        // Set character dimensions
        float w = face->glyph->bitmap.width;
        float h = face->glyph->bitmap.rows;

        // Render quad
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(xPos, yPos);
        glTexCoord2f(0, 1);
        glVertex2f(xPos, yPos + h);
        glTexCoord2f(1, 1);
        glVertex2f(xPos + w, yPos + h);
        glTexCoord2f(1, 0);
        glVertex2f(xPos + w, yPos);
        glEnd();

        // Advance cursor position
        x += (face->glyph->advance.x >> 6);
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Text Rendering", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize FreeType library
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Error: Could not initialize FreeType library" << std::endl;
        return -1;
    }

    // Load font face
    FT_Face face;
    if (FT_New_Face(ft, "/path/to/your/font.ttf", 0, &face)) {
        std::cerr << "Error: Failed to load font" << std::endl;
        return -1;
    }

    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Render text
        renderText("Hello, World!", 100, 100, face, 24);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Clean up GLFW
    glfwTerminate();

    return 0;
}

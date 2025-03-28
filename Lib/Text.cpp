// Text.cpp: display text using FreeType
// Copyright (c) 2024 Jules Bloomenthal, all rights reserved. Commercial use requires license.
// see tutorial: https://learnopengl.com/In-Practice/Text-Rendering

#include <glad.h>
#include "Draw.h"
#include "GLXtras.h"
#include "Letters.h"
#include "Text.h"
#include <map>
#include <stdio.h>
#include <string.h>

// if FreeType not linked, comment next line:
// #define FREETYPE_OK

#define FormatString(buffer, maxBufferSize, format) {  \
	(buffer)[0] = 0;                                   \
	if (format) {                                      \
		va_list ap;                                    \
		va_start(ap, format);                          \
		vsnprintf(buffer, maxBufferSize, format, ap);  \
		va_end(ap);                                    \
	}                                                  \
}

#ifndef FREETYPE_OK
float scaleAdj = 1;//.5f;
void Text(int x, int y, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	Letters(x, y, text, color, scaleAdj*scale);
}
void Text(vec3 p, mat4 m, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	vec2 s = ScreenPoint(p, m);
	Letters((int) s.x, (int) s.y, text, color, scaleAdj*scale);
}
void Text(float x, float y, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	Letters((int) x, (int) y, text, color, scaleAdj*scale);
}
void RenderText(const char *text, float x, float y, vec3 color, float scale, mat4 view) {
	vec2 s = ScreenPoint(vec3(x, y, 0), view);
	Letters((int) s.x, (int) s.y, text, color, scaleAdj*scale);
}
float TextWidth(float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	int nchars = (int) strlen(text);
	return scale*nchars;
}
int TextWidth(int scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	return (int) TextWidth((float) scale, text);
}
CharacterSet *SetFont(const char *fontName, int charRes, int pixelRes, bool forceInit) { return NULL; };
#else

#include <ft2build.h>
#include <freetype/freetype.h>

using std::string;

static GLuint textShaderProgram = 0, textVertexBuffer = 0;

CharacterSet *currentFont = NULL;

// font repository
struct Compare { bool operator() (const string &a, const string &b) const { return a.compare(b) > 0; }};
typedef std::map<string, CharacterSet, Compare> CharacterSets;
CharacterSets fonts;

void SetCharacterSet(CharacterSet &cs, const char *fontName, int charRes, int pixelRes) {
	cs.charRes = charRes;
	// init FreeType, load font face
	FT_Library ft;
	FT_Face face;
	if (FT_Init_FreeType(&ft) ||
		FT_New_Face(ft, fontName, 0, &face) ||
		FT_Set_Char_Size(face, 0, charRes*64, pixelRes, pixelRes) || // set character point size
		FT_Set_Pixel_Sizes(face, 0, pixelRes))  {                    // set pixel res
			printf("problem with FreeType, font load, or font face\n");
			return;
	}
	// load glyphs
	FT_GlyphSlot g = face->glyph;
	for (GLubyte c = 0; c < 128; c++) {
		FT_Error r = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (r)
			printf("FreeType: failed to load Glyph\n");
		else {
			// generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, g->bitmap.width, g->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
			// texture options
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// store character
			cs.characters[c] = Character(texture, int2(g->bitmap.width, g->bitmap.rows), int2(g->bitmap_left, g->bitmap_top), (GLuint) g->advance.x);
		}
	}
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

CharacterSet *SetFont(const char *fontName, int charRes, int pixelRes, bool forceInit) {
	CharacterSets::iterator it = fonts.find(fontName);
	if (it == fonts.end() || forceInit) {
		CharacterSet cs;
		SetCharacterSet(cs, fontName, charRes, pixelRes);
		fonts[string(fontName)] = cs;
		it = fonts.find(fontName);
	}
	currentFont = &it->second;
	return currentFont;
}

static const char *textVertexShader = "\
	#version 130                                    \n\
	in vec4 point;							        \n\
	out vec2 vUv;                                   \n\
	uniform mat4 view;                              \n\
	void main() {                                   \n\
		gl_Position = view*vec4(point.xy, 0, 1);    \n\
		vUv = point.zw;                             \n\
	}                                               \n";

static const char *textPixelShader = "\
	#version 130                                    \n\
	in vec2 vUv;                                    \n\
	out vec4 pColor;                                \n\
	uniform sampler2D textureImage;                 \n\
	uniform vec3 color;                             \n\
	void main() {                                   \n\
		float a = texture(textureImage, vUv).r;     \n\
		pColor = vec4(color, a);                    \n\
	}                                               \n";

void RenderText(const char *text, float x, float y, vec3 color, float scale, mat4 view, bool vertical) {
	if (!currentFont) {
		SetFont("C:/Fonts/OpenSans/OpenSans-Regular.ttf", 64, 100);  // unsure exact effect of charRes, pixelRes
		return;
	}
	if (!textShaderProgram)
		textShaderProgram = LinkProgramViaCode(&textVertexShader, &textPixelShader);
	glUseProgram(textShaderProgram);
	scale /= (float) currentFont->charRes;
	// create quad vertex buffer and build characters
	if (textVertexBuffer == 0)
		glGenBuffers(1, &textVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*4, NULL, GL_DYNAMIC_DRAW);
	VertexAttribPointer(textShaderProgram, "point", 4, 4*sizeof(float), 0);
	SetUniform(textShaderProgram, "view", view);
	SetUniform(textShaderProgram, "color", color);
	// SetUniform(textShaderProgram, "textureImage", (int) textureID); // not needed? (defaults to 0?)
	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_ARRAY_BUFFER, textVertexBuffer);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (const char *c = text; *c; c++) {
		Character ch = currentFont->characters[(int)*c];
		float xpos = x+ch.bearing.i1*scale, ypos = y-(ch.gSize.i2-ch.bearing.i2)*scale;
		float w = ch.gSize.i1*scale, h = ch.gSize.i2*scale;
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// update vertex memory
#ifndef __APPLE__
		float vertices[][4] = {{xpos, ypos+h, 0, 0}, {xpos+w, ypos+h, 1, 0}, {xpos+w, ypos, 1, 1}, {xpos, ypos, 0, 1}};
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_QUADS, 0, 4);     // render glyph texture with quad
#else
		float vertices[][4] = {{xpos, ypos+h, 0, 0}, {xpos+w, ypos+h, 1, 0}, {xpos+w, ypos, 1, 1},
							   {xpos, ypos+h, 0, 0}, {xpos+w, ypos, 1, 1},   {xpos, ypos, 0, 1}};
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glDrawArrays(GL_TRIANGLES, 0, 6); // render glyph texture with triangles
#endif
		if (vertical)
			y -= 24*scale;
		else
			x += (ch.advance >> 6)*scale;     // advance character position in terms of 1/64 pixel
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

float TextWidth(float scale, const char *format, ...) {
	float w = 0;
	char text[500];
	FormatString(text, 500, format);
	if (!currentFont)
		SetFont("C:/Fonts/OpenSans/OpenSans-Regular.ttf", 15, 30);  // unsure exact affect of charRes, pixelRes
			// name, charRes, pixelRes
	if (currentFont != NULL) {
		scale /= (float) currentFont->charRes;
		for (const char* c = text; *c; c++) {
			Character ch = currentFont->characters[(int)*c];
			w += (ch.advance >> 6) * scale;
		}
	}
//  printf("wid of %s = %4.3f\n", text, w);
	return w;
}

int TextWidth(int scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	return (int) TextWidth((float) scale, text);
}

void Text(vec3 p, mat4 m, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	vec2 s = ScreenPoint(p, m);
	RenderText(text, s.x, s.y, color, scale, ScreenMode());
}

void Text(int x, int y, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	RenderText(text, (float) x, (float) y, color, scale, ScreenMode());
}

void Text(float x, float y, vec3 color, float scale, const char *format, ...) {
	char text[500];
	FormatString(text, 500, format);
	RenderText(text, x, y, color, scale, ScreenMode());
}

#endif

std::string Nice(float f) {
	static char buf[100];
	sprintf(buf, fabs(f) > 1? "%.1f" : fabs(f) > .1f? "%.2f" : "%.3f", f);
	while (strlen(buf) > 1 && buf[strlen(buf)-1] == '0')
		buf[strlen(buf)-1] = 0;
	if (strlen(buf) > 1 && buf[strlen(buf)-1] == '.')
		buf[strlen(buf)-1] = 0;
	char *t = buf;
	if (strlen(buf) > 2 && buf[0] == '-' && buf[1] == '0') {
		buf[1] = '-';
		t = buf+1;
	}
	if (strlen(buf) > 2 && buf[0] == '0')
		t = buf+1;
	if (strlen(buf) == 2 && buf[0] == '-' && buf[1] == '0')
		t = buf+1;
	return std::string(t);
}

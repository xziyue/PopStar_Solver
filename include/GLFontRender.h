#pragma once

#include "GLDef.h"
#include "GLProgram.h"
#include "TfMatrix.h"

#pragma region ShaderSources
const string fontRenderVertexShaderSource{ R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;
uniform mat4 projection;
void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
} 
)" };

const string fontRenderFragmentShaderSource{ R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec3 textColor;
void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}
)" };
#pragma endregion

struct GlyphInfo {
	GLuint glTextureID;
	Vec2i size;
	Vec2i bearing;
	Integer advance;
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct FontInfo {
	float foregroundSizeScale = 1.0f;
	float backgroundSizeScale = 1.0f;
	Vec3f foregroundColor{ 1.0f, 1.0f, 1.0f };
	Vec3f backgroundColor{ 0.0f, 0.0f, 0.0f };
	float lineSpread = 1.0f;
	Vec2f windowSize{ 0.0f, 0.0f };
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

// make sure to use this class after creating OpenGL context
class GLFontRender {
public:
	GLFontRender() { this->__Initialize(); }

	~GLFontRender() { this->__Close(); }

	GLFontRender(const GLFontRender &other) = delete;
	GLFontRender &operator = (const GLFontRender &other) = delete;

	void LoadTypeface(const char* filename, Integer pxHeight);

	bool HasTypeface() const;

	void LoadASCIIGlyph();

	FT_UInt GetGlyphIndex(FT_ULong charCode) const;

	bool HasGlyph(FT_UInt glyphIndex) const;

	void LoadGlyph(FT_UInt glyphIndex);

	void RenderPlainText(const wstring &text, const Vec2f &pos, const FontInfo &info);

	void PreloadGlyph(const wstring &text);

	const GlyphInfo &GetGlyph(FT_UInt glyphIndex) const;

private:
	void __DeleteGlyphs();

	void __DeleteTypeface();

	void __Initialize();

	void __Close();

private:
	FT_Library __freetypeLib;

	FT_Face __freetypeTypeface;

	unique_ptr<GLProgramManager> __glProgMan;

	map<FT_UInt, GlyphInfo> __glyphs;

	bool __hasTypeface = false;

	GLuint __myVAO, __myVBO;

	float __baseLinespread = 0.0f;
};
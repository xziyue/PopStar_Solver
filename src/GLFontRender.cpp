#include "PCH.H"
#include "GLFontRender.h"

void GLFontRender::LoadTypeface(const char * filename, Integer pxHeight)
{
	if (this->HasTypeface()) {
		throw runtime_error{ "Typeface Already Loaded" };
	}

	// creates a new face
	auto error = FT_New_Face(this->__freetypeLib, filename, 0, &this->__freetypeTypeface);
	if (error) {
		cerr << "Error: unable to load font typeface " << filename << " \n";
		throw runtime_error{ "Can not Load Font Typeface" };
	}
	// sets the height of characters
	FT_Set_Pixel_Sizes(this->__freetypeTypeface, 0, pxHeight);

	this->__hasTypeface = true;

	// load letter h for linespread calculation
	auto glyphIndex = this->GetGlyphIndex('h');
	this->LoadGlyph(glyphIndex);
	const auto &glyph = this->GetGlyph(glyphIndex);
	this->__baseLinespread = (float)glyph.size[1];
}

bool GLFontRender::HasTypeface() const
{
	return this->__hasTypeface;
}

void GLFontRender::LoadASCIIGlyph()
{
	for (auto i = 0; i < 128; ++i) {
		this->LoadGlyph(this->GetGlyphIndex(i));
	}
}

FT_UInt GLFontRender::GetGlyphIndex(FT_ULong charCode) const
{
	assert(this->HasTypeface());
	return FT_Get_Char_Index(this->__freetypeTypeface, charCode);
}

bool GLFontRender::HasGlyph(FT_UInt glyphIndex) const
{
	assert(this->HasTypeface());
	return this->__glyphs.find(glyphIndex) != this->__glyphs.end();
}

void GLFontRender::LoadGlyph(FT_UInt glyphIndex)
{
	assert(this->HasTypeface());
	if (this->HasGlyph(glyphIndex)) {
		return;
	}

	// disables byte-alignment in OpenGL
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	auto error = FT_Load_Glyph(this->__freetypeTypeface, glyphIndex, FT_LOAD_RENDER);
	if (error) {
		cerr << "Error: Unable to load glyph (glyph index " << glyphIndex << ")\n";
		throw runtime_error{ "Can not Load Glyph" };
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
		this->__freetypeTypeface->glyph->bitmap.width,
		this->__freetypeTypeface->glyph->bitmap.rows,
		0, GL_RED, GL_UNSIGNED_BYTE,
		this->__freetypeTypeface->glyph->bitmap.buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);


	GlyphInfo glyphInfo;
	glyphInfo.glTextureID = textureID;
	glyphInfo.size = Vec2i{ this->__freetypeTypeface->glyph->bitmap.width,
		this->__freetypeTypeface->glyph->bitmap.rows, };
	glyphInfo.bearing = Vec2i{ this->__freetypeTypeface->glyph->bitmap_left,
		this->__freetypeTypeface->glyph->bitmap_top };
	glyphInfo.advance = this->__freetypeTypeface->glyph->advance.x;

	this->__glyphs.insert(make_pair(glyphIndex, glyphInfo));
}

void GLFontRender::RenderPlainText(const wstring & text, const Vec2f & pos, const FontInfo & info)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Mat4f projMatrix = GetOrthoMatrix(0.0, info.windowSize[0], 0.0, info.windowSize[1]);
	this->__glProgMan->UseProgram();
	auto projLocation = glGetUniformLocation(this->__glProgMan->GetProgram(), "projection");
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, projMatrix.data());

	auto textColorLocation = glGetUniformLocation(this->__glProgMan->GetProgram(), "textColor");
	glUniform3fv(textColorLocation, 1, info.foregroundColor.data());

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(this->__myVAO);
	
	float xPosFix = 0.0f;
	float yPosFix = 0.0f;
	auto initialPos = pos;

	for (auto iter = text.begin(); iter != text.end(); ++iter) {
		if (*iter == '\n') {
			xPosFix = 0.0f;
			yPosFix -= this->__baseLinespread * info.lineSpread;
			continue;
		}

		auto glyphIndex = this->GetGlyphIndex(*iter);

		if (!this->HasGlyph(glyphIndex)) {
			// load this glyph
			this->LoadGlyph(glyphIndex);
		}

		const auto &glyph = this->GetGlyph(glyphIndex);

		auto xPos = pos[0] + glyph.bearing[0] * info.foregroundSizeScale + xPosFix;
		auto yPos = pos[1] - (glyph.size[1] - glyph.bearing[1])* info.foregroundSizeScale + yPosFix;

		auto w = glyph.size[0] * info.foregroundSizeScale;
		auto h = glyph.size[1] * info.foregroundSizeScale;

		GLfloat buffer[6][4] = {
		{ xPos,     yPos + h,   0.0, 0.0 },
		{ xPos,     yPos,       0.0, 1.0 },
		{ xPos + w, yPos,       1.0, 1.0 },
		{ xPos,     yPos + h,   0.0, 0.0 },
		{ xPos + w, yPos,       1.0, 1.0 },
		{ xPos + w, yPos + h,   1.0, 0.0 } };


		glBindTexture(GL_TEXTURE_2D, glyph.glTextureID);
		glBindBuffer(GL_ARRAY_BUFFER, this->__myVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(buffer), buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		xPosFix += (glyph.advance >> 6) * info.foregroundSizeScale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLFontRender::PreloadGlyph(const wstring & text)
{
	for (auto iter = text.begin(); iter != text.end(); ++iter) {
		auto glyphIndex = this->GetGlyphIndex(*iter);
		if (this->HasGlyph(glyphIndex)) {
			continue;
		}

		this->LoadGlyph(glyphIndex);
	}
}

const GlyphInfo & GLFontRender::GetGlyph(FT_UInt glyphIndex) const
{
	return this->__glyphs.at(glyphIndex);
}

void GLFontRender::__DeleteTypeface()
{
	if (this->HasTypeface()) {
		FT_Done_Face(this->__freetypeTypeface);
	}

	this->__DeleteGlyphs();

	this->__hasTypeface = false;
}

void GLFontRender::__DeleteGlyphs()
{
	for (auto iter = this->__glyphs.begin(); iter != this->__glyphs.end(); ++iter) {
		glDeleteTextures(1, &(iter->second.glTextureID));
	}
	this->__glyphs.clear();
}

void GLFontRender::__Initialize()
{
	// initialize FreeType
	auto error = FT_Init_FreeType(&this->__freetypeLib);
	if (error) {
		cerr << "Error: unable to initialize FreeType library\n";
		throw runtime_error{ "Can not Initialize FreeType" };
	}

	// initialize smart pointers
	this->__glProgMan = make_unique<GLProgramManager>();

	// compile shader and link program
	this->__glProgMan->CompileShader(GL_VERTEX_SHADER, fontRenderVertexShaderSource.c_str());
	this->__glProgMan->CompileShader(GL_FRAGMENT_SHADER, fontRenderFragmentShaderSource.c_str());
	this->__glProgMan->LinkProgram();

	// initialize GL buffer
	glGenVertexArrays(1, &this->__myVAO);
	glGenBuffers(1, &this->__myVBO);
	glBindVertexArray(this->__myVAO);
	glBindBuffer(GL_ARRAY_BUFFER, this->__myVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
}

void GLFontRender::__Close()
{
	this->__DeleteTypeface();

	FT_Done_FreeType(this->__freetypeLib);

	glDeleteVertexArrays(1, &this->__myVAO);
	glDeleteBuffers(1, &this->__myVBO);
}

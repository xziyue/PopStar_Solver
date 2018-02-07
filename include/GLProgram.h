#pragma once

#include "GLDef.h"

// make sure to use this class after creating OpenGL context
class GLProgramManager {
public:
	GLProgramManager() {}

	GLProgramManager(const GLProgramManager &other) = delete;
	GLProgramManager &operator = (const GLProgramManager &other) = delete;

	// compiles a shader and store it in the map
	void CompileShader(GLEnumType shaderType, const char* shaderSource);

	void DeleteShader(GLEnumType shaderType);

	void DeleteAllShader();

	bool HasShader(GLEnumType shaderType) const;

	bool HasProgram() const;

	// creates OpenGL program, delete all shaders
	void LinkProgram();

	void DeleteProgram();

	GLuint GetProgram() const;

	void UseProgram() const;

private:
	map<GLEnumType, GLuint> __myShader;

	bool __hasProgram = false;

	GLuint __myProgram = static_cast<GLuint>(-1);

private:

	static constexpr const Integer __glInfoLogLength = 2048;
};
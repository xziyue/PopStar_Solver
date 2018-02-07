#include "PCH.H"
#include "GLProgram.h"

void GLProgramManager::CompileShader(GLEnumType shaderType, const char * shaderSource)
{
	// please make sure the shaderType is valid, otherwise
	// weird errors might occur
	GLuint newShader = glCreateShader(shaderType);

	glShaderSource(newShader, 1, &shaderSource, NULL);
	glCompileShader(newShader);

	GLint success;
	
	glGetShaderiv(newShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		auto charBuffer = make_unique<char[]>(this->__glInfoLogLength);
		glGetShaderInfoLog(newShader, this->__glInfoLogLength, NULL, charBuffer.get());
		cerr << "An error occured compiling OpenGL shader.\n";
		cerr << "Shader type: " << hex << shaderType << "\n";
		cerr << "Detail:\n" << charBuffer.get() << "\n";
		throw runtime_error{ "Shader Compilation Failure" };
	}

	if (this->HasShader(shaderType)) {
		this->DeleteShader(shaderType);
	}

	this->__myShader[shaderType] = newShader;

}

void GLProgramManager::DeleteShader(GLEnumType shaderType)
{
	if (!this->HasShader(shaderType)) {
		return;
	}
	
	glDeleteShader(this->__myShader[shaderType]);
	this->__myShader.erase(shaderType);
}

void GLProgramManager::DeleteAllShader()
{
	// creates a copy of all shaders
	vector<GLEnumType> allShaders;

	for (auto iter = this->__myShader.begin(); iter != this->__myShader.end(); ++iter) {
		allShaders.push_back(iter->first);
	}

	for (const auto &item : allShaders) {
		this->DeleteShader(item);
	}
}

bool GLProgramManager::HasShader(GLEnumType shaderType) const
{
	return this->__myShader.find(shaderType) != this->__myShader.end();
}

bool GLProgramManager::HasProgram() const
{
	return this->__hasProgram;
}

void GLProgramManager::LinkProgram()
{
	if (this->HasProgram()) {
		this->DeleteProgram();
	}

	this->__myProgram = glCreateProgram();

	for (auto iter = this->__myShader.begin(); iter != this->__myShader.end(); ++iter) {
		glAttachShader(this->__myProgram, iter->second);
	}
	glLinkProgram(this->__myProgram);

	GLint success;
	glGetProgramiv(this->__myProgram, GL_LINK_STATUS, &success);
	if (!success) {
		auto charBuffer = make_unique<char[]>(this->__glInfoLogLength);
		glGetProgramInfoLog(this->__myProgram, this->__glInfoLogLength, NULL, charBuffer.get());
		cerr << "An error occured linking OpenGL program.\n";
		cerr << "Detail:\n" << charBuffer.get() << "\n";
		throw runtime_error{ "Program Linking Failure" };
	}
	this->__hasProgram = true;
	

	this->DeleteAllShader();
}

void GLProgramManager::DeleteProgram()
{
	if (!this->HasProgram()) {
		return;
	}

	glDeleteProgram(this->__myProgram);
	this->__hasProgram = false;
}

GLuint GLProgramManager::GetProgram() const
{
	assert(this->HasProgram());
	return this->__myProgram;
}

void GLProgramManager::UseProgram() const
{
	assert(this->HasProgram());
	glUseProgram(this->__myProgram);
}

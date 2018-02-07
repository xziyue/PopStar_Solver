#pragma once

#include "CoreDef.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#ifdef NDEBUG
#pragma comment(lib, "freetype")
#else
#pragma comment(lib, "freetyped")
#endif

// tells GLEW to use static library
#define GLEW_STATIC
#include "GL/glew.h"
#ifdef NDEBUG
#pragma comment(lib, "glew32s")
#else
#pragma comment(lib, "glew32sd")
#endif


#include "GLFW/glfw3.h"
#ifdef NDEBUG
#pragma comment(lib, "glfw3")
#else
#pragma comment(lib, "glfw3d")
#endif

// extract the type of GL enums
using GLEnumType = decltype(GL_FRAGMENT_SHADER);
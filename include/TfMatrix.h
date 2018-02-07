#pragma once

#include "CoreDef.h"

template<typename T>
inline T RadToDeg(T rad) {
	return rad * (T)180 / M_PI;
}

template<typename T>
inline T DegToRad(T deg) {
	return deg / (T)180 * M_PI;
}

Mat4f GetOrthoMatrix(float left, float right, float bottom, float top);

Mat4f GetTranslationMatrix(float x, float y, float z);

Mat4f GetScaleMatrix(float x, float y, float z);

Mat4f GetRotationMatrix(float rad, const Vec3f &axis);
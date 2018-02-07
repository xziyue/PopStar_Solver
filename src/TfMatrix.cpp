#include "PCH.H"
#include "TfMatrix.h"

Mat4f GetOrthoMatrix(float left, float right, float bottom, float top)
{
	auto width = right - left;
	auto height = top - bottom;
	Mat4f result = GetTranslationMatrix(-1.0f, -1.0f, 0.0f) * 
		GetScaleMatrix(2.0f / width, 2.0f / height, 1.0) * 
		GetTranslationMatrix(-left, -bottom, 0.0f);
	return result;
}

Mat4f GetTranslationMatrix(float x, float y, float z)
{
	Mat4f result = Mat4f::Identity();
	result(0, 3) = x;
	result(1, 3) = y;
	result(2, 3) = z;
	return result;
}

Mat4f GetScaleMatrix(float x, float y, float z)
{
	Mat4f result = Mat4f::Identity();
	result(0, 0) = x;
	result(1, 1) = y;
	result(2, 2) = z;
	return result;
}

Mat4f GetRotationMatrix(float rad, const Vec3f & axis)
{
	assert(axis.isUnitary());
	Mat4f result = Mat4f::Identity();
	Mat3f _3dresult = Eigen::AngleAxisf(rad, axis).toRotationMatrix();
	result.block<3, 3>(0, 0) = _3dresult.block<3, 3>(0, 0);
	return result;
}



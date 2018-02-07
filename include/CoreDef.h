#pragma once

#include <cassert>
#include <array>
#include <vector>
#include <set>
#include <iostream>
#include <memory>
#include <limits>
#include <string>
#include <map>
#include <queue>
#include <iomanip>
#include <random>
#include <thread>
#include <future>
#include <chrono>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Eigen/Dense"
#include "Eigen/StdVector"

using namespace std;

using Integer = int32_t;
using Byte = unsigned int;

// using Eigen types and std::vector
template<typename DataType>
using AVector = std::vector<DataType, Eigen::aligned_allocator<DataType>>;

template<typename DataType>
using ASet = std::set<DataType, less<DataType>, Eigen::aligned_allocator<DataType>>;

template<typename DataType>
using ADeque = std::deque <DataType, Eigen::aligned_allocator<DataType>>;

template<typename DataType>
using AQueue = std::queue<DataType, ADeque<DataType>>;

struct PopStarProperties {
	// defines number of board rows and columns
	static constexpr const Integer popStarBoardRows = 10;
	static constexpr const Integer popStarBoardCols = 10;

	static_assert(popStarBoardRows > 0, "invalid number of rows");
	static_assert(popStarBoardCols > 0, "invalid number of cols");

	// defines number of colors
	static constexpr const Integer popStarNumOfColors = 4;

	static_assert(popStarNumOfColors > 0, "invalid number of colors");
	static constexpr const Integer popStarTotalNumOfBoardState = popStarNumOfColors + 1;
	// guard overflow
	static_assert(popStarTotalNumOfBoardState <= numeric_limits<Byte>::max(), "invalid number of total  board states");

	// returns the number of points canceling stars
	static Integer GetPointCancelStar(Integer nStar);
	// returns the number of points when one round ends
	static Integer GetPointRemainStar(Integer nStar);
};

struct BoardProperties {
	static constexpr const Byte boardLow = 0;
	static constexpr const Byte boardUp = PopStarProperties::popStarNumOfColors - 1;
	static constexpr const Byte boardEmpty = PopStarProperties::popStarNumOfColors;
};

template<typename DataType, Integer Row, Integer Col>
using ColMajorMatrix = Eigen::Matrix<DataType, Row, Col, Eigen::ColMajor>;

// the Board class uses unsigned int to save memory
using Board = ColMajorMatrix<Byte, PopStarProperties::popStarBoardRows, PopStarProperties::popStarBoardCols>;
using BoardCol = ColMajorMatrix<Byte, PopStarProperties::popStarBoardRows, 1>;
using Vec2i = ColMajorMatrix<Integer, 2, 1>;
using Vec2f = ColMajorMatrix<float, 2, 1>;
using Vec3f = ColMajorMatrix<float, 3, 1>;
using Vec4f = ColMajorMatrix<float, 4, 1>;
using Mat3f = ColMajorMatrix<float, 3, 3>;
using Mat4f = ColMajorMatrix<float, 4, 4>;

template<>
struct less<Vec2i> {
	bool operator()(const Vec2i &left, const Vec2i &right) const {
		if (left[0] > right[0]) {
			return false;
		}
		else if (left[0] == right[0]) {
			return left[1] < right[1];
		}
		else {
			return true;
		}
	}
};

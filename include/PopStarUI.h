#pragma once

#include "CoreDef.h"
#include "GLDef.h"
#include "GLProgram.h"
#include "GLFontRender.h"
#include "BoardOp.h"
#include "ScoreOp.h"

const Integer blockLength = 40;
const Integer blockColDistance = 6;
const Integer blockRowDistance = 6;
const Integer blockOutlineWidth = 2;
const Integer upperReservedHeight = 150;

// randomly generate a window board with choices more than this
const Integer choiceLowerBound = 17;

const Integer windowWidth = blockLength * PopStarProperties::popStarBoardCols + 
	blockColDistance * (PopStarProperties::popStarBoardCols + 1);
const Integer windowHeight = upperReservedHeight + blockLength * PopStarProperties::popStarBoardRows +
	blockRowDistance * (PopStarProperties::popStarBoardRows);

// defines colors of corresponding stars

const Vec3f backgroundColor{ 0.1f, 0.1f, 0.1f };
const Vec3f outlineColor{ 1.0f, 1.0f, 1.0f };
const vector<Vec3f> colorDefinition{
	Vec3f{ 1.0f, 0.0f, 1.0f }, // purple
	Vec3f{ 0.0f, 0.0f, 1.0f }, // blue
	Vec3f{ 1.0f, 1.0f, 0.0f}, // yellow
	Vec3f{ 1.0f, 0.0f, 0.0f }, // red
	backgroundColor
};

const string fontFilename{ "C:/windows/fonts/consola.ttf" };

const float minimumOutlineChangeInverval = 0.08f;

const chrono::milliseconds multiThreadWaitTime{10};

extern Integer currentScore;

extern bool acceptMouseInput;

extern bool acceptKeyInput;

struct CursorPosInfo {
	bool isCursorOnStar = false;
	Vec2i closestStar;
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

extern GLFWwindow *currentWindow;

// this board will be drawn in the window
extern unique_ptr<Board> windowBoard;

// font renderer
extern unique_ptr<GLFontRender> fontRender;

// shader manager
extern unique_ptr<GLProgramManager> glProgMan;

// specify wheter the star needs to be outlined
extern unique_ptr<ASet<Vec2i>> outlinedStar;

extern unique_ptr<GroupType> windowBoardGroupInfo;

CursorPosInfo GetCursurPosInfo(const Vec2f &cursorPos);

void RestartGame();

// updates windowBoardGroupInfo
// alternatively, one can choose to manually update the variable
// without calling this function
void UpdateWindowBoardGroupInfo();

// Initializes OpenGL context, creates current window
void InitializeDemo();

void RunDemo();
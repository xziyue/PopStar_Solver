#include "PCH.H"
#include "PopStarUI.h"

// definitions of global variables
GLFWwindow *currentWindow = nullptr;
unique_ptr<Board> windowBoard;
unique_ptr<GLFontRender> fontRender;
unique_ptr<GLProgramManager> glProgMan;
unique_ptr<ASet<Vec2i>> outlinedStar;
unique_ptr<GroupType> windowBoardGroupInfo;
unique_ptr<CursorPosInfo> cursorPosInfo;
unique_ptr<default_random_engine> randomEngine;
unique_ptr<Board> backupBoard;

FontInfo fontInfo;
wstringstream sstr;

Integer currentScore = 0;
Integer savedScore = 0;
bool acceptMouseInput = true;
bool acceptKeyInput = true;
bool gameOver = false;

using ColorStorageType = array<array<Vec3f, PopStarProperties::popStarBoardCols>, PopStarProperties::popStarBoardRows>;

// OpenGL VAO and VBO objects
// 0 - background, 1 - foreground
GLuint triVBO[2], triVAO[2];
ColorStorageType backgroundColorStorage, foregroundColorStorage;
// specifies number of triangles in VAO
Integer numOfTriangles{ 0 };

// this variable tracks the time of outline changes
// to avoid subtle change of color
float outlineTimeStamp = 0.0f;

#pragma region ShaderSource
// shader sources
const string triangleVertexShaderSource{ R"(
#version 330 core
layout(location = 0) in vec2 pos;
uniform mat4 projection;
void main(){
	gl_Position = projection * vec4(pos, 0.0, 1.0);
}
)"
};

const string triangleFragmentShaderSource{ R"(
#version 330 core
uniform vec3 inColor;
out vec4 color;
void main(){
	color = vec4(inColor, 1.0);
	//color = vec4(1.0, 1.0, 1.0, 1.0);
}
)"
};
#pragma endregion

// declaration of DrawContent function below
void DrawContent();

#pragma region HelperFunc
// helper functions
CursorPosInfo GetCursurPosInfo(const Vec2f & cursorPos)
{
	CursorPosInfo result;
	result.isCursorOnStar = false;

	float estX = (cursorPos[1] - (float)blockRowDistance) / (float)(blockRowDistance + blockLength);
	float estY = (cursorPos[0] - (float)blockColDistance) / (float)(blockColDistance + blockLength);

	Integer closestX = (Integer)floor(estX);
	Integer closestY = (Integer)floor(estY);

	result.closestStar = Vec2i{ PopStarProperties::popStarBoardRows - 1 - closestX, closestY };

	if (!IsPositionValid(result.closestStar)) {
		result.isCursorOnStar = false;
	}
	else {
		Integer botLeftWidth = blockColDistance + closestY * blockColDistance + closestY * blockLength;
		Integer botLeftHeight = blockRowDistance + closestX * blockRowDistance + closestX * blockLength;
		Vec2f botLeft{ (float)botLeftWidth, (float)botLeftHeight };
		Vec2f topRight = botLeft + Vec2f{ (float)blockLength, (float)blockLength };

		if (cursorPos[0] >= botLeft[0] && cursorPos[1] >= botLeft[1]) {
			if (cursorPos[0] <= topRight[0] && cursorPos[1] <= topRight[1]) {
				result.isCursorOnStar = true;
			}
		}
	}

	return result;
}

void FillWindowBoardWithRandom() {
	uniform_int_distribution<Byte> dist{ BoardProperties::boardLow, BoardProperties::boardUp };
	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			(*windowBoard)(i, j) = dist(*randomEngine);
		}
	}
}

void SaveGame() {
	savedScore = currentScore;
	*backupBoard = *windowBoard;
}

void RestoreGame() {
	currentScore = savedScore;
	*windowBoard = *backupBoard;
	UpdateWindowBoardGroupInfo();

	if (windowBoardGroupInfo->size() == 0) {
		acceptMouseInput = false;
		gameOver = true;
	}
	else {
		acceptMouseInput = true;
		gameOver = false;
	}
}

void RestartGame() {
	currentScore = 0;

	while (true) {
		FillWindowBoardWithRandom();
		UpdateWindowBoardGroupInfo();
		if (windowBoardGroupInfo->size() > choiceLowerBound) {
			break;
		}
	}
	acceptMouseInput = true;
	gameOver = false;

	SaveGame();
}

void SelectGroup(const Vec2i &star) {
	bool foundGroup = false;
	for (const auto &subGroup : *windowBoardGroupInfo) {
		if (subGroup.find(star) != subGroup.end()) {
			for (auto iter = subGroup.begin(); iter != subGroup.end(); ++iter) {
				outlinedStar->insert(*iter);
			}
			foundGroup = true;
			break;
		}
	}

	if (!foundGroup) {
		if ((*windowBoard)(star[0], star[1]) != BoardProperties::boardEmpty) {
			outlinedStar->insert(star);
		}
	}
}

void EliminateGroup(const Vec2i &star) {
	// check whether we can find a group
	for (const auto &subGroup : *windowBoardGroupInfo) {
		if (subGroup.find(star) != subGroup.end()) {

			// set all blocks of this group to empty
			for (auto iter = subGroup.begin(); iter != subGroup.end(); ++iter) {
				(*windowBoard)((*iter)[0], (*iter)[1]) = BoardProperties::boardEmpty;
			}

			Integer deltaScore = PopStarProperties::GetPointCancelStar(subGroup.size());
			currentScore += deltaScore;

			(*windowBoard) = GetNextBoard(*windowBoard);
			outlinedStar->clear();

			UpdateWindowBoardGroupInfo();
			break;
		}
	}

	// check if it is game over
	if (windowBoardGroupInfo->empty()) {
		outlinedStar->clear();
		gameOver = true;
		acceptMouseInput = false;
		auto colorStarLeft = CountColorStars(*windowBoard);
		currentScore += PopStarProperties::GetPointRemainStar(colorStarLeft);
	}
}

void PlayAnimation(const ProblemState &state) {
	*windowBoard = *state.initialBoard;
	UpdateWindowBoardGroupInfo();

	auto KeepDrawingFor = [](chrono::milliseconds duration) {
		auto startTime = chrono::system_clock::now();

		while (true) {
			auto nowTime = chrono::system_clock::now();
			auto timeGap = chrono::duration_cast<chrono::milliseconds>(nowTime - startTime);

			if (nowTime - startTime >= duration) {
				break;
			}

			DrawContent();
			this_thread::sleep_for(multiThreadWaitTime);
		}
	};

	for (const auto &step : *state.stepTaken) {
		SelectGroup(step);
		KeepDrawingFor(chrono::milliseconds{ 500 });
		EliminateGroup(step);
		KeepDrawingFor(chrono::milliseconds{ 500 });
	}
}

void PlaySolution() {
	SaveGame();

	acceptKeyInput = false;
	acceptMouseInput = false;
	outlinedStar->clear();

	ProblemState state = move(GetProblemState(*backupBoard));

	auto GetBestSolution = [](bool &foundSolution, ProblemState &&state) {
		return GetNaiveBestSolution(foundSolution, move(state));
	};

	bool foundSolution = false;

	auto theFuture = async(std::launch::async, GetBestSolution, ref(foundSolution), move(state));

	future_status status = theFuture.wait_for(multiThreadWaitTime);

	while (status != future_status::ready) {
		DrawContent();
		status = theFuture.wait_for(multiThreadWaitTime);
	}

	ProblemState result = move(theFuture.get());

	if (!foundSolution) {
		cerr << "unalbe to find a solution.\n";
		return;
	}

	RestoreGame();
	acceptMouseInput = false;

	PlayAnimation(result);
	acceptKeyInput = true;
}
#pragma endregion

#pragma region OpenGLCallback
// OpenGL callback functions
void CursorCallback(GLFWwindow *window, double xPos, double yPos) {
	// convert the pos to our coordinate system
	Vec2f actualPos{ (float)xPos, (float)windowHeight - (float)yPos };

	// updates the cursorPosInfo(global) when cursor moves
	*cursorPosInfo = move(GetCursurPosInfo(actualPos));

	// check whether we have pass the desired interval
	float currentTime = (float)glfwGetTime();
	if (currentTime - outlineTimeStamp < minimumOutlineChangeInverval) {
		return;
	}

	if (acceptMouseInput) {

		outlinedStar->clear();

		if (cursorPosInfo->isCursorOnStar) {
			SelectGroup(cursorPosInfo->closestStar);
		}
		outlineTimeStamp = currentTime;
	}


}

void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
	if (acceptMouseInput) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			if (cursorPosInfo->isCursorOnStar) {
				EliminateGroup(cursorPosInfo->closestStar);
			}
		}
	}
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (acceptKeyInput) {
		if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
			RestartGame();
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
			PlaySolution();
		}
		if (key == GLFW_KEY_T && action == GLFW_RELEASE) {
			RestoreGame();
		}
	}
}

#pragma endregion

#pragma region Initialization
// initializes OpenGL context, creates current window
void InitializeOpenGL()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	currentWindow = glfwCreateWindow(windowWidth, windowHeight, "PopStar Demo", NULL, NULL);
	glfwMakeContextCurrent(currentWindow);
	glViewport(0, 0, windowWidth, windowHeight);

	glewExperimental = GL_TRUE;
	glewInit();
}

// initializes VAO, VBO objects, color storage
void InitializeGLMem() {
	glGenBuffers(2, triVBO);
	glGenVertexArrays(2, triVAO);

	vector<float> foreground, background;
	numOfTriangles = PopStarProperties::popStarBoardRows * PopStarProperties::popStarBoardCols * 2;

	auto numOfFloat = numOfTriangles * 3 * 2;
	foreground.reserve(numOfFloat);
	background.reserve(numOfFloat);

	auto PushTriangle = [](const array<Vec2f, 3> &triangle, vector<float> &triArray) {
		for (Integer i = 0; i < 3; ++i) {
			for (Integer j = 0; j < 2; ++j) {
				triArray.push_back(triangle[i][j]);
			}
		}
	};

	Vec2f currentCorner{ (float)blockColDistance, (float)blockRowDistance };
	for (Integer i = PopStarProperties::popStarBoardRows - 1; i > -1; --i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			Vec2f innerBotLeft = currentCorner;
			Vec2f innerTopLeft = innerBotLeft + Vec2f{ 0.0f, (float)blockLength };
			Vec2f innerBotRight = innerBotLeft + Vec2f{ (float)blockLength, 0.0f };
			Vec2f innerTopRight = innerTopLeft + Vec2f{ (float)blockLength, 0.0f };

			Vec2f outerTopLeft = innerTopLeft + Vec2f{ (float)-blockOutlineWidth, (float)blockOutlineWidth };
			Vec2f outerTopRight = innerTopRight + Vec2f{ (float)blockOutlineWidth, (float)blockOutlineWidth };
			Vec2f outerBotLeft = innerBotLeft + Vec2f{ (float)-blockOutlineWidth, (float)-blockOutlineWidth };
			Vec2f outerBotRight = innerBotRight + Vec2f{ (float)blockOutlineWidth, (float)-blockOutlineWidth };

			array<Vec2f, 3> innerTri1{ innerTopLeft, innerBotLeft, innerBotRight };
			array<Vec2f, 3> innerTri2{ innerBotRight, innerTopRight, innerTopLeft };

			array<Vec2f, 3> outerTri1{ outerTopLeft, outerBotLeft, outerBotRight };
			array<Vec2f, 3> outerTri2{ outerBotRight, outerTopRight, outerTopLeft };

			PushTriangle(innerTri1, foreground);
			PushTriangle(innerTri2, foreground);
			PushTriangle(outerTri1, background);
			PushTriangle(outerTri2, background);

			currentCorner[0] += (float)blockLength + (float)blockColDistance;
		}
		currentCorner[0] = (float)blockColDistance;
		currentCorner[1] += (float)blockLength + (float)blockRowDistance;
	}

	// initialize background VAO
	glBindVertexArray(triVAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, triVBO[0]);
	// must call glBufferData when initializing VAO objects
	glBufferData(GL_ARRAY_BUFFER, background.size() * sizeof(float), background.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);
	glBindVertexArray(0);

	// initialize foreground VAO
	glBindVertexArray(triVAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, triVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, foreground.size() * sizeof(float), foreground.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

}

// initializes everything, get ready to call RunDemo()
void InitializeDemo()
{
	assert(blockOutlineWidth < blockRowDistance);

	InitializeOpenGL();
	InitializeGLMem();

	// initializes variables
	windowBoard = move(make_unique<Board>());
	fontRender = move(make_unique<GLFontRender>());
	glProgMan = move(make_unique<GLProgramManager>());
	outlinedStar = move(make_unique<ASet<Vec2i>>());
	windowBoardGroupInfo = move(make_unique<GroupType>());
	cursorPosInfo = move(make_unique<CursorPosInfo>());
	randomEngine = move(make_unique<default_random_engine>());
	backupBoard = move(make_unique<Board>());

	randomEngine->seed(chrono::system_clock::now().time_since_epoch().count() % numeric_limits<unsigned int>::max());

	windowBoard->fill(0);

	glProgMan->CompileShader(GL_VERTEX_SHADER, triangleVertexShaderSource.c_str());
	glProgMan->CompileShader(GL_FRAGMENT_SHADER, triangleFragmentShaderSource.c_str());
	glProgMan->LinkProgram();

	fontRender->LoadTypeface(fontFilename.c_str(), 25);
	fontRender->LoadASCIIGlyph();

	// set up callback functions
	glfwSetCursorPosCallback(currentWindow, &CursorCallback);
	glfwSetMouseButtonCallback(currentWindow, &MouseButtonCallback);
	glfwSetKeyCallback(currentWindow, &KeyCallback);

	outlineTimeStamp = (float)glfwGetTime();

	RestartGame();
}
#pragma endregion

#pragma region UpdateState
// update funtions

// update foreground and background color storage
void UpdateColor() {
	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			Vec2i currentStar{ i,j };

			// updates foreground
			foregroundColorStorage[i][j] = colorDefinition[(*windowBoard)(i, j)];

			// updates background
			if (outlinedStar->find(currentStar) != outlinedStar->end()) {
				backgroundColorStorage[i][j] = outlineColor;
			}
			else {
				backgroundColorStorage[i][j] = backgroundColor;
			}
		}
	}
}

void UpdateWindowBoardGroupInfo()
{
	*windowBoardGroupInfo = move(GetAllGroup(*windowBoard));
}

#pragma endregion

#pragma region DrawFunc
// drawing functions
void DrawBackground() {
	Integer triangleVertexCounter = 0;

	glProgMan->UseProgram();

	auto projectionLocation = glGetUniformLocation(glProgMan->GetProgram(), "projection");
	auto inColorLocation = glGetUniformLocation(glProgMan->GetProgram(), "inColor");
	Mat4f projectionMatrix = GetOrthoMatrix(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);

	// pass the projection matrix
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix.data());

	for (Integer i = PopStarProperties::popStarBoardRows - 1; i > -1; --i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			// pass the color vector
			glUniform3fv(inColorLocation, 1, backgroundColorStorage[i][j].data());

			glBindVertexArray(triVAO[0]);
			// draw triangles
			glDrawArrays(GL_TRIANGLES, triangleVertexCounter, 6);
			glBindVertexArray(0);
			triangleVertexCounter += 6;
		}
	}
}

void DrawForeground() {
	Integer triangleVertexCounter = 0;

	glProgMan->UseProgram();

	auto projectionLocation = glGetUniformLocation(glProgMan->GetProgram(), "projection");
	auto inColorLocation = glGetUniformLocation(glProgMan->GetProgram(), "inColor");
	Mat4f projectionMatrix = GetOrthoMatrix(0.0f, (float)windowWidth, 0.0f, (float)windowHeight);

	// pass the projection matrix
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix.data());

	for (Integer i = PopStarProperties::popStarBoardRows - 1; i > -1; --i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			// pass the color vector
			glUniform3fv(inColorLocation, 1, foregroundColorStorage[i][j].data());

			glBindVertexArray(triVAO[1]);
			// draw triangles
			glDrawArrays(GL_TRIANGLES, triangleVertexCounter, 6);
			glBindVertexArray(0);
			triangleVertexCounter += 6;
		}
	}
}

void DrawContent() {
	glfwPollEvents();
	glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	UpdateColor();
	DrawBackground();
	DrawForeground();

	Vec2i cursorPos = (cursorPosInfo->closestStar.cast<int>() + Vec2i{ 1,1 }).transpose();
	cursorPos[0] = max(cursorPos[0], 0);
	cursorPos[1] = max(cursorPos[1], 0);

	if (gameOver) {
		sstr.str(L"");
		sstr << "Game Over\n";
		sstr << "Final Score: " << currentScore << "\n";
		sstr << "Press 'R' to restart\n";
		sstr << "Press 'T' to load";
		fontRender->RenderPlainText(sstr.str(), Vec2f{ 20.0f, (float)windowHeight - 20.0f }, fontInfo);
	}
	else {
		sstr.str(L"");
		sstr << "Score: " << currentScore << "\n";
		sstr << "Press S to compute solution\n";
		sstr << "Press R to restart\n";
		sstr << "Press T to load";
		//sstr << "Valid Choices: " << windowBoardGroupInfo->size() << "\n";
		//sstr << "Score LB: " << GetScoreLowerBound(*windowBoard, *windowBoardGroupInfo) << "\n";
		//sstr << "Current Block: " << cursorPos[0] << ", " << cursorPos[1];
		fontRender->RenderPlainText(sstr.str(), Vec2f{ 20.0f, (float)windowHeight - 20.0f }, fontInfo);
	}

	glfwSwapBuffers(currentWindow);
}
#pragma endregion

void RunDemo()
{
	fontInfo.foregroundSizeScale = 1.0f;
	fontInfo.lineSpread = 1.5f;
	fontInfo.windowSize = Vec2f{ (float)windowWidth, (float)windowHeight };

	while (!glfwWindowShouldClose(currentWindow)) {
		
		DrawContent();
	}

	// exits
	glfwTerminate();
}

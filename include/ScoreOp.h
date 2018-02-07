#pragma once

#include "CoreDef.h"
#include "BoardOp.h"

// calculate the lower bound of score, given the current gruop information
Integer GetScoreLowerBound(const Board &board, const GroupType &group);

// a very loose upperbound that is not useful unless the number of group
// left is very small.
Integer GetScoreUpperBound(const Board &board);

// an upper bound that is slightly tighter than the upper bound function above
// but it is still very inaccurate
Integer GetScoreTighterUpperBound(const Board &board, const GroupType &group);

struct ProblemState {
	unique_ptr<Board> initialBoard;
	unique_ptr<Board> currentBoard;
	unique_ptr<GroupType> currentGroup;
	unique_ptr<AVector<Vec2i>> stepTaken;
	Integer currentScore = 0;
	Integer currentMetric = 0;

	/*
	ProblemState(const ProblemState &&other);

	ProblemState &operator = (const ProblemState &other);
	*/

	void CopyFrom(const ProblemState &other);

	void GainResource();
};

ProblemState GetProblemState(const Board &board);

vector<ProblemState> ExpandState(const ProblemState &state);

Integer GetNaiveStateMetric(const ProblemState &state);

// default value is 4
extern Integer naiveSearchWidth;

extern Integer naiveSearchWidthUp;

extern Integer naiveSearchWidthLow;

extern Integer naiveSearchSatisfactoryScore;

extern chrono::seconds naiveSearchTimeLowerBound;

extern chrono::seconds naiveSearchTimeUpperBound;

extern chrono::seconds naiveSearchGiveUpLimit;

// the width decreases as step increases
Integer GetNaiveSearchWidth(Integer currentStep);

ProblemState GetNaiveBestSolution(bool &foundSolution, ProblemState &&state);
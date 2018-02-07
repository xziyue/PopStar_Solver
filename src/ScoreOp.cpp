#include "PCH.H"
#include "ScoreOp.h"

Integer naiveSearchWidth = 2;

Integer naiveSearchWidthUp = 5;

Integer naiveSearchWidthLow = 3;

Integer naiveSearchSatisfactoryScore = 5500;

chrono::seconds naiveSearchTimeLowerBound{ 4 };

chrono::seconds naiveSearchTimeUpperBound{ 8 };

chrono::seconds naiveSearchGiveUpLimit{ 10 };

Integer GetScoreLowerBound(const Board &board, const GroupType & group)
{
	Integer result = 0;
	for (const auto &subGroup : group) {
		result += PopStarProperties::GetPointCancelStar(subGroup.size());
	}

	result += PopStarProperties::GetPointRemainStar(CountColorStars(board));
	return result;
}

Integer GetScoreUpperBound(const Board & board)
{
	array<Integer, PopStarProperties::popStarNumOfColors + 1> stat;
	stat.fill(0);

	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			stat[board(i, j)]++;
		}
	}

	Integer result = 0;
	result += PopStarProperties::GetPointRemainStar(0);
	for (Integer i = BoardProperties::boardLow; i <= BoardProperties::boardUp; ++i) {
		result += PopStarProperties::GetPointCancelStar(stat[i]);
	}

	return result;
}

Integer GetScoreTighterUpperBound(const Board & board, const GroupType & group)
{
	Board tempBoard = board;
	Integer additionalScore = 0;
	for (const auto &subGroup : group) {
		for (auto iter = subGroup.begin(); iter != subGroup.end(); ++iter) {
			// eliminate all the groups
			tempBoard((*iter)[0], (*iter)[1]) = BoardProperties::boardEmpty;
		}
		// add up the scores
		additionalScore += PopStarProperties::GetPointCancelStar(subGroup.size());
	}

	// calculate upper bound of metric
	Integer upperBound = additionalScore + GetScoreUpperBound(tempBoard);
	return upperBound;
}

/*
ProblemState & ProblemState::operator=(const ProblemState & other)
{
	*this->initialBoard = *other.initialBoard;
	*this->currentBoard = *other.currentBoard;

	CopyGroup(*other.currentGroup, *this->currentGroup);

	this->stepTaken->clear();
	this->stepTaken->reserve(other.stepTaken->size() + 1);
	copy(other.stepTaken->begin(), other.stepTaken->end(), back_inserter(*this->stepTaken));

	this->currentScore = other.currentScore;

	return *this;
}
*/

void ProblemState::CopyFrom(const ProblemState & other)
{
	*this->initialBoard = *other.initialBoard;
	*this->currentBoard = *other.currentBoard;

	CopyGroup(*other.currentGroup, *this->currentGroup);

	this->stepTaken->clear();
	this->stepTaken->reserve(other.stepTaken->size() + 1);
	copy(other.stepTaken->begin(), other.stepTaken->end(), back_inserter(*this->stepTaken));

	this->currentScore = other.currentScore;
	this->currentMetric = other.currentMetric;
}

void ProblemState::GainResource()
{
	this->initialBoard = move(make_unique<Board>());
	this->currentBoard = move(make_unique<Board>());
	this->currentGroup = move(make_unique<GroupType>());
	this->stepTaken = move(make_unique<AVector<Vec2i>>());
}

ProblemState GetProblemState(const Board & board)
{
	ProblemState state;
	state.GainResource();
	auto allGroup = move(GetAllGroup(board));
	*state.currentGroup = move(allGroup);
	*state.currentBoard = board;
	*state.initialBoard = board;
	
	return state;
}

vector<ProblemState> ExpandState(const ProblemState & state)
{
	vector<ProblemState> result;
	result.reserve(state.currentGroup->size());
	for (const auto &subGroup : *state.currentGroup) {
		ProblemState newState;
		newState.GainResource();
		// copy from the old state
		newState.CopyFrom(state);

		// remove stars in this group
		for (auto iter = subGroup.begin(); iter != subGroup.end(); ++iter) {
			(*newState.currentBoard)((*iter)[0], (*iter)[1]) = BoardProperties::boardEmpty;
		}

		// add up score
		newState.currentScore += PopStarProperties::GetPointCancelStar(subGroup.size());

		// calculate next board and group info
		(*newState.currentBoard) = GetNextBoard(*newState.currentBoard);
		(*newState.currentGroup) = move(GetAllGroup(*newState.currentBoard));
		
		// find the smallest element in the group, and use that as the step indicator
		Vec2i smallestElement = *min_element(subGroup.begin(), subGroup.end(), less<Vec2i>{});
		newState.stepTaken->push_back(smallestElement);

		result.emplace_back(move(newState));
	}

	return result;
}

Integer GetNaiveStateMetric(const ProblemState & state)
{
	return state.currentScore + GetScoreLowerBound(*state.currentBoard, *state.currentGroup);
}

Integer GetNaiveSearchWidth(Integer currentStep)
{
	currentStep = max(0, currentStep);
	Integer currentWidth = max(naiveSearchWidthUp -  currentStep, naiveSearchWidthLow);
	return currentWidth;
}

ProblemState GetNaiveBestSolution(bool &foundSolution, ProblemState && state)
{
	if (naiveSearchWidthLow < 1) {
		throw runtime_error{ "invalid parameter" };
	}

	state.currentMetric = GetNaiveStateMetric(state);

	struct MetricSortLess {
		bool operator()(const ProblemState &left, const ProblemState &right) const {
			return left.currentMetric < right.currentMetric;
		}
	};

	struct MetricSortGreater {
		bool operator()(const ProblemState &left, const ProblemState &right) const {
			return left.currentMetric > right.currentMetric;
		}
	};

	struct SubGroupMetricSortGreater {
		bool operator()(const ProblemState &left, const ProblemState &right) const {
			if (left.currentGroup->size() > right.currentGroup->size()) {
				return true;
			}
			else if (left.currentGroup->size() == right.currentGroup->size()) {
				return left.currentMetric < right.currentMetric;
			}
			else {
				return false;
			}
		}
	};
	
	struct SubGroupSortGreater {
		bool operator()(const ProblemState &left, const ProblemState &right) const {
			return left.currentGroup->size() > right.currentGroup->size();
		}
	};

	priority_queue<ProblemState, vector<ProblemState>, MetricSortLess> myQueue;
	myQueue.push(forward<ProblemState&&>(state));

	Integer expandMetricLowBound = 0;

	// store the best state that we found
	ProblemState bestState;
	bestState.GainResource();

	auto startTime = chrono::system_clock::now();

	while (!myQueue.empty()) {
		auto nowTime = chrono::system_clock::now();
		auto timeGap = chrono::duration_cast<chrono::seconds>(nowTime - startTime);
		if (timeGap > naiveSearchGiveUpLimit) {
			break;
		}
		else if (timeGap > naiveSearchTimeUpperBound) {
			if (foundSolution) {
				break;
			}
		}

		const auto &front = myQueue.top();
		
		// check whether it is a solution
		if (front.currentGroup->size() == 0) {
			foundSolution = true;
			if (front.currentMetric > bestState.currentMetric) {
				bestState.CopyFrom(front);
				cout << "current best: " << bestState.currentMetric << endl;
			}
			myQueue.pop();
			continue;
		}

		if (foundSolution && bestState.currentMetric >= naiveSearchSatisfactoryScore) {
			if (timeGap > naiveSearchTimeLowerBound) {
				break;
			}
		}

		// expand current state
		// there should be at least one state
		auto expansion = move(ExpandState(front));
		// refresh the metric for expansion
		for (auto &state : expansion) {
			state.currentMetric = GetNaiveStateMetric(state);
		}

		// front has been used off, pop it
		myQueue.pop();

		// sort the states according to the number of remaining subgroups
		// the ones with very little subgroups left will be estimated differently
		// just to make sure it makes reasonable choices towards the end
		sort(expansion.begin(), expansion.end(), SubGroupSortGreater{});
		auto rIter = expansion.rbegin();
		for (; rIter != expansion.rend(); ++rIter) {
			// when there is less than 4 groups left
			if (rIter->currentGroup->size() < 5) {
				auto metricUpperBound = rIter->currentScore + GetScoreTighterUpperBound(*rIter->currentBoard, *rIter->currentGroup);
				if (metricUpperBound > bestState.currentMetric) {
					myQueue.push(move(*rIter));
				}
			}
			else {
				break;
			}
		}

		auto rIterBase = rIter.base();
		if (rIter == expansion.rend()) {
			rIterBase = expansion.begin();
		}
		expansion.erase(rIterBase, expansion.end());

		if (expansion.empty()) {
			continue;
		}

		// sort the states according to the metric
		sort(expansion.begin(), expansion.end(), MetricSortGreater{});

		// calculate average step
		float stepTotal = 0.0f;
		for (const auto &state : expansion) {
			stepTotal += (float)state.stepTaken->size();
		}
		Integer stepAverage = (Integer)round(stepTotal / (float)expansion.size());

		// select the top results for next step
		auto lookElement = min(GetNaiveSearchWidth(stepAverage) - 1, (Integer)expansion.size() - 1);
		assert(lookElement > -1);

		// push in the states for further search
		for (auto i = 0U; i < expansion.size(); ++i) {
			auto &state = expansion[i];
			if (state.currentMetric < expansion[lookElement].currentMetric) {
				break;
			}
			Integer stepDiff = abs((Integer)(state.stepTaken->size() - bestState.stepTaken->size()));

			auto metricUpperBound = state.currentScore + GetScoreTighterUpperBound(*state.currentBoard, *state.currentGroup);
			
			if (state.stepTaken->size() >= bestState.stepTaken->size()) {
				if (metricUpperBound >= bestState.currentMetric) {
					myQueue.push(move(state));
					continue;
				}
			}

			if (state.currentMetric * (stepDiff + 2) > bestState.currentMetric) {
				myQueue.push(move(state));
				continue;
			}

		}
	}

	if (foundSolution) {
		cout << "found!\n";
		for (const auto &ele : *bestState.stepTaken) {
			cout << (ele.cast<int>() + Vec2i{ 1,1 }).transpose() << endl;
		}
	}

	return bestState;

}


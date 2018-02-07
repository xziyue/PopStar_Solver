#include "PCH.H"
#include "BoardOp.h"

const AVector<Vec2i> validDirection{
	Vec2i{-1,0},
	Vec2i{1,0},
	Vec2i{0,-1},
	Vec2i{0,1}
};

void CopyGroup(const GroupType & from, GroupType & to)
{
	to.clear();
	to.reserve(from.size());

	for (const auto &subGroup : from) {
		SubGroupType newSubGroup;
		for (auto iter = subGroup.begin(); iter != subGroup.end(); ++iter) {
			newSubGroup.insert(*iter);
		}
		to.emplace_back(move(newSubGroup));
	}
}

bool IsPositionValid(const Vec2i & pos)
{
	if (pos[0] > -1 && pos[0] < PopStarProperties::popStarBoardRows) {
		if (pos[1] > -1 && pos[1] < PopStarProperties::popStarBoardCols) {
			return true;
		}
	}
	return false;
}

GroupType GetAllGroup(const Board & board)
{
	ASet<Vec2i> processed;
	AQueue<Vec2i> myQueue;
	unique_ptr<SubGroupType> groupElement;
	GroupType result;

	groupElement = move(make_unique<SubGroupType>());

	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			Vec2i currentStar{ i,j };
			if (processed.find(currentStar) != processed.end()) {
				continue;
			}

			auto targetColor = board(currentStar[0], currentStar[1]);

			// do not find grouping for empty blocks
			if (targetColor == BoardProperties::boardEmpty) {
				processed.insert(currentStar);
				continue;
			}

			assert(myQueue.empty());
			myQueue.push(currentStar);

			while (!myQueue.empty()) {
				Vec2i front = move(myQueue.front());
				myQueue.pop();

				if (processed.find(front) != processed.end()) {
					continue;
				}

				groupElement->insert(front);
				processed.insert(front);

				for (const auto &dir : validDirection) {
					Vec2i newPos = front + dir;
					if (IsPositionValid(newPos)) { // if the new position is valid
						if (board(newPos[0], newPos[1]) == targetColor) {
							// if it happens to share the same color
							myQueue.push(newPos);
						}
					}
				}
			}

			// if there is only one block , it is not a group
			if (groupElement->size() == 1) {
				groupElement->clear();
				continue;
			}

			// if there is a group found
			if (!groupElement->empty()) {
				result.emplace_back(move(*groupElement));
				groupElement = move(make_unique<SubGroupType>());
			}

		}
	}

	return result;

}

LooseColumnInfo GetLooseStarCol(const Board & board)
{
	LooseColumnInfo result;

	// sequential scan from left to right
	for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
		bool hasContent = false;
		bool hasLooseStar = false;

		// scan for the lowest color block
		Integer lowestColorBlock = PopStarProperties::popStarBoardRows - 1;
		for (; lowestColorBlock > -1; --lowestColorBlock) {
			if (board(lowestColorBlock, j) != BoardProperties::boardEmpty) {
				break;
			}
		}
		if (lowestColorBlock == -1) {
			// the entire column is emtry
			result.emptyCol.push_back(j);
			continue;
		}
		

		// scan for the lowest empty block
		Integer lowestEmptyBlock = PopStarProperties::popStarBoardRows - 1;
		for (; lowestEmptyBlock > -1; --lowestEmptyBlock) {
			if (board(lowestEmptyBlock, j) == BoardProperties::boardEmpty) {
				break;
			}
		}
		
		if (lowestEmptyBlock == -1) {
			continue;
		}

		// see if there is color block on top of it
		for (Integer i = lowestEmptyBlock - 1; i > -1; --i) {
			if (board(i, j) != BoardProperties::boardEmpty) {
				result.looseStarCol.push_back(j);
				break;
			}
		}

	}

	return result;

}

Integer CountColorStars(const Board & board)
{
	Integer result = 0;
	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			if (board(i, j) != BoardProperties::boardEmpty) {
				result++;
			}
		}
	}
	return result;
}

Board GetNextBoard(const Board & board, const LooseColumnInfo & info)
{
	Board result = board;
	Board finalResult;

	finalResult.fill(BoardProperties::boardEmpty);

	for (const auto &col : info.looseStarCol) {
		BoardCol newCol;
		Integer newColIndex = PopStarProperties::popStarBoardRows - 1;

		newCol.fill(BoardProperties::boardEmpty);
		// copy all the color stars into the new column vector
		for (Integer i = PopStarProperties::popStarBoardRows - 1; i > -1; --i) {
			if (board(i, col) != BoardProperties::boardEmpty) {
				newCol[newColIndex--] = board(i, col);
			}
		}

		// replace the old column with the new one
		result.block(0, col, result.rows(), 1) = newCol.block(0, 0, newCol.rows(), 1);
	}

	vector<Integer> allColumns;
	allColumns.reserve(PopStarProperties::popStarBoardCols);

	for (Integer i = 0; i < PopStarProperties::popStarBoardCols; ++i) {
		allColumns.push_back(i);
	}

	if (info.emptyCol.empty()) {
		return result;
	}

	vector<Integer> validColumns;
	validColumns.reserve(PopStarProperties::popStarBoardCols);
	set_difference(allColumns.begin(), allColumns.end(), info.emptyCol.begin(), info.emptyCol.end(), back_inserter(validColumns));

	Integer finalResultColIndex = 0;
	for (const auto &col : validColumns) {
		finalResult.block(0, finalResultColIndex, finalResult.rows(), 1) = result.block(0, col, result.cols(), 1);
		finalResultColIndex++;
	}

	return finalResult;
}

Board GetNextBoard(const Board & board)
{
	auto info = move(GetLooseStarCol(board));
	return GetNextBoard(board, info);
}
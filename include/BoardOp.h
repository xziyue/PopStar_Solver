#pragma once

#include "CoreDef.h"

using SubGroupType = ASet<Vec2i>;
using GroupType = AVector<SubGroupType>;

void CopyGroup(const GroupType &from, GroupType &to);

bool IsPositionValid(const Vec2i &pos);

// returns all groups on the board
GroupType GetAllGroup(const Board &board);

struct LooseColumnInfo {
	vector<Integer> looseStarCol;
	vector<Integer> emptyCol;
};

// returns the columns with loose star
// the result has already been sorted
LooseColumnInfo GetLooseStarCol(const Board &board);

Integer CountColorStars(const Board &board);

Board GetNextBoard(const Board &board, const LooseColumnInfo &info);

Board GetNextBoard(const Board &board);
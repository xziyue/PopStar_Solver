#include "PCH.H"
#include "CoreDef.h"
#include "BoardOp.h"
#include "ScoreOp.h"
#include "PopStarUI.h"
#include <cstdlib>


void ModifyWindowBoard() {
	for (Integer i = 0; i < PopStarProperties::popStarBoardRows; ++i) {
		for (Integer j = 0; j < PopStarProperties::popStarBoardCols; ++j) {
			Vec2i shiftedPos{ i - 4, j - 4 };
			Integer colorAssignment;
			if (shiftedPos[0] > 0) {
				if (shiftedPos[1] > 0) {
					colorAssignment = 0;
				}
				else {
					colorAssignment = 1;
				}
			}
			else {
				if (shiftedPos[1] > 0) {
					colorAssignment = 2;
				}
				else {
					colorAssignment = 3;
				}
			}
			(*windowBoard)(i, j) = colorAssignment;
		}
	}
}

int main() {

	InitializeDemo();
	
	RunDemo();

	system("pause");

	return 0;
}


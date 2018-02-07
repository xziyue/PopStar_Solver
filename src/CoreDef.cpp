#include "PCH.H"
#include "CoreDef.h"

Integer PopStarProperties::GetPointCancelStar(Integer nStar)
{
	assert(nStar > -1);

	return 5 * nStar * nStar;
}

Integer PopStarProperties::GetPointRemainStar(Integer nStar)
{
	assert(nStar > -1);

	if (nStar > 9) {
		return 0;
	}
	else {
		return -20 * nStar * nStar + 2000;
	}
}

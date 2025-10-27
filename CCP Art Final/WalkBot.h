#include "Includes.h"

struct WalkPath {
	std::vector<float> points;
	std::vector<float> rotations;

	std::vector<std::vector<int>> map;

	float nextPointX;
	float nextPointY;

	float currentPointX;
	float currentPointY;
};

float computeTaxicabDistance(float x1, float x2, float y1, float y2) {
	return abs(y2 - y1) + abs(x2 - x1);
}

std::vector<float> computeTaxicabComponents(float x1, float x2, float y1, float y2) {
	float distanceX;
	float distanceY;

	distanceX = abs(x2 - x1);
	distanceY = abs(y2 - y1);

	return { distanceX, distanceY };
}

float computeRotation() {

}

float getNextPoint() {

}

float generatePathStrategy() {

}


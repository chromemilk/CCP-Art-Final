#include "MathHelpers.h"
inline float2 tileCenter( int tx, int ty ) {
	return { tx + 0.5f, ty + 0.5f };
}
inline int2 worldToTile( float x, float y ) {
	return { int( x ), int( y ) };
}

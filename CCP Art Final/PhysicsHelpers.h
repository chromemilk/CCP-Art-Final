#include "GameEngine.h"

static void artworkMountedCenter( const Engine &engineContext, const Artwork &art, float &centerX, float &centerY) {
    centerX = art.x;
    centerY = art.y;

    // Bounds and wall check
    if (!art.onWall || art.wx < 0 || art.wy < 0) return;

    auto emptyAt = [&]( int x, int y ) {
        if (x < 0 || y < 0 || x >= engineContext.map.width || y >= engineContext.map.height) return false;
        return engineContext.map.tiles[ y * engineContext.map.width + x ] == 0;
        };


    if (art.side == 0)
    {
        // vertical wall
        bool emptyWest = emptyAt( art.wx - 1, art.wy );
        bool emptyEast = emptyAt( art.wx + 1, art.wy );
        float xEdge = art.wx + (emptyEast ? 1.0f : 0.0f); // exposed edge
        centerX = xEdge;
        centerY = art.wy + std::clamp( art.uCenter, 0.0f, 1.0f );
    }
    else
    {
        // horizontal wall face; u runs along X
        bool emptyNorth = emptyAt( art.wx, art.wy - 1 );
        bool emptySouth = emptyAt( art.wx, art.wy + 1 );
        float yEdge = art.wy + (emptySouth ? 1.0f : 0.0f);
        centerX = art.wx + std::clamp( art.uCenter, 0.0f, 1.0f );
        centerY = yEdge;
    }
}



static int findNearestArtwork( Engine &engineContext ) {
    int best = -1;
    float bestD2 = (1.2f * 1.2f); // 1.2m radius

    for (const auto &currentWork : engineContext.artworks)
    {
        float centerX;
        float centerY;
        artworkMountedCenter( engineContext, currentWork, centerX, centerY );
        float dx = centerX - engineContext.positionX;
        float dy = centerY - engineContext.positionY;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2)
        {
            best = currentWork.id;
            bestD2 = d2;
        }
    }
    return best;
}



static void attachArtworksToWalls( Engine &engineContext ) {
    auto inBounds = [&]( int x, int y ) {
        return (x >= 0 && y >= 0 && x < engineContext.map.width && y < engineContext.map.height);
        };
    auto isWall = [&]( int x, int y ) {
        return inBounds( x, y ) && (engineContext.map.tiles[ y * engineContext.map.width + x ] == 1);
        };
    // What counts as "interior"? floor (0); optionally treat doors (2) as interior too.
    auto isInterior = [&]( int x, int y ) {
        if (!inBounds( x, y )) return false;
        int t = engineContext.map.tiles[ y * engineContext.map.width + x ];
        return (t == 0) || (t == 2); // include closed doors as interior; change to (t==0) if you prefer
        };

    struct Face
    {
        int wx;
        int wy;   // wall tile coords
        int side;     // 0 = vertical face, 1 = horizontal face (matches your renderer)
        float ax;
        float ay; // segment start in world coords
        float bx;
        float by; // segment end   in world coords
    };

    std::vector<Face> faces;
    faces.reserve( engineContext.map.width * engineContext.map.height * 2 );

    // Go through all inside wall segments 
    for (int y = 0; y < engineContext.map.height; ++y)
    {
        for (int x = 0; x < engineContext.map.width; ++x)
        {
            if (!isWall( x, y )) continue;

            // West face (vertical) if west neighbor is interior
            if (isInterior( x - 1, y ))
            {
                faces.push_back( Face{ x, y, 0, float( x ),   float( y ), float( x ),   float( y + 1 ) } );
            }
            // East face (vertical) if east neighbor is interior
            if (isInterior( x + 1, y ))
            {
                faces.push_back( Face{ x, y, 0, float( x + 1 ), float( y ), float( x + 1 ), float( y + 1 ) } );
            }
            // North face (horizontal) if north neighbor is interior
            if (isInterior( x, y - 1 ))
            {
                faces.push_back( Face{ x, y, 1, float( x ), float( y ),   float( x + 1 ), float( y ) } );
            }
            // South face (horizontal) if south neighbor is interior
            if (isInterior( x, y + 1 ))
            {
                faces.push_back( Face{ x, y, 1, float( x ), float( y + 1 ), float( x + 1 ), float( y + 1 ) } );
            }
        }
    }

    // Project a point onto a segment, returning squared distance and param u in [0..1]
    auto pointSegment = []( float px, float py,
        float ax, float ay, float bx, float by,
        float &uOut )->float {
            float vx = bx - ax, vy = by - ay;
            float wx = px - ax, wy = py - ay;
            float vv = vx * vx + vy * vy;
            if (vv < 1e-8f)
            {
                uOut = 0.f; return wx * wx + wy * wy;
            }
            float t = (wx * vx + wy * vy) / vv;
            uOut = std::clamp( t, 0.f, 1.f );
            float cx = ax + vx * uOut, cy = ay + vy * uOut;
            float dx = px - cx, dy = py - cy;
            return dx * dx + dy * dy;
        };

    for (auto &art : engineContext.artworks)
    {
        art.onWall = false;

        if (faces.empty()) continue;

        float bestD2 = std::numeric_limits<float>::infinity();
        float bestU = 0.5f;
        int bestI = -1;

        for (int i = 0; i < (int)faces.size(); ++i)
        {
            const Face &face = faces[ i ];
            float u;
            float d2 = pointSegment( art.x, art.y, face.ax, face.ay, face.bx, face.by, u );
            if (d2 < bestD2)
            {
                bestD2 = d2; 
                bestU = u; 
                bestI = i;
            }
        }

        if (bestI >= 0)
        {
            const Face &f = faces[ bestI ];
            art.wx = f.wx;
            art.wy = f.wy;
            art.side = f.side;                     // 0 = vertical, 1 = horizontaal
            art.uCenter = std::clamp( bestU, 0.10f, 0.90f ); // avoid seam/edge clipping
          //  art.uWidth = 0.55f;
      //      art.vCenter = 0.55f;
         //   art.vHeight = 0.60f;
            art.onWall = true;
        }
    }
}

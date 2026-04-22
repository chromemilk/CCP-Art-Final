#include "Constants.h"
#include "MapHelpers.h"
#include <cmath>
#include <vector>
#include <string>
#include <array>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>   
#include <filesystem>
#include <iostream>

enum Levels
{
    ENTRANCE = 0,
    MUSEUM = 1,
    MUSEUM_UPPER = 2,
    TRANSITION = 3,
    CAVE = 4,
    COUNT
};



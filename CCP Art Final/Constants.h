#pragma once 
#include <cmath>
static const int RENDER_W = 960;      
static const int RENDER_H = 540;
static const int WIN_SCALE = 2;      
<<<<<<< HEAD
static const float FOV = 60.0f * (3.14159265f / 180.0f);
=======
static const float FOV = 66.0f * (3.14159265f / 180.0f);
>>>>>>> 3cd17373d5b7247ded538646bc679b3bba787dca
static const float FOV_TAN = std::tan( FOV * 0.5f );
static const float MOVE_SPEED = 2.5f; // units/sec
static const float TURN_SPEED = 2.2f; // rad/sec

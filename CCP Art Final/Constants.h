#pragma once 
#include <cmath>
static const int RENDER_W = 1920;      
static const int RENDER_H = 1080;
static const int WIN_SCALE = 1;      
static const float FOV = 66.0f * (3.14159265f / 180.0f);
static const float FOV_TAN = std::tan( FOV * 0.5f );
static const float MOVE_SPEED = 2.5f; // units/sec
static const float TURN_SPEED = 2.2f; // rad/sec

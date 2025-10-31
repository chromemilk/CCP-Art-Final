#pragma once 
#include <cmath>
static const int RENDER_W = 960;      
static const int RENDER_H = 540;
static const int WIN_SCALE = 2;   

// Maybe render at 1080p when fully optimized 
//static const int RENDER_W = 1920;      
//static const int RENDER_H = 1080;
//static const int WIN_SCALE = 1;      

static const float FOV = 60.0f * (3.14159265f / 180.0f);
static const float FOV_TAN = std::tan( FOV * 0.5f );
static const float MOVE_SPEED = 1.8f; // units/sec
static const float TURN_SPEED = 1.3f; // rad/sec

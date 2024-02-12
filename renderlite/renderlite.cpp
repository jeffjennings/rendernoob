// renderlite.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

 #include "olcConsoleGameEngine.h"
using namespace std;

struct vec3d
{
    float x, y, z;
};

struct triangle
{
    vec3d p[3];
};

struct mesh
{
    vector<triangle> tris;
};

struct mat4x4
{
    // 4x4 matrix
    float m[4][4] = { 0 };
};
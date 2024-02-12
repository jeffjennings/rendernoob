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

class olcEngine3D : public olcConsoleGameEngine
{
public:
    olcEngine3D()
    {
        m_sAppName = L"3D rendering demo";
    }

private:
    mesh meshCube;
    // projection matrix
    mat4x4 matProj;
    // viewing angle theta
    float fTheta;
    // position of camera
    vec3d vCamera;

    // matrix-vector multiplication function
    void MultiplyMatrixVector(vec3d& i, vec3d& o, mat4x4& m)
    {
        o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
        o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
        o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
        float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];

        // reduce 4d to 3d
        if (w != 0.0f)
        {
            o.x /= w; o.y /= w, o.z /= w;
        }
    }

public:
    bool OnUserCreate() override
    {
        // an initializer list defining a standard vector:
        // a cube composed of triangles, with each cube face defined as a cardinal direction
        // (or top/bottom) and composed of 2 triangles, each connecting 3 vertices in
        // a clockwise order
        meshCube.tris = {

            // south face sub-lists for 2 triangles, each with 3 vectors
            { 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

            // east                                                     
            { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
            { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

            // north                                                
            { 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
            { 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

            // west                                                     
            { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

            // top                                                      
            { 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
            { 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

            // bottom
            { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
            { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

        };
};


int main()
{
    olcEngine3D demo;
    if (demo.ConstructConsole(256, 240, 4, 4))
        demo.Start();
    return 0;
}
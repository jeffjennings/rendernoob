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

        // projection matrix for projection (multiplication) of a 3D vector to 2D screen.
        // vector [x,y,z] --> [a * f * x, f * y, g * z], 
        // aspect ratio a = h / w (height, width coordinates on 2d screen),
        // f = 1 / tan(theta / 2), field of view (angle) theta is normalized to [-1, 1]
        // (https://www.youtube.com/watch?v=ih20l3pJoeU&list=PLrOv9FMX8xJE8NgepZR1etrsU63fDDGxO&index=24&ab_channel=javidx9&t=1067).
        // g =  [z_far / (z_far - z_near)] - [(z_far * z_near) / (z_far - z_near)]
        // account for apparent motion decreasing at larger viewing distance z:  
        // x' = x / z, y' = y / z ==>
        // [a * f / z * x, f / z * y, g * z] -->
        // [a * f / z * x, f / z * y , q * (z - z_near)], q = [z_far / (z_far - z_near)].
        // want the projection matrix to multiply any 3d vector by to project it to 2D:
        // [x, y, z, 1] * 
        // | a * f      0       0           0 | <-- projection matrix
        // |  0         f       0           0 |
        // |  0         0       q           1 |
        // |  0         0   -z_near * q     0 |
        // = [a * f * x, f * y, q * (z - z_near), z] = [a * f / z * x, f / z * y, q * (z - z_near) / z, 1]
        // so:
        // near plane
        float fNear = 0.1f;
        float fFar = 1000.0f;
        // FOV f [deg]
        float fFov = 90.0f;
        // aspect ratio a
        float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
        // do tangent calculation once [rad]
        float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

        // matrix elements:
        // a * f (first element)
        matProj.m[0][0] = fAspectRatio * fFovRad;
        matProj.m[1][1] = fFovRad;
        matProj.m[2][2] = fFar / (fFar - fNear);
        matProj.m[2][3] = 1.0f;
        matProj.m[3][2] = -fNear * fFar / (fFar - fNear);
        matProj.m[3][3] = 0.0f;

        return true;
    }


    bool OnUserUpdate(float fElapsedTime) override
    {
        // clear screen from top-left to bottom-right
        Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

        // rotate about z- and x-axis
        mat4x4 matRotZ, matRotX;
        fTheta += 1.0f * fElapsedTime;

        // rotation about z
        matRotZ.m[0][0] = cosf(fTheta);
        matRotZ.m[0][1] = sinf(fTheta);
        matRotZ.m[1][0] = -sinf(fTheta);
        matRotZ.m[1][1] = cosf(fTheta);
        matRotZ.m[2][2] = 1;
        matRotZ.m[3][3] = 1;

        // rotation about x by different rate than about z to avoid gimball lock
        matRotX.m[0][0] = 1;
        matRotX.m[1][1] = cosf(fTheta * 0.5f);
        matRotX.m[1][2] = sinf(fTheta * 0.5f);
        matRotX.m[2][1] = -sinf(fTheta * 0.5f);
        matRotX.m[2][2] = cosf(fTheta * 0.5f);
        matRotX.m[3][3] = 1;

        // draw triangles on screen
        for (auto tri : meshCube.tris)
        {
            triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;

            // rotate triangle about z-axis
            MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotZ);
            MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotZ);
            MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotZ);

            // rotate triangle about x-axis
            MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
            MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
            MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);
        return true;
    }
};


int main()
{
    olcEngine3D demo;
    if (demo.ConstructConsole(256, 240, 4, 4))
        demo.Start();
    return 0;
}
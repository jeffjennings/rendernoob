// renderlite.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <strstream>
#include <iostream>
#include "olcConsoleGameEngine.h"
using namespace std;

struct vec3d
{
    float x, y, z;
};

struct triangle
{
    vec3d p[3];

    // triangle symbol
    wchar_t sym;
    // triangle color
    short col;
};

struct mesh
{
    vector<triangle> tris;

    bool LoadFromObjectFile(string sFilename)
    {
        ifstream fi(sFilename);
        if (!fi.is_open())
            return false;

        // local cache of vertices
        vector<vec3d> vertices;

        // while not at end of file
        while (!fi.eof())
        {
            // assume line length <= 128 characters
            char line[128];
            fi.getline(line, 128);

            strstream ss;
            ss << line;

            // store character at start of line
            char cSol;

            // if 'v', the line is a vertex
            if (line[0] == 'v')
            {
                vec3d vv;
                ss >> cSol >> vv.x >> vv.y >> vv.z;
                vertices.push_back(vv);
            }

            // if 'f', the line is a triangle
            if (line[0] == 'f')
            {
                int ff[3];
                ss >> cSol >> ff[0] >> ff[1] >> ff[2];
                // make triangle
                tris.push_back({ vertices[ff[0] - 1], vertices[ff[1] - 1], vertices[ff[2] - 1] });
            }
        }

        return true;
    }
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

    // simulate color in the console using gray shades
    CHAR_INFO GetColour(float lum)
    {
        short bg_col, fg_col;
        wchar_t sym;
        int pixel_bw = (int)(13.0f * lum);
        switch (pixel_bw)
        {
        case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

        case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
        case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
        case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
        case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

        case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
        case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
        case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
        case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

        case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
        case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
        case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
        case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;

        default:
            bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID;
        }

        CHAR_INFO color;
        color.Attributes = bg_col | fg_col;
        color.Char.UnicodeChar = sym;
        return color;
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

        //meshCube.LoadFromObjectFile("./ship.obj");

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

            // translate triangle before projection (offset z-coordinate into the screen)
            triTranslated = triRotatedZX;
            triTranslated.p[0].z = triRotatedZX.p[0].z + 3.0f;
            triTranslated.p[1].z = triRotatedZX.p[1].z + 3.0f;
            triTranslated.p[2].z = triRotatedZX.p[2].z + 3.0f;

            // calculate normal to triangle
            vec3d normal, line1, line2;
            line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
            line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
            line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

            line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
            line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
            line2.z = triTranslated.p[2].z - triTranslated.p[0].z;

            // cross product
            normal.x = line1.y * line2.z - line1.z * line2.y;
            normal.y = line1.z * line2.x - line1.x * line2.z;
            normal.z = line1.x * line2.y - line1.y * line2.x;

            float normlen = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            normal.x /= normlen; normal.y /= normlen; normal.z /= normlen;

            // only show triangle if it's not occulted
            // (i.e. if dot product is nonzero; if z-component of triangle's normal 
            // projected onto the line b/t the camera and the triangle in 3D space is <90 deg)
            if (normal.x * (triTranslated.p[0].x - vCamera.x) + 
                normal.y * (triTranslated.p[0].y - vCamera.y) +
                normal.z * (triTranslated.p[0].z - vCamera.z) < 0.0f)
            {
                // illuminate triangle with light coming from -z
                vec3d light_source = { 0.0f, 0.0f, -1.0f };
                float ldlen = sqrtf(light_source.x * light_source.x + light_source.y * light_source.y + light_source.z * light_source.z);
                light_source.x /= ldlen; light_source.y /= ldlen; light_source.z /= ldlen;

                // dot product b/t normal of triangle plane and light source 
                float dp = normal.x * light_source.x + normal.y * light_source.y + normal.z * light_source.z;

                // set triangle color and symbol values
                CHAR_INFO color = GetColour(dp);
                triTranslated.col = color.Attributes;
                triTranslated.sym = color.Char.UnicodeChar;

                // project triangle from 3D --> 2D
                MultiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
                MultiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
                MultiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);
                triProjected.col = triTranslated.col;
                triProjected.sym = triTranslated.sym;

                // scale field into screen viewing area 
                triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;
                triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
                triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;

                triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
                triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
                triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

                // rasterize triangle
                FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
                    triProjected.p[1].x, triProjected.p[1].y,
                    triProjected.p[2].x, triProjected.p[2].y,
                    triProjected.sym, triProjected.col);
                
                // show wireframe
                //DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
                //    triProjected.p[1].x, triProjected.p[1].y,
                //    triProjected.p[2].x, triProjected.p[2].y,
                //    PIXEL_SOLID, FG_BLUE);
            }

        }

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
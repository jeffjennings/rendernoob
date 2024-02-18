// renderlite.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <strstream>
#include <iostream>
#include <algorithm>
#include "olcConsoleGameEngine.h"
using namespace std;

// char asset[] = "ship.obj";
char asset[] = "teapot.obj";
bool show_wireframe = false;
float zdepth = 6.0f;

struct vec3d
{
    float x = 0;
    float y = 0;
    float z = 0;
    // 4th term for easy matrix-vector multiplication
    float w = 1; 
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
        m_sAppName = L"Render";
    }

private:
    mesh meshCube;
    // position of camera in world space
    vec3d vCamera;
    // projection matrix (converts from view space to screen space)
    mat4x4 matProj;
    // viewing angle theta (spins world transform matrix)
    float fTheta;

    // vector arithmetic utility functions
    vec3d Vector_Add(vec3d& v1, vec3d& v2)
    {
        return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
    }

    vec3d Vector_Sub(vec3d& v1, vec3d& v2)
    {
        return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
    }

    vec3d Vector_Mul(vec3d& v, float k)
    {
        return { v.x * k, v.y * k, v.z * k };
    }

    vec3d Vector_Div(vec3d& v, float k)
    {
        return { v.x / k, v.y / k, v.z / k };
    }

    float Vector_DotProduct(vec3d& v1, vec3d& v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    vec3d Vector_CrossProduct(vec3d& v1, vec3d& v2)
    {
        vec3d v;
        
        v.x = v1.y * v2.z - v1.z * v2.y;
        v.y = v1.z * v2.x - v1.x * v2.z;
        v.z = v1.x * v2.y - v1.y * v2.x;
        
        return v;
    }

    float Vector_Length(vec3d& v)
    {
        return sqrtf(Vector_DotProduct(v, v));
    }

    vec3d Vector_Normalize(vec3d& v)
    {
        float len = Vector_Length(v);
        
        return { v.x / len, v.y / len, v.z / len };
    }


    // matrix utility functions
    mat4x4 Matrix_MakeIdentity()
    {
        mat4x4 matrix;
        
        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        
        return matrix;
    }

    mat4x4 Matrix_MakeRotationX(float fAngleRad)
    {
        mat4x4 matrix;

        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[1][2] = sinf(fAngleRad);
        matrix.m[2][1] = -sinf(fAngleRad);
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;

        return matrix;
    }

    mat4x4 Matrix_MakeRotationY(float fAngleRad)
    {
        mat4x4 matrix;

        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][2] = sinf(fAngleRad);
        matrix.m[2][0] = -sinf(fAngleRad);
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = cosf(fAngleRad);
        matrix.m[3][3] = 1.0f;

        return matrix;
    }

    mat4x4 Matrix_MakeRotationZ(float fAngleRad)
    {
        mat4x4 matrix;

        matrix.m[0][0] = cosf(fAngleRad);
        matrix.m[0][1] = sinf(fAngleRad);
        matrix.m[1][0] = -sinf(fAngleRad);
        matrix.m[1][1] = cosf(fAngleRad);
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;

        return matrix;
    }

    mat4x4 Matrix_MakeTranslation(float x, float y, float z)
    {
        mat4x4 matrix;

        matrix.m[0][0] = 1.0f;
        matrix.m[1][1] = 1.0f;
        matrix.m[2][2] = 1.0f;
        matrix.m[3][3] = 1.0f;
        matrix.m[3][0] = x;
        matrix.m[3][1] = y;
        matrix.m[3][2] = z;

        return matrix;
    }

    mat4x4 Matrix_MakeProjection(float fFovDeg, float fAspectRatio, float fNear, float fFar)
    {
        float fFovRad = 1.0f / tanf(fFovDeg * 0.5f / 180.0f * 3.14159f);

        mat4x4 matrix;

        matrix.m[0][0] = fAspectRatio * fFovRad;
        matrix.m[1][1] = fFovRad;
        matrix.m[2][2] = fFar / (fFar - fNear);
        matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
        matrix.m[2][3] = 1.0f;
        matrix.m[3][3] = 0.0f;

        return matrix;
    }

    vec3d Matrix_MultiplyVector(mat4x4& m, vec3d &i)
    {
        vec3d v;

        v.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + i.w * m.m[3][0];
        v.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + i.w * m.m[3][1];
        v.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + i.w * m.m[3][2];
        v.w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + i.w * m.m[3][3];

        return v;
    }

    mat4x4 Matrix_MultiplyMatrix(mat4x4& m1, mat4x4& m2)
    {
        mat4x4 matrix;

        for (int c = 0; c < 4; c++)
            for (int r = 0; r < 4; r++)
                matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];

        return matrix;
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
        //meshCube.tris = {

        //    // south face sub-lists for 2 triangles, each with 3 vectors
        //    { 0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 0.0f },
        //    { 0.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f },

        //    // east                                                     
        //    { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f },
        //    { 1.0f, 0.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 0.0f, 1.0f },

        //    // north                                                
        //    { 1.0f, 0.0f, 1.0f,    1.0f, 1.0f, 1.0f,    0.0f, 1.0f, 1.0f },
        //    { 1.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 0.0f, 1.0f },

        //    // west                                                     
        //    { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 1.0f,    0.0f, 1.0f, 0.0f },
        //    { 0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f },

        //    // top                                                      
        //    { 0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 1.0f,    1.0f, 1.0f, 1.0f },
        //    { 0.0f, 1.0f, 0.0f,    1.0f, 1.0f, 1.0f,    1.0f, 1.0f, 0.0f },

        //    // bottom
        //    { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f },
        //    { 1.0f, 0.0f, 1.0f,    0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f },

        //};


        // load 3d asset from .obj file
        meshCube.LoadFromObjectFile(asset);


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
        //float fNear = 0.1f;
        //float fFar = 1000.0f;
        // FOV f [deg]
        //float fFov = 90.0f;
        // aspect ratio a
        //float fAspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
        // do tangent calculation once [rad]
        //float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

        // matrix elements:
        // a * f (first element)
        //matProj.m[0][0] = fAspectRatio * fFovRad;
        //matProj.m[1][1] = fFovRad;
        //matProj.m[2][2] = fFar / (fFar - fNear);
        //matProj.m[2][3] = 1.0f;
        //matProj.m[3][2] = -fNear * fFar / (fFar - fNear);
        //matProj.m[3][3] = 0.0f;

        // replacing the above with utility function to make the projection matrix
        matProj = Matrix_MakeProjection(90.0f, (float)ScreenHeight() / (float)ScreenWidth(), 0.1f, 1000.0f);

        return true;
    }


    bool OnUserUpdate(float fElapsedTime) override
    {
        // clear screen from top-left to bottom-right
        Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, FG_BLACK);

        // set up world transform
        mat4x4 matRotZ, matRotX;
        // rotate about z- and x-axis over time
        fTheta += 1.0f * fElapsedTime;

        //// rotation about z
        //matRotZ.m[0][0] = cosf(fTheta);
        //matRotZ.m[0][1] = sinf(fTheta);
        //matRotZ.m[1][0] = -sinf(fTheta);
        //matRotZ.m[1][1] = cosf(fTheta);
        //matRotZ.m[2][2] = 1;
        //matRotZ.m[3][3] = 1;
        matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f);

        //// rotation about x by different rate than about z to avoid gimball lock
        //matRotX.m[0][0] = 1;
        //matRotX.m[1][1] = cosf(fTheta * 0.5f);
        //matRotX.m[1][2] = sinf(fTheta * 0.5f);
        //matRotX.m[2][1] = -sinf(fTheta * 0.5f);
        //matRotX.m[2][2] = cosf(fTheta * 0.5f);
        //matRotX.m[3][3] = 1;
        matRotX = Matrix_MakeRotationX(fTheta);

        mat4x4 matTrans;
        // how far into screen to translate triangle
        matTrans = Matrix_MakeTranslation(0.0f, 0.0f, zdepth);

        mat4x4 matWorld;
        matWorld = Matrix_MakeIdentity();
        // rotate
        matWorld = Matrix_MultiplyMatrix(matRotZ, matRotX);
        // translate
        matWorld = Matrix_MultiplyMatrix(matWorld, matTrans);


        // store triangles for later rasterization
        vector<triangle> vecTrianglesToRaster;

        // draw triangles on screen
        for (auto tri : meshCube.tris)
        {
            //triangle triProjected, triTranslated, triRotatedZ, triRotatedZX;
            triangle triProjected, triTransformed;

            // rotate triangle about z-axis
            //MultiplyMatrixVector(tri.p[0], triRotatedZ.p[0], matRotZ);
            //MultiplyMatrixVector(tri.p[1], triRotatedZ.p[1], matRotZ);
            //MultiplyMatrixVector(tri.p[2], triRotatedZ.p[2], matRotZ);

            // rotate triangle about x-axis
            //MultiplyMatrixVector(triRotatedZ.p[0], triRotatedZX.p[0], matRotX);
            //MultiplyMatrixVector(triRotatedZ.p[1], triRotatedZX.p[1], matRotX);
            //MultiplyMatrixVector(triRotatedZ.p[2], triRotatedZX.p[2], matRotX);

            // translate triangle before projection (offset z-coordinate into the screen)
            //triTranslated = triRotatedZX;
            //triTranslated.p[0].z = triRotatedZX.p[0].z + 8.0f;
            //triTranslated.p[1].z = triRotatedZX.p[1].z + 8.0f;
            //triTranslated.p[2].z = triRotatedZX.p[2].z + 8.0f;

            // world matrix transform
            triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
            triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
            triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);

         
            // calculate triangle normal
            vec3d normal, line1, line2;
            // lines on either side of triangle
            //line1.x = triTranslated.p[1].x - triTranslated.p[0].x;
            //line1.y = triTranslated.p[1].y - triTranslated.p[0].y;
            //line1.z = triTranslated.p[1].z - triTranslated.p[0].z;

            //line2.x = triTranslated.p[2].x - triTranslated.p[0].x;
            //line2.y = triTranslated.p[2].y - triTranslated.p[0].y;
            //line2.z = triTranslated.p[2].z - triTranslated.p[0].z;
            line1 = Vector_Sub(triTransformed.p[1], triTransformed.p[0]);
            line2 = Vector_Sub(triTransformed.p[2], triTransformed.p[0]);

            // normal to triangle surface
            //normal.x = line1.y * line2.z - line1.z * line2.y;
            //normal.y = line1.z * line2.x - line1.x * line2.z;
            //normal.z = line1.x * line2.y - line1.y * line2.x;
            normal = Vector_CrossProduct(line1, line2);

            // normalize
            //float normlen = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            //normal.x /= normlen; normal.y /= normlen; normal.z /= normlen;
            normal = Vector_Normalize(normal);


            // only show triangle if it's not occulted
            // (i.e. if dot product is nonzero; if z-component of triangle's normal 
            // projected onto the line b/t the camera and the triangle in 3D space is <90 deg)
            //if (normal.x * (triTranslated.p[0].x - vCamera.x) + 
            //    normal.y * (triTranslated.p[0].y - vCamera.y) +
            //    normal.z * (triTranslated.p[0].z - vCamera.z) < 0.0f)
            //{
            // get ray from triangle to camera
            vec3d vCameraRay = Vector_Sub(triTransformed.p[0], vCamera);
            // if ray is aligned w/ normal, triangle is visible
            if (Vector_DotProduct(normal, vCameraRay) < 0.0f)
            {

                // illuminate triangle with light coming from -z
                //vec3d light_source = { 0.0f, 0.0f, -1.0f };
                //float ldlen = sqrtf(light_source.x * light_source.x + light_source.y * light_source.y + light_source.z * light_source.z);
                //light_source.x /= ldlen; light_source.y /= ldlen; light_source.z /= ldlen;
                vec3d light_dir = { 0.0f, 1.0f, -1.0f };           
                light_dir = Vector_Normalize(light_dir);

                // dot product b/t normal of triangle plane and light source 
                //float dp = normal.x * light_source.x + normal.y * light_source.y + normal.z * light_source.z;
                float dp = max(0.1f, Vector_DotProduct(light_dir, normal));


                // set triangle color and symbol values
                CHAR_INFO color = GetColour(dp);
                //triTranslated.col = color.Attributes;
                //triTranslated.sym = color.Char.UnicodeChar;
                triTransformed.col = color.Attributes;
                triTransformed.sym = color.Char.UnicodeChar;

                // project triangle from 3D --> 2D
                //MultiplyMatrixVector(triTranslated.p[0], triProjected.p[0], matProj);
                //MultiplyMatrixVector(triTranslated.p[1], triProjected.p[1], matProj);
                //MultiplyMatrixVector(triTranslated.p[2], triProjected.p[2], matProj);
                //triProjected.col = triTranslated.col;
                //triProjected.sym = triTranslated.sym;
                triProjected.p[0] = Matrix_MultiplyVector(matProj, triTransformed.p[0]);
                triProjected.p[1] = Matrix_MultiplyVector(matProj, triTransformed.p[1]);
                triProjected.p[2] = Matrix_MultiplyVector(matProj, triTransformed.p[2]);
                triProjected.col = triTransformed.col;
                triProjected.sym = triTransformed.sym;

                // normalize coordinates
                triProjected.p[0] = Vector_Div(triProjected.p[0], triProjected.p[0].w);
                triProjected.p[1] = Vector_Div(triProjected.p[1], triProjected.p[1].w);
                triProjected.p[2] = Vector_Div(triProjected.p[2], triProjected.p[2].w);

                // scale field into screen viewing area 
                //triProjected.p[0].x += 1.0f; triProjected.p[0].y += 1.0f;
                //triProjected.p[1].x += 1.0f; triProjected.p[1].y += 1.0f;
                //triProjected.p[2].x += 1.0f; triProjected.p[2].y += 1.0f;
                vec3d vOffsetView = { 1,1,0 };
                triProjected.p[0] = Vector_Add(triProjected.p[0], vOffsetView);
                triProjected.p[1] = Vector_Add(triProjected.p[1], vOffsetView);
                triProjected.p[2] = Vector_Add(triProjected.p[2], vOffsetView);

                triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
                triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
                triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
                triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

                // store triangle for z-sorting 
                vecTrianglesToRaster.push_back(triProjected);                
            }

        }

        // sort triangles by midpoint z of each (average of z of the triangle's 3 points), 
        // using lambda function evaluating a pair of triangles (a hack, the "painter's algorithm")
        sort(vecTrianglesToRaster.begin(), vecTrianglesToRaster.end(), [](triangle& t1, triangle& t2)
        {
                float z1 = (t1.p[0].z + t1.p[1].z + t1.p[2].z) / 3.0f;
                float z2 = (t2.p[0].z + t2.p[1].z + t2.p[2].z) / 3.0f;
                // return boolean to decide if positions of the two triangles should be swapped (one in front of other)
                return z1 > z2;
        });

        for (auto& triProjected : vecTrianglesToRaster)
        {
            // rasterize triangle
            FillTriangle(triProjected.p[0].x, triProjected.p[0].y,
                triProjected.p[1].x, triProjected.p[1].y,
                triProjected.p[2].x, triProjected.p[2].y,
                triProjected.sym, triProjected.col);

            // show wireframe (triangle edges)
            if (show_wireframe) {
                DrawTriangle(triProjected.p[0].x, triProjected.p[0].y,
                    triProjected.p[1].x, triProjected.p[1].y,
                    triProjected.p[2].x, triProjected.p[2].y,
                    PIXEL_SOLID, FG_BLUE);
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
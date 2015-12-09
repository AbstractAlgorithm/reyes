#pragma once

#include "vecmx.hpp"
#include <cstdio>

namespace reyes
{
    /* RGBA32F color struct. */
    struct color
    {
        float r, g, b, a;
        color(float _r = 0, float _g = 0, float _b = 0, float _a = 1)
            : r(_r)
            , g(_g)
            , b(_b)
            , a(_a)
        {}
    };

    typedef vec2 uv;

    struct PosNormal
    {
        position p;
        normal n;
    };
    struct PosColor
    {
        position p;
        color col;
    };
    struct PosNormalColor
    {
        position p;
        normal n;
        color col;
    };
    struct PosNormalUV
    {
        position p;
        normal n;
        uv uv;
    };

    struct Texture2D
    {
        uint16_t width, height;
        char* data;

        Texture2D(uint16_t w, uint16_t h)
            : width(w)
            , height(h)
        {}

        bool loadBMP(const char* filename)
        {
            unsigned char header[54];
            unsigned int dataPos;
            unsigned int width, height;
            unsigned int imageSize;
            data = 0;

            // Open the file
            FILE * file = fopen(filename, "rb");
            if (!file)
                return false;
            if (fread(header, 1, 54, file) != 54)
                return false;

            if (header[0] != 'B' || header[1] != 'M')
                return false;

            // Read ints from the byte array
            dataPos = *(int*)&(header[0x0A]);
            imageSize = *(int*)&(header[0x22]);
            width = *(int*)&(header[0x12]);
            height = *(int*)&(header[0x16]);

            if (imageSize == 0)    imageSize = width*height * 3;
            if (dataPos == 0)      dataPos = 54;
            data = new char[imageSize];
            if (!data)
                return false;
            fread(data, 1, imageSize, file);
            fclose(file);

            return true;
        }

        color sampleNearest(uv uv)
        {
            // repeat
            /*while (uv.x > 1.0f) uv.x -= 1.0f;
            while (uv.y > 1.0f) uv.y -= 1.0f;*/

            // calc offset
            uint16_t x = (uint16_t)(uv.x*width);
            uint16_t y = (uint16_t)(uv.y*height);
            uint16_t offset = y*width + x;

            // sample
            return{ data[3 * offset] / 255.0f, data[3 * offset + 1] / 255.0f, data[3 * offset + 2] / 255.0f, 1.0f };
        }
    };
}
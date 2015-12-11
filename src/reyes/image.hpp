#pragma once 

#include "vecmx.hpp"
#include "grid.hpp"
#include "misc.hpp"
#include <cstring>
#include <cstdint>

namespace reyes
{
    /* Image interface. */
    struct ImageI
    {
        uint16_t width, height;
        float half_width, half_height;
        virtual char* getRGB(void) = 0;
        virtual void rasterize(MicrogridI<PosColor>& grid) = 0;
        ImageI(uint16_t _width, uint16_t _height)
            : width(_width)
            , height(_height)
            , half_width(_width/2.0f)
            , half_height(_height/2.0f)
        {}
        void writePPM(const char* filename)
        {
            FILE* ppm_file = fopen(filename, "wb");
            if (!ppm_file)
                return;

            fprintf(ppm_file, "P6\n%d %d\n%d\n", width, height, 255);
            size_t size = width * height * 3;
            char* rgbdata = getRGB();
            size_t cnt = fwrite(rgbdata, sizeof(char), size, ppm_file);
            fclose(ppm_file);
        }
        inline vec2 estimate(vec2 dist)
        {
            return vec2(dist.x*half_width, dist.y*half_height);
        }
    };

    /* Primitive RGB rasterizer. */
    struct RGB8Image : public ImageI
    {
        struct RGB8Pixel
        {
            char r, g, b;
        } *data;

        RGB8Image(uint16_t _width, uint16_t _height)
            : ImageI(_width, _height)
            , data(new RGB8Pixel[_width*_height])
        {
            for (uint16_t x = 0; x < height; x++)
                for (uint16_t y = 0; y < width; y++)
                        data[x*width + y] = { 127, 127, 127 };
        }

        void rasterize(MicrogridI<PosColor>& grid)
        {
            uint16_t p_cnt = grid.count();
            SPrimitiveI* prim;
            float halfpx_x = 1.0f / width;
            float halfpx_y = 1.0f / height;
            for (uint16_t pidx = 0; pidx < p_cnt; pidx++)
            {
                prim = static_cast<SPrimitiveI*>(grid.at(pidx));
                for (uint16_t x = 0; x < width; x++)
                    for (uint16_t y = 0; y < height; y++)
                    {
                        // construct pixel location
                        float px = -1.0f + (2 * x + 1)*halfpx_x;
                        float py = 1.0f - (2 * y + 1)*halfpx_y;
                        vec3 p(px, py, 0);

                        // test
                        if (prim->in(p))
                        {
                            // color
                            color c = prim->at(p).col;
                            data[y*width + x] = { c.r, c.g, c.b };
                        }
                    }
            }
        }
        char* getRGB(void)
        {
            return (char*)data;
        }
    };

    /* G-Buffer storage. */
    struct GBuffer : public ImageI
    {
        struct RGBpixel
        {
            char r, g, b;
        } *rgb_data;
        struct Apixel
        {
            float a;
        } *a_data;
        struct Zpixel
        {
            float z;
        } *z_data;
    
        GBuffer(uint16_t _width, uint16_t _height)
            : ImageI(_width, _height)
            , rgb_data(new RGBpixel[_width*_height])
            , a_data(new Apixel[_width*_height])
            , z_data(new Zpixel[_width*_height])
        {
            for (uint16_t x = 0; x < width; x++)
            for (uint16_t y = 0; y < height; y++)
            {
                rgb_data[y*width + x] = { 0, 0, 0 };
                a_data[y*width + x] = { 1.0f };
                z_data[y*width + x] = { 1.0f };
            }
        }
    
        void rasterize(MicrogridI<PosColor>& grid)
        {
            uint16_t p_cnt = grid.count();
            SPrimitiveI* prim;
            float halfpx_x = 1.0f / width;
            float halfpx_y = 1.0f / height;
            for (uint16_t pidx = 0; pidx < p_cnt; pidx++)
            {
                prim = static_cast<SPrimitiveI*>(grid.at(pidx));
                // get bounding box
                AABB2 bb = prim->aabb();
                uint16_t start_x=0, end_x=width, start_y=0, end_y=height;
                // calc tight rasterization box
                for (uint16_t i = 0; i < width; i++)
                {
                    float px = -1.0f + (2 * i + 1)*halfpx_x;
                    if (px >= bb.min.x)
                    {
                        start_x = i;
                        for (uint16_t j = 0; j < width; j++)
                        {
                            px = -1.0f + (2 * j + 1)*halfpx_x;
                            if (px>bb.max.x)
                            {
                                end_x = j;
                                break;
                            }
                        }
                        break;
                    }
                }
                for (uint16_t i = 0; i < height; i++)
                {
                    float py = 1.0f - (2 * i + 1)*halfpx_y;
                    if (py <= bb.max.y)
                    {
                        start_y = i;
                        for (uint16_t j = 0; j < height; j++)
                        {
                            float py = 1.0f - (2 * j + 1)*halfpx_y;
                            if (py<bb.min.y)
                            {
                                end_y = j;
                                break;
                            }
                        }
                        break;
                    }
                }

                // rasterize
                for (uint16_t x = start_x; x < end_x; x++)
                {
                    for (uint16_t y = start_y; y < end_y; y++)
                    {
                        // construct pixel location
                        float px = -1.0f + (2 * x + 1)*halfpx_x;
                        float py = 1.0f - (2 * y + 1)*halfpx_y;
                        vec3 p(px, py, 0);

                        // test
                        if (prim->in(p))
                        {
                            PosColor r = prim->at(p);
                            RGBpixel px_rgb = rgb_data[y*width + x];
                            Zpixel px_z = z_data[y*width + x];

                            // rasterized pixel should overwrite information
                            if (r.p.z <= px_z.z && r.p.z >= 0.0f)
                            {
                                // TODO depth test settings
                                // TODO blending
                                px_z.z = r.p.z;
                                px_rgb = { r.col.r*255.0f, r.col.g*255.0f, r.col.b*255.0f };
                                rgb_data[y*width + x] = px_rgb;
                                z_data[y*width + x] = px_z;
                            }
                        }
                    }
                }
            }
        }
    
        char* getRGB(void)
        {
            return (char*)rgb_data;
        }
    };
}
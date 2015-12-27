#pragma once

#include "mem.hpp"
#include "shading.hpp"
#include "grid.hpp"
#include "scene.hpp"
#include <cstdlib>
#include <ctime>
#include "settings.hpp"

namespace reyes
{
    inline bool requiresSplit(vec2 size, vec2 threshold)
    {
        return (size.x > threshold.x || size.y > threshold.y);
    }

    struct Shape
    {
        float start_u, end_u, start_v, end_v;
        Microgrid grid;
        Material* material;

        Shape()
            : start_u(0)
            , end_u(1)
            , start_v(0)
            , end_v(1)
        {}

        void dice(void)
        {
            float wu = (end_u - start_u) / (float)(GRID_DIM);
            float wv = (end_v - start_v) / (float)(GRID_DIM);
            Vertex vx;
            uv _uv;

            // vertices
            for (uint16_t u = 0; u < GRID_DIM+1; u++)
            for (uint16_t v = 0; v < GRID_DIM+1; v++)
            {
                _uv = vec2(start_u + u*wu, start_v + v*wv);
                vx.uv = uv(start_u + u*wu, start_v + v*wv);
                vx.p = P(_uv);
                vx.n = N(_uv);
                vx.p = material->pShdr(vx);
                grid.vertices[v * (GRID_DIM + 1) + u] = vx;
            }
        }

        void splitData(Shape* shp, char i)
        {
            shp->material = material;
            if (0 == i)
            {
                shp->start_u = start_u;
                shp->end_u = (start_u + end_u)*0.5f;
                shp->start_v = start_v;
                shp->end_v = (start_v + end_v)*0.5f;
            }
            else if (1 == i)
            {
                shp->start_u = (start_u + end_u)*0.5f;
                shp->end_u = end_u;
                shp->start_v = start_v;
                shp->end_v = (start_v + end_v)*0.5f;
            }
            else if (2 == i)
            {
                shp->start_u = start_u;
                shp->end_u = (start_u + end_u)*0.5f;
                shp->start_v = (start_v + end_v)*0.5f;
                shp->end_v = end_v;
            }
            else if (3 == i)
            {
                shp->start_u = (start_u + end_u)*0.5f;
                shp->end_u = end_u;
                shp->start_v = (start_v + end_v)*0.5f;
                shp->end_v = end_v;
            }
        }

        void shade(void)
        {
#ifdef DEBUG_GRID
            color rnd = { rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 1 };
#endif
            for (uint16_t i = 0; i < GRID_SIZE; i++)
            {
#ifdef DEBUG_GRID
                grid.vertices[i].c = rnd;
#else
                grid.vertices[i].c = material->cShdr(grid.vertices[i]);
#endif
            }
        }

        // -----------------------------------------------
        // every parametric shape needs to implement these
        virtual void split(Scene& scene) = 0;
        virtual position P(uv _uv) = 0;
        virtual normal N(uv _uv) = 0;
        // -----------------------------------------------
    };
}
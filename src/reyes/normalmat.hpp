#pragma once

#include "shading.hpp"

#define CLAMP(_a,_min,_max) ((_a)<(_min)?(_min):(((_a)>(_max))?(_max):(_a)))

namespace reyes
{
    namespace lib
    {
        UNIFORM(NormalColor) {};
        MATERIAL(NormalColor)
        {
            DISPLACE
            {
                return vertex.p;
            }

            SHADE
            {
                /*float nx = CLAMP(vertex.n.x, 0, 1);
                float ny = CLAMP(vertex.n.y, 0, 1);
                float nz = CLAMP(vertex.n.z, 0, 1);
                vec3 n(nx, ny, nz);
                return{ n.x, n.y, n.z, 1 };*/

                vec3 n(vertex.n.x*0.5f + 0.5f, vertex.n.y*0.5f + 0.5f, vertex.n.z*0.5f + 0.5f);
                return{ n.x, n.y, n.z, 1 };
            }
        };
    }
}
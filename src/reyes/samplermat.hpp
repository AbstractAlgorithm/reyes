#pragma once

#include "shading.hpp"

namespace reyes
{
    namespace lib
    {
        UNIFORM(SamplerMat)
        {
            Sampler* sampler;
        };
        MATERIAL(SamplerMat)
        {
            DISPLACE
            {
                return vertex.p;
            }

            SHADE
            {
                return uniform.sampler->sample(vertex.uv);
            }
        };
    }
}
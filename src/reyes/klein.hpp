//#pragma once
//
//#include "shape.hpp"
//
//namespace reyes
//{
//    namespace lib
//    {
//        template<class MaterialTy>
//        struct Klein : public Shape<MaterialTy>
//        {
//            Klein()
//            {}
//
//            normal EvalN(uv uv)
//            {
//                return vec3(0, 0, 1);
//            }
//            position EvalP(uv uv)
//            {
//                float u = (uv.x *2.0f)*M_PI;
//                float v = (uv.y *2.0f) *M_PI;
//
//
//                float A = 6.0f, B = 16.0f, C = 4.0f;
//
//                float r = C * (1.0f - cos(u) / 2.0f);
//                float x, y, z;
//                if (u < M_PI)
//                {
//                    x = A * cos(u)*(1 + sin(u)) + r*cos(u)*cos(v);
//                    y = B * sin(u) + r*sin(u)*cos(v);
//                }
//                else
//                {
//                    x = A * cos(u)*(1 + sin(u)) + cos(v + M_PI);
//                    y = B * sin(u);
//                }
//                z = r*sin(v);
//                /*x = cos(u) * (cos(0.5f*u)*(M_SQRT2 + cos(v)) + sin(0.5f*u)*sin(v)*cos(v));
//                y = sin(u) * (cos(0.5f*u)*(M_SQRT2 + cos(v)) + sin(0.5f*u)*sin(v)*cos(v));
//                z = -sin(0.5f*u)*(M_SQRT2 + cos(v)) + cos(0.5f*u)*sin(v)*cos(v);*/
//                position p(x, -y, z);
//                return p*0.05f;
//            }
//
//            void split(SplitDir direction, Scene& scene)
//            {
//                mem::blk mblks[4];
//                for (char i = 0; i < 4; i++)
//                {
//                    mblks[i] = scene.alloc(sizeof(Klein<MaterialTy>));
//                    Klein<MaterialTy>* p = ::new(mblks[i].ptr) Klein<MaterialTy>;
//                    Shape<MaterialTy>::splitData(p, i);
//                }
//            }
//        };
//    }
//}
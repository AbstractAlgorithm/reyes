#include <cstdio>
#include "pipeline.hpp"
#include "matlib.hpp"

#define MAKE_SHAPE(_alloc, _shp, _mat) new(_alloc.alloc(sizeof(_shp<_mat>))) _shp<_mat>

using namespace reyes;

void appMain()
{
    // scene setup
    mem::ObjectStack<ShapeI> scene(1024);
    new(scene.alloc(sizeof(Quadrilateral<matlib::Lambert>))) Quadrilateral<matlib::Lambert>();

    // camera setup
    camera camera;
    camera.orthographic(-10, 10, 10, 10);
    camera.lookAt(/*eye*/ { 0, 0, -5 }, /*target*/ { 0, 0, 0 } /*up*/);

    // image setup
    GBuffer image(10,10);

    // render
    reyes::render(scene, camera, image);
}
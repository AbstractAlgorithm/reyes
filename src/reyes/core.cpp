#include "core.h"

reyes::mem::mAllocator reyes::MEMORY;
reyes::mem::StackAllocator< 1 << 24/*16MB*/ > reyes::STACK;
reyes::renderer REYES;

using namespace reyes;

int main()
{
    appMain();
    return 0;
}
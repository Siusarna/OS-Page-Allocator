#include <iostream>
#include <ctime>


#include "allocator.h"

using namespace std;


int main(int argc, char const* argv[])
{
    Allocator alloc;

    cout << "Allocating memory for x variable..." << endl;
    uint8_t* x = (uint8_t*)alloc.mem_alloc(8080);
    alloc.mem_dump();

    cout << "Realoc" << endl;
    x = (uint8_t*)alloc.mem_realloc(x, 1024);

    alloc.mem_dump();
    alloc.mem_free(x);
    alloc.mem_dump();
    return 0;
}
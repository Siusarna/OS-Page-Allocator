#include <cstdint>
#include <vector>
#include <map>

using namespace std;

#define MEMORY_SIZE 4096 * 3 // 20kb
#define PAGE_SIZE 4096 // 4kb
#define MINIMAL_BLOCK_SIZE 16 // 2^x де x >= 4
#define PAGE_AMOUNT ceil(MEMORY_SIZE / PAGE_SIZE)

enum PageStatus
{
	Free,
	DividedIntoBlocks,
	MultiPageBlock
};

struct PageDescription
{
	PageStatus state;
	size_t bsize;
	size_t blocksAmount;
	uint8_t* ptrToFirstFree;
};

class Allocator
{
private:
	static uint8_t memory[MEMORY_SIZE];
	static size_t countOfPage;
	static map <void*, PageDescription> pageDescriptions;
	static vector <void*> freePages;
	static map <int, vector<void*>> blockedPages;

	size_t getLogarithmByTwo(size_t);
	bool splitPageByBlocks(size_t);
	void* anyFreeBlock(uint8_t*, size_t);
	size_t amountOfPage(size_t);
	void setPageDescription(void*, PageStatus, size_t, size_t, uint8_t*);
	void* allocateByPages(size_t);
	void* allocateByBlocks(size_t);

public:
	Allocator();

	void* mem_alloc(size_t);
	void* mem_realloc(void*, size_t);
	void mem_free(void*);
	void mem_dump();
};
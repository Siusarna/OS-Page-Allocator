#include "allocator.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <math.h>

uint8_t Allocator::memory[MEMORY_SIZE];
size_t Allocator::countOfPage = PAGE_AMOUNT;
map<void*, PageDescription> Allocator::pageDescriptions;
vector<void*> Allocator::freePages;
map<int, vector<void*>> Allocator::blockedPages;


Allocator::Allocator() {
	uint8_t* pagePtr = (uint8_t*)memory;

	for (int i = 0; i < countOfPage; i++) {
		PageDescription header = { Free, 0, 0, NULL };
		pageDescriptions.insert({ pagePtr, header });
		freePages.push_back(pagePtr);
		pagePtr += PAGE_SIZE;
	}

	int bsize = MINIMAL_BLOCK_SIZE;

	for (; bsize <= PAGE_SIZE / 2; bsize <<= 1) {
		blockedPages.insert({ bsize, {} });
	}
}

size_t Allocator::getLogarithmByTwo(size_t size) {
	return ceil(log2(size));
}

void* Allocator::anyFreeBlock(uint8_t* pagePtr, size_t blockSize) {
	for (uint8_t* currPtr = (uint8_t*)pagePtr; currPtr < pagePtr + PAGE_SIZE; currPtr += blockSize) {
		if ((bool)*currPtr) {
			return currPtr;
		}
	}

	return nullptr;
}

size_t Allocator::amountOfPage(size_t size) {
	return size / PAGE_SIZE + (int)(size % PAGE_SIZE != 0 && size > PAGE_SIZE / 2);
}

void Allocator::setPageDescription(void* pagePtr, PageStatus status, size_t bsize, size_t blockAmount, uint8_t* ptrToFirstFree) {
	pageDescriptions[pagePtr].state = status;
	pageDescriptions[pagePtr].bsize = bsize;
	pageDescriptions[pagePtr].blocksAmount = blockAmount;
	pageDescriptions[pagePtr].ptrToFirstFree = ptrToFirstFree;
}

void setBlockHeader(void* blockPointer, bool status) {
	uint8_t* pointer = (uint8_t*)blockPointer;
	*pointer = status;
}

bool Allocator::splitPageByBlocks(size_t bsize) {
	if (freePages.empty()) {
		return false;
	}

	uint8_t* pagePtr = (uint8_t*)freePages[0];
	freePages.erase(freePages.begin());

	setPageDescription(pagePtr, DividedIntoBlocks, bsize, PAGE_SIZE / bsize, pagePtr);

	blockedPages[bsize].push_back(pagePtr);

	uint8_t* currPtr = (uint8_t*)pagePtr;

	for (; currPtr < pagePtr + PAGE_SIZE; currPtr += bsize) {
		*currPtr = true;
	}

	return true;
}

void* Allocator::allocateByPages(size_t size) {
	size_t pagesNeeded = amountOfPage(size);

	if (freePages.size() < pagesNeeded) {
		cout << "$$$$$$$ FAIL need more memory $$$$$$$";
		return nullptr;
	}

	void* ptr = freePages[0];

	for (int i = 1; i <= pagesNeeded; i++) {
		uint8_t* pagePtr = (uint8_t*)freePages[0];

		uint8_t* nextPagePtr;
		if (i == pagesNeeded) {
			nextPagePtr = nullptr;
		}
		else {
			nextPagePtr = (uint8_t*)freePages[1];
		}

		setPageDescription(pagePtr, MultiPageBlock, pagesNeeded * PAGE_SIZE, pagesNeeded - i, nextPagePtr);

		freePages.erase(freePages.begin());
	}

	return ptr;
}

void* Allocator::allocateByBlocks(size_t size) {
	// +1 עמלף שמ ץוהונ לא÷ bool פכאד
	size_t classSize = getLogarithmByTwo(size + 1);

	if (blockedPages[classSize].empty() && !splitPageByBlocks(classSize)) {
		cout << "$$$$$$$ No free memory $$$$$$$";
		return nullptr;
	}

	uint8_t* pagePtr = (uint8_t*)blockedPages[classSize].at(0);
	uint8_t* allocBlock = pageDescriptions[pagePtr].ptrToFirstFree;

	*allocBlock = false;
	pageDescriptions[pagePtr].blocksAmount -= 1;

	if (pageDescriptions[pagePtr].blocksAmount <= 0) {
		blockedPages[classSize].erase(std::find(blockedPages[classSize].begin(),
			blockedPages[classSize].end(), pagePtr));
	}
	else {
		pageDescriptions[pagePtr].ptrToFirstFree = (uint8_t*)anyFreeBlock(pagePtr, classSize);
	}
	return allocBlock;
}

void* Allocator::mem_alloc(size_t size) {
	if (size > PAGE_SIZE / 2) {
		return allocateByPages(size);
	}
	
	return allocateByBlocks(size);
}

void* Allocator::mem_realloc(void* address, size_t size)
{
	if (address == nullptr) {
		return mem_alloc(size);
	}

	void* ptr = address;

	size_t pageNumber = ((uint8_t*)address - memory) / PAGE_SIZE;
	uint8_t* pagePtr = memory + pageNumber * PAGE_SIZE;

	if (pageDescriptions[pagePtr].state == DividedIntoBlocks) {
		size_t bsize = getLogarithmByTwo(size);
		if (pageDescriptions[pagePtr].bsize == bsize) {
			return address;
		}

		ptr = mem_alloc(size);
		if (ptr) {
			mem_free(address);
			return ptr;
		}
		return address;
	}
	size_t oldSize = pageDescriptions[pagePtr].bsize;
	size_t oldPageNum = pageDescriptions[pagePtr].blocksAmount + 1;
	size_t newPageNum = amountOfPage(size);

	if (oldPageNum == newPageNum) {
		return ptr = address;
	}
	else if (oldPageNum > newPageNum) {
		uint8_t* currPage = pagePtr;
		uint8_t* nextPage;
		for (int i = 0; i < oldPageNum; i++) {
			nextPage = pageDescriptions[currPage].ptrToFirstFree;
			if (i >= newPageNum) {
				setPageDescription(currPage, Free, 0, 0, nullptr);

				freePages.push_back(currPage);
			}
			else {
				setPageDescription(currPage, MultiPageBlock, newPageNum * PAGE_SIZE, newPageNum - i - 1, newPageNum - 1 == i ? nullptr : nextPage);
			}
			currPage = nextPage;
		}

		if (size <= PAGE_SIZE / 2) {
			ptr = mem_alloc(size);
			for (int i = 0; i < size; i++) {
				*((uint8_t*)ptr + i + 1) = *((uint8_t*)address + i);
			}
		}

		return ptr;
	}
	size_t neededPage = newPageNum - oldPageNum;
	if (freePages.size() < neededPage) {
		return address;
	}

	uint8_t* currPage = pagePtr;
	for (int i = 1; i <= newPageNum; i++) {
		if (i >= oldPageNum) {
			pageDescriptions[currPage].ptrToFirstFree = newPageNum == i ? NULL : (uint8_t*)freePages[0];
			freePages.erase(freePages.begin());
		}
		setPageDescription(currPage, MultiPageBlock, newPageNum * PAGE_SIZE, newPageNum - i, pageDescriptions[currPage].ptrToFirstFree);

		currPage = pageDescriptions[currPage].ptrToFirstFree;
	}
	return address;
}

void Allocator::mem_free(void* address) {

	if (freePages.size() == countOfPage) {
		return;
	}

	size_t pageNumber = ((uint8_t*)address - memory) / PAGE_SIZE;
	uint8_t* pagePtr = memory + pageNumber * PAGE_SIZE;
	size_t numOfPage = pageDescriptions[pagePtr].blocksAmount + 1;

	if (pageDescriptions[pagePtr].state == MultiPageBlock) {
		uint8_t* currPage = pagePtr;
		int i = 0;
		while (i < numOfPage) {
			uint8_t* nextPage = pageDescriptions[currPage].ptrToFirstFree;
			setPageDescription(currPage, Free, 0, 0, nullptr);
			freePages.push_back(currPage);
			currPage = nextPage;
			i++;
		}
	}
	else if (pageDescriptions[pagePtr].state == DividedIntoBlocks) {
		size_t classSize = pageDescriptions[pagePtr].bsize;
		uint8_t* currBlock = pagePtr;

		for (int i = 0; i < PAGE_SIZE / classSize; i++) {
			if (currBlock == address) {
				*(uint8_t*)address = true;
				break;
			}
			currBlock += classSize;
		}
		pageDescriptions[pagePtr].blocksAmount += 1;

		if (pageDescriptions[pagePtr].blocksAmount == PAGE_SIZE / classSize) {
			pageDescriptions[pagePtr] = { Free, 0, 0, NULL };
			freePages.push_back(pagePtr);

			blockedPages[classSize].erase(std::find(blockedPages[classSize].begin(),
				blockedPages[classSize].end(), pagePtr));
		}
		if (pageDescriptions[pagePtr].blocksAmount == 1) {
			blockedPages[classSize].push_back(pagePtr);
		}
	}
}

void Allocator::mem_dump()
{
	uint8_t* currPage = memory;

	cout << "### mem_dump ###" << endl;
	cout << "============================================" << endl;

	for (int i = 0; i < countOfPage; i++) {
		PageDescription header = pageDescriptions[currPage];

		int paddingLeft = 2 + (i < 9 ? 1 : 0);
		cout << "Page" << setw(paddingLeft) << "#" << i + 1;
		cout << ". Address: " << (uint16_t*)currPage;
		cout << ". PageState: " << header.state;
		if (header.state == DividedIntoBlocks) {
			cout << ". ClassSize: " << header.bsize << endl << endl;

			uint8_t* currBlock = currPage;
			for (int j = 0; j < PAGE_SIZE / header.bsize; j++)
			{
				paddingLeft = 2 + (j < 9 ? 1 : 0);
				cout << "\tBlock" << setw(paddingLeft) << "#" << j + 1;
				cout << ". Address:" << (uint16_t*)currBlock;
				cout << ". IsFree: " << (bool)*currBlock << endl;
				currBlock += header.bsize;
			}
			cout << endl;
		}
		else if (header.state == MultiPageBlock) {
			cout << ". BlockSize: " << header.bsize;
			cout << ". Part #" << header.bsize / PAGE_SIZE - header.blocksAmount;
			cout << ". NextPage: " << (uint16_t*)header.ptrToFirstFree << endl;
		}
		else {
			cout << ". PageSize: " << PAGE_SIZE << endl;
		}

		currPage += PAGE_SIZE;
	}
	cout << "============================================" << endl;
}
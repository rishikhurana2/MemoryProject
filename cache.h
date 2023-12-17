#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "math.h"
#include <limits.h>
#include <iomanip>

using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for SA only
	int data; // the actual data stored in the cache/memory
	bool valid;
	int adr;
};

struct Stat
{
	int missL1;
	int missL2; 
	int accL1;
	int accL2;
	int accVic;
	int missVic;
	// add more stat if needed. Don't forget to initialize!
};

class cache {
private:
	cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
	cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row 
	// Add your Victim cache here ...
	cacheBlock LVic[VICTIM_SIZE];

	Stat myStat;

	//characteristic variables for addresses in L1
	int L1_offset_bits;
	int L1_index_bits;
	int L1_tag_bits;

	//characteristic variables for addresses in LVic
	int LVic_offset_bits;
	int LVic_index_bits;
	int LVic_tag_bits;

	//characteristic variables for addresses in L2
	int L2_offset_bits;
	int L2_index_bits;
	int L2_tag_bits;
public:
	cache();
	void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
	// add more functions here ...	
	int loadWord(int adr, int *myMem);
	void storeWord(int* data, int adr, int * myMem);

	//searching functions
	bool searchL1(int adr, int &data);
	bool searchLVic(int adr, int &data);
	bool searchL2(int adr, int &data);
	int searchMainMem(int adr, int *myMem); //always is a hit so return val no matter what

	//evict function
	void evict(int adr, int data);

	//helper functions for evict
	bool evictL1(int adr, int data, int &evicted_val, int &evicted_adr);
	bool evictLVic(int adr, int data, int &evicted_val, int &evicted_adr);
	bool evictL2(int adr, int data);

	//helper functions
	bitset<32> intToBin(int val);
	void setOffsetIndexTag(int adr, int offset_bits, int index_bits, int tag_bits, int &offset, int &index, int &tag);

	//get stats
	double getL1MissRate();
	double getL2MissRate();
	double getAAT();
};



#include "cache.h"

cache::cache() {
	for (int i=0; i<L1_CACHE_SETS; i++) {
		L1[i].valid = false; 
	}
	for (int i=0; i<L2_CACHE_SETS; i++)
		for (int j=0; j<L2_CACHE_WAYS; j++) {
			L2[i][j].valid = false; 
			L2[i][j].lru_position = 0;
	}

	// Do the same for Victim Cache ...
	for (int i = 0; i < VICTIM_SIZE; i++) {
		LVic[i].valid = false;
		LVic[i].lru_position = 0;
	}

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;

	//victim cache stats
	this->myStat.missVic = 0;
	this->myStat.accVic = 0;

	//characteristic variables for addresses in L1
	L1_offset_bits = 2;
	L1_index_bits = 4;
	L1_tag_bits = 32 - L1_offset_bits - L1_index_bits;

	//characteristic variables for addresses in LVic
	LVic_offset_bits = 2;
	LVic_index_bits = 0;
	LVic_tag_bits = 32 - LVic_offset_bits - LVic_index_bits;

	//characteristic variables for addresses in L2
	L2_offset_bits = 2;
	L2_index_bits = 4;
	L2_tag_bits = 32 - L2_offset_bits - L2_index_bits;
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	if (MemR) {
		int val = loadWord(adr, myMem);
	}
	if (MemW) {
		storeWord(data, adr, myMem);
	}
}

int cache::loadWord(int adr, int * myMem) {
	int data = 0;
	if (searchL1(adr, data)) { //First search L1, and if its found, then do nothing (no updates needed)
		return data;
	} else if (searchLVic(adr, data)) { //If not found, then search Victim Cache (and move to L1 and evict downward if needed)
		evict(adr, data);
		return data;
	} else if (searchL2(adr, data)) { //If not found, then search L2 (and move to L1 and evict downward if needed)
		evict(adr, data);
		return data;
	}
	//If data is not found in any cache, then search memory (and move to L1 and evict downward if needed)
	data = searchMainMem(adr, myMem);
	evict(adr, data);
	return data;
}

bool cache::searchL1(int adr, int &data) {
	//initializing integers to hold block offset, index, and tag
	int offset = -1;
	int index = -1;
	int tag = -1;

	//compute the integer values of offset, index, and tag
	setOffsetIndexTag(adr, L1_offset_bits, L1_index_bits, L1_tag_bits, offset, index, tag);

	//searching L1 tags to see if we have a hit
	if (L1[index].valid && L1[index].tag == tag) {
		this->myStat.accL1++;
		data = L1[index].data;
		return true;
	}

	this->myStat.missL1++;
	return false;
}

bool cache::searchLVic(int adr, int &data){
	//initializing integers for block offset, index, and tag values
	int offset = -1;
	int index = -1;
	int tag = -1;

	//compute the values of offset, index, and tag
	setOffsetIndexTag(adr, LVic_offset_bits, LVic_index_bits, LVic_tag_bits, offset, index, tag);

	//Searching victim cache
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (LVic[i].valid && LVic[i].tag == tag) {
			this->myStat.accVic++;
			data = LVic[i].data;
			return true;
		}
	}

	this->myStat.missVic++;
	return false;
}

bool cache::searchL2(int adr, int &data) {
	//initializing integers for block offset, index, and tag
	int offset = -1;
	int index = -1;
	int tag = -1;

	//compute the integer values of offset, index, and tag
	setOffsetIndexTag(adr, L2_offset_bits, L2_index_bits, L2_tag_bits, offset, index, tag);

	//searching L2 cache
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[index][i].valid && L2[index][i].tag == tag) {
			this->myStat.accL2++;
			data = L2[index][i].data;
			return true;
		}
	}

	this->myStat.missL2++;
	return false;
}

int cache::searchMainMem(int adr, int *myMem) {
	return myMem[adr];
}

void cache::storeWord(int * data, int adr, int * myMem) {
	//update data in memory
	myMem[adr] = *data;

	//compute characteristc values for each cache
	int L1_offset = -1;
	int L1_index = -1;
	int L1_tag = -1;

	int LVic_offset = -1;
	int LVic_index = -1;
	int LVic_tag = -1;

	int L2_offset = -1;
	int L2_index = -1;
	int L2_tag = -1;

	setOffsetIndexTag(adr, L1_offset_bits, L1_index_bits, L1_tag_bits, L1_offset, L1_index, L1_tag);
	setOffsetIndexTag(adr, LVic_offset_bits, LVic_index_bits, LVic_tag_bits, LVic_offset, LVic_index, LVic_tag);
	setOffsetIndexTag(adr, L2_offset_bits, L2_index_bits, L2_tag_bits, L2_offset, L2_index, L2_tag);

	//update the data in cache
	L1[L1_index].data = *data;

	//update the data in the victim cache
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (LVic[i].tag == LVic_tag) {
			LVic[i].data = *data;
		} 
	}
	
	//update the data in the L2 cache
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[L2_index][i].tag == L2_tag) {
			L2[L2_index][i].data = *data;
		}
	}
}

void cache::evict(int adr, int data) {
	//evict outline:
		//evict from L1 and send to Victim (remove element at specified tag)
		//evict from Victim and send to L2 (using LRU)
		//Evict from L2 and send to main memory (using LRU)
	
	//First we place data in L1 and evict (if needed) -- and grab the val,adr of evicted value
	int L1_evicted_val = 0; //will store the evicted value from L1
	int L1_evicted_address = 0; //will store the address of the evicted value from L1
	bool evicted_L1 = evictL1(adr, data, L1_evicted_val, L1_evicted_address);
	if (!evicted_L1) { //if a value did not need to be evicted, then we are done
		return;
	}

	//move the value that was evicted from L1 into Victim cache (and evict from Victim if needed) -- and grab the val,adr of the evicted value from victim cache
	int LVic_evicted_val = 0; //will store the evicted value
	int LVic_evicted_adr = 0; //will store the address of the evicted value
	bool evicted_LVic = evictLVic(L1_evicted_address, L1_evicted_val, LVic_evicted_val, LVic_evicted_adr);
	if (!evicted_LVic) { //if a value did not need to be evicted, then we are done
		return;
	}

	//move the value that was evicted from Victim cahce into L2 (and evict from L2 if needed)
	int L2_evicted_val = 0;
	bool evicted_L2 = evictL2(LVic_evicted_adr, LVic_evicted_val);
	if (!evicted_L2) {
		return;
	}

}

bool cache::evictL1(int adr, int data, int &evicted_val, int &evicted_adr) {
	int L1_offset = -1;
	int L1_index = -1;
	int L1_tag = -1;

	//compute the offset, index, and tag values for L1 Cache
	setOffsetIndexTag(adr, L1_offset_bits, L1_index_bits, L1_tag_bits, L1_offset, L1_index, L1_tag);

	//If L1 at the specifed index hasn't been initialized, then we have space and we can just store
	if (L1[L1_index].valid == false) {
		L1[L1_index].data = data;
		L1[L1_index].tag = L1_tag;
		L1[L1_index].valid = true;
		L1[L1_index].adr = adr;
		return false;
	}

	//If the L1 at the specified index has been intialize, we evict and store the evicted value and its address
	evicted_adr = L1[L1_index].adr;
	evicted_val = L1[L1_index].data;
	L1[L1_index].tag = L1_tag;
	L1[L1_index].data = data;
	L1[L1_index].adr = adr;

	return true;
}

bool cache::evictLVic(int adr, int data, int &evicted_val, int &evicted_adr) {
	int LVic_offset = -1;
	int LVic_index = -1;
	int LVic_tag = -1;

	setOffsetIndexTag(adr, LVic_offset_bits, LVic_index_bits, LVic_tag_bits, LVic_offset,LVic_index, LVic_tag);

	//If the victim cache has space, then we can just store and exit
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (LVic[i].valid == false) { //find empty space and store
			LVic[i].tag = LVic_tag;
			LVic[i].adr = adr;
			LVic[i].data = data;
			LVic[i].valid = true;
			LVic[i].lru_position = 0;
			for (int j = 0; j < VICTIM_SIZE; j++) { //update lru of all other valid entries
				if (j != i && LVic[j].valid) {
					LVic[j].lru_position++;
				}
			}
			return false;
		}
	}

	//If the victim cache has no space, then we evict using LRU and store the val,adr of the evicted entry

	//finding the entry with the maximum lru
	int max_lru = INT_MIN;
	int max_lru_index = 0;
	for (int i = 0; i < VICTIM_SIZE; i++) {
		if (LVic[i].lru_position > max_lru) {
			max_lru = LVic[i].lru_position;
			max_lru_index = i;
		}
	}

	//evicting the entry that has the max lru and storing the data, adr that was there
	evicted_val = LVic[max_lru_index].data;
	evicted_adr = LVic[max_lru_index].adr;
	LVic[max_lru_index].data = data;
	LVic[max_lru_index].tag = LVic_tag;
	LVic[max_lru_index].lru_position = 0;
	LVic[max_lru_index].adr = adr;

	//updating the lru of the other elements
	for (int j = 0; j < VICTIM_SIZE; j++) {
		if (LVic[j].lru_position < max_lru && j != max_lru_index) {
			LVic[j].lru_position++;
		}
	}
	return true;
}

bool cache::evictL2(int adr, int data) {
	int L2_offset = -1;
	int L2_index = -1;
	int L2_tag = -1;

	//compute the offset, index, and tag values for L1 Cache
	setOffsetIndexTag(adr, L2_offset_bits, L2_index_bits, L2_tag_bits, L2_offset, L2_index, L2_tag);

	//If there is space at the index specified, then we can store
	for (int j = 0; j < L2_CACHE_WAYS; j++) {
		if (L2[L2_index][j].valid == false) { //finding an empty space
			L2[L2_index][j].tag = L2_tag;
			L2[L2_index][j].data = data;
			L2[L2_index][j].valid = true;
			L2[L2_index][j].lru_position = 0;

			for (int k = 0; k < L2_CACHE_WAYS; k++) { //update the lru of all other valid entries
				if (k != j && L2[L2_index][k].valid) {
					L2[L2_index][j].lru_position++;
				}
			}
			return false;
		}
	}

	//If L2 has no space at the specified index, then we choose the element with max LRU position to evict

	//finding the entry with the maximum LRU at the specified index
	int max_lru = INT_MIN;
	int max_lru_index = 0;
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[L2_index][i].lru_position > max_lru) {
			max_lru = L2[L2_index][i].lru_position;
			max_lru_index = i;
		}
	}

	//evicting the entry with max lru (no need to store element that was there because it is removed)
	L2[L2_index][max_lru_index].data = data;
	L2[L2_index][max_lru_index].tag = L2_tag;
	L2[L2_index][max_lru_index].lru_position = 0;

	//updating the lru of all other elements
	for (int i = 0; i < L2_CACHE_WAYS; i++) {
		if (L2[L2_index][i].lru_position < max_lru && i != max_lru_index) {
			L2[L2_index][i].lru_position++;
		}
	}
	return true;
}

bitset<32> cache::intToBin(int val) {
	//helper function to convert from int to binary
	return bitset<32>(val);
}

void cache::setOffsetIndexTag(int adr, int offset_bits, int index_bits, int tag_bits, int &offset, int &index, int &tag) {
	//helper function to parse and address and return the offset, index, and tag given the amount of bits for the offset, index, and tag

	bitset<32> adr_in_binary = intToBin(adr);

	bitset<32> offset_in_binary;
	bitset<32> index_in_binary;
	bitset<32> tag_in_binary;

	for (int i = 0; i < offset_bits; i++) {
		offset_in_binary[i] = adr_in_binary[i]; //offset occurs first
	}
	for (int i = offset_bits; i < offset_bits + index_bits; i++) {
		index_in_binary[i - offset_bits] = adr_in_binary[i]; //then index occurs
	}
	for (int i = offset_bits + index_bits; i < 32; i++) {
		tag_in_binary[i - (offset_bits + index_bits)] = adr_in_binary[i]; //then the final set of bits is the tag
	}

	//set offset, index, and tag
	offset = (int)offset_in_binary.to_ulong(); 
	index = (int) index_in_binary.to_ulong();
	tag = (int) tag_in_binary.to_ulong();
}

//functions for getting stats
double cache::getL1MissRate() {
	double L1MissRate = ((double) (this->myStat.missL1))/((double)( this->myStat.accL1 + this->myStat.missL1));
	return L1MissRate;
}

double cache::getL2MissRate() {
	double L2MissRate = ((double)(this->myStat.missL2))/((double)(this->myStat.accL2 + this->myStat.missL2));
	return L2MissRate;
}

double cache::getAAT() {
	double l1_rate = getL1MissRate();
	double l2_rate = getL2MissRate();
	double victim_rate = ((double)(this->myStat.missVic))/((double)(this->myStat.accVic + this->myStat.missVic));
	return 1 + l1_rate * (1 + victim_rate * (8 + l2_rate*100));
}
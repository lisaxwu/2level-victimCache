#include "cachesim.hpp"
#include <cstdlib>
#include <stdlib.h>

#ifdef CCOMPILER
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#else
#include <cstdio>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#endif

struct cache_entry{
    uint64_t tag;
    int valid;
    int dirty;
};

struct cache_entry* Cache_1;  //Cache table [bps*entry]
int* URL_1;                   //URL index v1v2v3v4 table [bps*entry]
uint64_t C_1;
uint64_t S_1;
uint64_t B_1;
uint64_t entry_1;
uint64_t bps_1;

struct cache_entry* Cache_2;  //Cache table [bps*entry]
int* URL_2;                   //URL index v1v2v3v4 table [bps*entry]
uint64_t C_2;
uint64_t S_2;
uint64_t B_2;
uint64_t entry_2;
uint64_t bps_2;

struct victim_entry{
	uint64_t tag;
	struct victim_entry* next;
};

struct victim_entry* V_Cache;     //a circular linked list
uint64_t V;

uint64_t Cache_get_index(int L, uint64_t addr){
	if (L==1){
		uint64_t index = addr >> B_1;
		uint64_t tmp = ((index >> (C_1-S_1-B_1))<<(C_1-S_1-B_1));
		index = index -tmp;
	//	printf("addr %x : addr >> B_1 %x : L1 index %x",addr,(addr>>B_1),index);
		return index;
	}
	else{
		uint64_t index = addr >> B_2;
		uint64_t tmp = ((index >> (C_2-S_2-B_2))<<(C_2-S_2-B_2));
		index = index -tmp;
		return index;
	}
	
}

uint64_t Cache_get_tag(int L, uint64_t addr){
	if (L==1){
		uint64_t tag = addr >> (C_1-S_1);
		return tag;
	}
	else{
		uint64_t tag = addr >> (C_2-S_2);
		return tag;
	}
	
}

uint64_t C1_cat_addr(uint64_t index, uint64_t tag){
	uint64_t addr = (tag << (C_1-S_1)) + (index << (B_1));
	return addr;
}

uint64_t C1_toC2_index(uint64_t index, uint64_t tag){
	uint64_t addr = C1_cat_addr(index,tag);
	return Cache_get_index(2,addr);
}

uint64_t C1_toC2_tag(uint64_t index, uint64_t tag){
	uint64_t addr = C1_cat_addr(index,tag);
	return Cache_get_tag(2,addr);
}


/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c1 The total number of bytes for data storage in L1 is 2^c1
 * @b1 The size of L1's blocks in bytes: 2^b1-byte blocks.
 * @s1 The number of blocks in each set of L1: 2^s1 blocks per set.
 * @v Victim Cache's total number of blocks (blocks are of size 2^b1).
 *    v is in [0, 4].
 * @c2 The total number of bytes for data storage in L2 is 2^c2
 * @b2 The size of L2's blocks in bytes: 2^b2-byte blocks.
 * @s2 The number of blocks in each set of L2: 2^s2 blocks per set.
 * Note: c2 >= c1, b2 >= b1 and s2 >= s1.
 */
void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1, uint64_t v,
                 uint64_t c2, uint64_t b2, uint64_t s2) {
	C_1=c1;
	B_1=b1;
	S_1=s1;
	V=v;
	C_2=c2;
	B_2=b2;
	S_2=s2;

	entry_1 = 1<<(C_1-B_1-S_1);
	bps_1 = 1<<(S_1);
	entry_2 = 1<<(C_2-B_2-S_2);
	bps_2 = 1<<(S_2);

	Cache_1 = (struct cache_entry*)malloc(sizeof(struct cache_entry)*entry_1*bps_1);
	Cache_2 = (struct cache_entry*)malloc(sizeof(struct cache_entry)*entry_2*bps_2);

	for(uint i=0;i<entry_1;i++)
        for(uint j=0;j<bps_1;j++)
        {
            Cache_1[i*bps_1+j].valid=0;
            Cache_1[i*bps_1+j].dirty=0;
            Cache_1[i*bps_1+j].tag=0;
        }
    for(uint i=0;i<entry_2;i++)
        for(uint j=0;j<bps_2;j++)
        {
            Cache_2[i*bps_2+j].valid=0;
            Cache_2[i*bps_2+j].dirty=0;
            Cache_2[i*bps_2+j].tag=0;
        }

    URL_1 = (int *)malloc(sizeof(int)*entry_1*bps_1);
    URL_2 = (int *)malloc(sizeof(int)*entry_2*bps_2);

    for(uint i=0;i<entry_1;i++)
        for(uint j=0;j<bps_1;j++)
        {
            URL_1[i*bps_1+j]=0;            // 1--N     24356 come 5 then 52436
        }

    for(uint i=0;i<entry_2;i++)
        for(uint j=0;j<bps_2;j++)
        {
            URL_2[i*bps_2+j]=0;
        }

    if (V>0){
	    struct victim_entry* tmp;
	    V_Cache = (struct victim_entry*)malloc(sizeof(struct victim_entry));
	    tmp=V_Cache;
	    for(uint i=0; i<V-1; i++){
	    	tmp->tag = 0;
	    	tmp->next = (struct victim_entry*)malloc(sizeof(struct victim_entry));
	    	tmp = tmp->next;
		}
		tmp->next = V_Cache;              //V_Cache is the head
	}
	else{
		V_Cache=NULL;
	}

}

void update_URL1(uint64_t row,uint64_t col) //newest visit row col
{
    int tmp[bps_1];                //copy the row in URL
    for(uint j=0;j<bps_1;j++)
    {
        tmp[j]=URL_1[int(row*bps_1+j)];
    }
    
    URL_1[int(row*bps_1+0)]=col+1;    //URL contents start from 1 to 2^s
    
    int diff=1;            //URL[row*bps+j]=tmp[j-diff];
    for(uint j=1;j<bps_1;j++)
    {
        if(tmp[j-diff]==(col+1))
        {
            diff=0;
        }
        URL_1[int(row*bps_1+j)]=tmp[j-diff];
    }    

}

void update_URL2(uint64_t row,uint64_t col) //newest visit row col
{
    int tmp[bps_2];                //copy the row in URL
    for(uint j=0;j<bps_2;j++)
    {
        tmp[j]=URL_2[int(row*bps_2+j)];
    }
    
    URL_2[int(row*bps_2+0)]=col+1;    //URL contents start from 1 to 2^s
    
    int diff=1;            //URL[row*bps+j]=tmp[j-diff];
    for(uint j=1;j<bps_2;j++)
    {
        if(tmp[j-diff]==(col+1))
        {
            diff=0;
        }
        URL_2[int(row*bps_2+j)]=tmp[j-diff];
    }

}


int L1_hit(uint64_t index,uint64_t tag){

	int col = -1;

	for(uint i=0;i<bps_1;i++){

		if( Cache_1[int(index*bps_1+i)].tag == tag){
			if (Cache_1[int(index*bps_1+i)].valid == 1){
				col=i;
				break;
			}
		}
	}

	return col;
}

int L2_hit(uint64_t index,uint64_t tag){

	int col = -1 ;
	for(uint i=0;i<bps_2;i++){

		if( Cache_2[int(index*bps_2+i)].tag == tag){
			if (Cache_2[int(index*bps_2+i)].valid == 1){
				col = i;
				break;
			}
		}
	}
	return col;
}

struct victim_entry* VC_hit(uint64_t index,uint64_t tag){

	uint64_t tagv = C1_cat_addr(index, tag)>>(B_1);
	struct victim_entry* tmp = V_Cache;
	int hit=0;
	for (uint i=0; i<V; i++){
		if((tmp->tag) == tagv){
			hit = 1;
			break;
		}
		tmp=tmp->next;
	}
	if (hit){
		return tmp;
	}
	else{
		return NULL;
	}

}


int L1_find_victim(uint64_t index){
	int base = index*bps_1;
	uint col;
	for(col = 0; col<bps_1 ; col++){
		if ( Cache_1[base+col].valid ==0 ){
			return col;
		}
	}
	col=URL_1[base+bps_1-1]-1;
	return col;
}

int L2_find_victim(uint64_t index){
	int base = index*bps_2;
	uint col;
	for(col = 0; col<bps_2 ; col++){
		if ( Cache_2[base+col].valid ==0 ){
			return col;
		}
	}
	col=URL_2[base+bps_2-1]-1;
	return col;
}


void write_back_toL2(uint64_t L2_ind,uint64_t L2_tag,struct cache_stats_t* p_stats){

	p_stats->accesses_l2 +=1;
	p_stats->write_back_l1 += 1;

	int col=L2_hit(L2_ind,L2_tag);
	if(col>=0){
		;
	}
	else{
		p_stats->write_misses_l2 +=1;
		col=L2_find_victim(L2_ind);
		if(Cache_2[int(L2_ind*bps_2+col)].valid == 1){
			if(Cache_2[int(L2_ind*bps_2+col)].dirty == 1){
				p_stats->write_back_l2 += 1;
			}
		}
	}

	Cache_2[int(L2_ind*bps_2+col)].valid=1;
	Cache_2[int(L2_ind*bps_2+col)].dirty=1;
	Cache_2[int(L2_ind*bps_2+col)].tag=L2_tag;
	update_URL2(L2_ind,col);

}

//the only difference is the dirty bit
void write_main_toL2(uint64_t L2_ind,uint64_t L2_tag,struct cache_stats_t* p_stats){

	int col=L2_find_victim(L2_ind);

	if(Cache_2[int(L2_ind*bps_2+col)].valid == 1){
		if(Cache_2[int(L2_ind*bps_2+col)].dirty == 1){
			p_stats->write_back_l2 += 1;
		}
	}
	Cache_2[int(L2_ind*bps_2+col)].valid=1;
	Cache_2[int(L2_ind*bps_2+col)].dirty=0;
	Cache_2[int(L2_ind*bps_2+col)].tag=L2_tag;
	update_URL2(L2_ind,col);
}
//since it is called when 
//read access: write clean data back to L1 from VC or L2, dirty=0
//write access: dirty=1
//vc hit vctype=0, else =1
void miss_write_L1(uint64_t index,uint64_t tag,int dirty,int vchit,struct cache_stats_t* p_stats){

	int col = L1_find_victim(index);

	if (Cache_1[int(index*bps_1+col)].valid==1){
		if (Cache_1[int(index*bps_1+col)].dirty==1){
			//write_back_to L2
			uint64_t L2_ind = C1_toC2_index(index,Cache_1[int(index*bps_1+col)].tag);
			uint64_t L2_tag = C1_toC2_tag(index,Cache_1[int(index*bps_1+col)].tag);;
			write_back_toL2(L2_ind,L2_tag,p_stats);
		}
		if(V>0){
			//update vc
			uint64_t out_tag = Cache_1[int(index*bps_1+col)].tag;
			uint64_t out_tagv= C1_cat_addr(index, out_tag)>>(B_1);
			uint64_t this_tagv = C1_cat_addr(index, tag)>>(B_1);
			if (vchit==1){ //when vc hit
				//remove the required block from vc
				//add the pushed-out block right before V_Cache
				//printf("miss write vchit############\n");
				if(V_Cache->tag == this_tagv){
					V_Cache->tag = out_tagv;
					V_Cache=V_Cache->next;
				}
				else{
					struct victim_entry* tmp1 = V_Cache;
					struct victim_entry* tmp2 = tmp1->next;
					while(tmp2 != V_Cache){
						if (tmp2->tag==this_tagv){
							break;
						}
						tmp1=tmp2;
						tmp2=tmp2->next;
					}
					tmp1->next=tmp2->next;
					free(tmp2);

					tmp2=(struct victim_entry*)malloc(sizeof(struct victim_entry));
					tmp2->tag = out_tagv;
					tmp2->next = V_Cache;

					tmp1=V_Cache;
					while (tmp1->next != V_Cache){
						tmp1=tmp1->next;
					}
					tmp1->next=tmp2;

				}	

			}
			else{ //vc miss
				//add the pushed-out block right at V_Cache
				//move V_Cache
				//printf("miss write not #vchit\n");
				V_Cache->tag = out_tagv;
				V_Cache=V_Cache->next;

			}
		}
	}
	//update L1
	Cache_1[int(index*bps_1+col)].valid=1;
	Cache_1[int(index*bps_1+col)].dirty=dirty;
	Cache_1[int(index*bps_1+col)].tag=tag;
	update_URL1(index,col);
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
 //         printf("point1\n");
void cache_access(char type, uint64_t arg, struct cache_stats_t* p_stats) {
	
	
	p_stats->accesses += 1;

	uint64_t row1 = Cache_get_index(1,arg);
	uint64_t tag1 = Cache_get_tag(1, arg);
	uint64_t row2 = Cache_get_index(2,arg);
	uint64_t tag2 = Cache_get_tag(2, arg);

    // --------------------read ----------------------------------
	if (type == READ){
		p_stats->reads += 1;
		//-------- if L1 hit, just update URL ------
		int col = L1_hit(row1,tag1);
		if(col>=0){
			update_URL1(row1,col);
			if(V>0) printf("H1****\n"); else printf("H1**\n");
		}
		else{  
		//---------if L1 miss---------
			p_stats->read_misses_l1 += 1;
			// ------victim cache exists--------
			if(V>0){

				p_stats->accesses_vc += 1;

				struct victim_entry* tmp = VC_hit(row1,tag1);
				if (tmp != NULL){
					// VC hit 	
					printf("M1HV**\n");
					p_stats->victim_hits +=1;
					miss_write_L1(row1,tag1,0,1,p_stats);
				}
				else{    //go for L2
					p_stats->accesses_l2 +=1;
					col = L2_hit(row2,tag2);
					if(col>=0){
						//L2 hit
						printf("M1MVH2\n");
						update_URL2(row2,col);
						miss_write_L1(row1,tag1,0,0,p_stats);
					}
					else{
						//L2 miss
						printf("M1MVM2\n");
						p_stats->read_misses_l2 += 1;
						write_main_toL2(row2,tag2,p_stats);
						miss_write_L1(row1,tag1,0,0,p_stats);
					}

				}
			}
			// -----no victim cache, go for L2-----------
			else{
				p_stats->accesses_l2 +=1;
				col = L2_hit(row2,tag2);
				if(col>=0){
					//L2 hit
					printf("M1H2\n");
					update_URL2(row2,col);
					miss_write_L1(row1,tag1,0,0,p_stats);
				}
				else{
					//L2 miss
					printf("M1M2\n");
					p_stats->read_misses_l2 += 1;
					write_main_toL2(row2,tag2,p_stats);
					miss_write_L1(row1,tag1,0,0,p_stats);
				}
			}

		}
	}
	//-------------------- write -----------------------------------
	else{
		p_stats->writes += 1;
		//-------- if L1 hit, dirty & update URL ------
		int col = L1_hit(row1,tag1);
		if(col>=0){
			Cache_1[int(row1*bps_1+col)].dirty=1;
			update_URL1(row1,col);
			if(V>0) printf("H1****\n"); else printf("H1**\n");
		}
		//--------L1 miss---------
		else{
			p_stats->write_misses_l1 += 1;
			//-----VC exist---
			if (V>0){
				p_stats->accesses_vc += 1;
				struct victim_entry* tmp = VC_hit(row1,tag1);
				if (tmp != NULL){
					// VC hit
					printf("M1HV**\n");
					p_stats->victim_hits +=1;
					miss_write_L1(row1,tag1,1,1,p_stats);
				}
				else{
					p_stats->accesses_l2 +=1;
					col = L2_hit(row2,tag2);
					if(col>=0){
						//L2 hit
						printf("M1MVH2\n");
						update_URL2(row2,col);
						miss_write_L1(row1,tag1,1,0,p_stats);
					}
					else{
						//L2 miss
						printf("M1MVM2\n");
						p_stats->read_misses_l2 += 1;
						write_main_toL2(row2,tag2,p_stats);
						miss_write_L1(row1,tag1,1,0,p_stats);
					}
					
				}

			}
			//No VC, currently wirte to L1 Only
			else{
				//printf("node2\n");
				p_stats->accesses_l2 +=1;
				col = L2_hit(row2,tag2);
					if(col>=0){
						//L2 hit
						printf("M1H2\n");
						update_URL2(row2,col);
						miss_write_L1(row1,tag1,1,0,p_stats);
					}
					else{
						//L2 miss
						printf("M1M2\n");
						write_main_toL2(row2,tag2,p_stats);
						p_stats->read_misses_l2 += 1;
						miss_write_L1(row1,tag1,1,0,p_stats);
					}
			}

		}


	}
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(struct cache_stats_t *p_stats) {

	struct victim_entry* tmp = V_Cache;
	for(int i=0;i<V;i++){
		V_Cache=V_Cache->next;
		free(tmp);
		tmp=V_Cache;
	}
    p_stats->avg_access_time_l1=0;
    double ht1 = 2+ 0.2*S_1;
    double ht2 = 4+ 0.4*S_2;
    double MRate1 = (p_stats->read_misses_l1+p_stats->write_misses_l1+0.0)/p_stats->accesses;
    double MRate2 = (p_stats->read_misses_l2+p_stats->write_misses_l2+0.0)/p_stats->accesses_l2;
    double AAT_l2 = ht2+MRate2*500.0;
    double MRatevc = (p_stats->accesses_vc-p_stats->victim_hits+0.0)/p_stats->accesses_vc;
    if (V==0){
    	p_stats->avg_access_time_l1 = ht1 + MRate1*AAT_l2;
    }
    else{
    	p_stats->avg_access_time_l1 = ht1 + MRate1*MRatevc*AAT_l2;
    }
}

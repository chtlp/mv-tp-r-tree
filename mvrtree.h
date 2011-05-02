
//////////////////////////////////////////////////////////
//This is the implementation of MVR-tree (Multiversion R-tree).
//The codes in this version are not decorated, so not ready for
//distribution, but for internal test only.
//This version is coded by TAO Yufei (taoyf@ust.hk), Sept 2000.
//
//Last modified Sept. 19, 2000
#ifndef __MVRRTREE
#define __MVRRTREE
//////////////////////////////////////////////////////////////////////////////
//enum
//////////////////////////////////////////////////////////////////////////////
extern enum INSERTABLE {YES, NO, FULL, NOBRANCH};
//////////////////////////////////////////////////////////////////////////////
#include "blockmem.h"
#include "mvrlinlist.h"
#include "blk_file.h"
#include "cache.h"
#include "rtreeheader.h"
#include "mvrtnode.h"
#include "mvrtentry.h"
#include "blockmem.h"
#include "para.h"
//////////////////////////////////////////////////////////////////////////////
//structures
//////////////////////////////////////////////////////////////////////////////
struct RTTable
{
    int *rno;                //This records which roots are stored
	int *bno;                //This records which blocks roots are stored
	int rtnum;               //This records the total number of roots
    RTTable()
	{
		rno=new int[NOWTIME];
		bno=new int[NOWTIME];
		rtnum=0;
	}
	~RTTable()
	{
		delete [] rno;
		delete [] bno;
	}
};

struct TmpEntry
{
	int son;
	int sttime, edtime;
	float bounces[6];
};
//////////////////////////////////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////////////////////////////////

class MVRLinList;
class SortedMVRLinList;
class Cache;
class MVREntry;
class MVRTNode;
class MVRTree;
class BlockMem;

class MVRTree : public AbsRTree
{
public:
	bool *re_level;                      // if re_level[i] is TRUE,
                                         // there was a reinsert on level i
	bool flushing;						 // indicate whether the entry table is being flushed
	char *header;
	char *rtafname;                      // The name of the root table file
	int akt;                             // # of actually got data (get_next)
    int dimension;                       // dimension of the data's
	int etablecount;
	int most_recent_time;                //The version of the last entry inserted
    int num_of_data;	                 // # of stored data
    int num_of_dnodes;	                 // # of stored data pages
    int num_of_inodes;	                 // # of stored directory pages

    int root;                            // block # of current root node
	int wasted;                          // Used for testing purpose in rangequery

	//*****These vars record number of times for different overflows****
	int purekey;       //pure key split
	int purever;       //pure version split
	int verkey;        //version split and then a key split
	int verunder;      //strong version underflow
	int keyver;        //key split after a version copy
	int asplit;        //avoided a split
	int leafvc;        //leaf version copy
	int nonleafvc;     //nonleaf version copy
	//*****These vars record number of times for different underflows***
	int nerase;
	int wunder;
	//******************************************************************
	BlockMem *blockmem;
	TmpEntry *entrytable;
	RTTable rtable;                      // This is the root table
    MVRLinList *reinsert_list;           // data entries to reinsert
	MVRLinList *delete_list;
	BlockFile *file;	                 // storage manager for harddisc blocks
    Cache *cache;                        // LRU cache for managing blocks in memory
    MVRTNode *root_ptr;                  // pointer to the current root node
	FILE *def;

    //<-----Functions----->

    MVRTree(char *fname, char *rtfile, int _b_length, Cache* c, int _dimension);
    MVRTree(char *fname, char *rtfile, Cache* c);
    virtual ~MVRTree();
	
	void AddNodetoEtable(MVRTNode *rn);
	void addtoetable(MVREntry *e);
	bool CheckInstability(MVREntry *e, int oblock, bool asign);
	void delete_entry(MVREntry *e);      //This function deletes the entry e
	   //Note that the sttime field does not matter
	   //the edtime field shoulbe be the time e is deleted
	   //e must be currently an alive object
	bool delfrometable(MVREntry *e);
	void DelMVRNode(MVRTNode **mr);
	bool FindLeaf(MVREntry *e);          // This function tries to find the specified
	   //entry.  If found, return true.  False otherwise.
	void FlushEtable();
	int get_root_ts();                   // This function returns the time stamp of the root
	int get_num()                        // returns # of stored data
		{ return num_of_data; }
    void insert(MVREntry *d, bool afterdel);            // inserts new data into tree
	                                     //afterdel denotes whether the insert is called after a deletion
	void InsertDelList(bool asign);
	void increase_node_count(int sign);  // Increament the counting of dnode and inode
	                                     // sign=1 => inode, sign=0 dnode
	void load_root(int ts);              // loads the root of the cloest tree to ts
	MVRTNode * NewMVRNode(int reqblock=-1);
    void rangeQuery(float *mbr, SortedMVRLinList *res, char *rec, char *dupblock);	// loads root_node into memory
	//Note that mbr is of the size 7 (including 2 spatial intervals and 1 time interval
	//as well as another field used to optimize the query
    void read_root_table();   // This function reads the root table from the file
	void read_header(char *fname);       // reads Rtree header
	bool Reinsert(MVREntry *e, int oblock, bool asign);
	void ResetEntrytable();
	void update_root_table(int ts, int bno);  //This function updates the tree corresponding to ts
	                                     // if bno is negative, it means creating a new record for ts 
    void write_header(char *buffer);     // writes Rtree header
	void write_root_table();      // This function writes the root table to the file
};

#endif // __MVRTREE

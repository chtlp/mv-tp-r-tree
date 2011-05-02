#ifndef __E3RTREE
#define __E3RTREE

#include "e3rlinlist.h"
#include "blk_file.h"
#include "cache.h"
#include "rtreeheader.h"
/////////////////////////////////////////////////////////////////////////////
class E3RLinList;
class SortedE3RLinList;
class Cache;
/////////////////////////////////////////////////////////////////////////////
class E3REntry : public AbsEntry
{
    friend class E3RTNode;
    friend class E3RTree;
public:
    
    int son;                          // block # of son
	  //This variable is no longer necessary because it has been defined in the 
	  //rtree-header
    int dimension;                      // dimension of the box
	int level;
    float *bounces;                     // pointer to box
	E3RTree *my_tree;                     // pointer to my R-tree
    E3RTNode *son_ptr;                    // pointer to son if in main mem.

	//<---functions--->

	E3REntry(int dimension = E3RTREEDIMENSION, E3RTree *rt = NULL);
    virtual ~E3REntry();


    bool check_valid();
	int get_size();                     // returns amount of needed buffer space
	E3RTNode *get_son();                  // returns pointer to the son,
	void read_from_buffer(char *buffer);// reads data from buffer
    SECTION section(float *mbr);        // tests, if mbr intersects the box
    bool section_circle(float *center, float radius);
    void write_to_buffer(char *buffer); // writes data to buffer


    virtual E3REntry & operator = (E3REntry &_d);
	bool operator == (E3REntry &_d);
};

class E3RTNode : public AbsRTNode
{
    friend class E3RTree;
public:
	bool dirty;                         // TRUE, if node has to be written
    char level;                         // level of the node in the tree (0 means leaf)
	int block;                          // disc block
    int capacity;                       // max. # of entries
	int dimension;
    int num_entries;                    // # of used entries
	E3REntry *entries;                     // array of entries
	E3RTree *my_tree;                     // pointer to R-tree
    
    //<---functions--->
	E3RTNode(E3RTree *rt);
    E3RTNode(E3RTree *rt, int _block);
    virtual ~E3RTNode();

    int choose_subtree(float *brm);     // chooses best subtree for insertion
	R_DELETE delete_entry(E3REntry *e);
	void enter(E3REntry *de);              // inserts new entry
	bool FindLeaf(E3REntry *e);
	int get_headersize()
	{return sizeof(char) + sizeof(int);}
	float *get_mbr();                   // returns mbr enclosing whole page
	int get_num_of_data();              // returns number of data entries
	R_OVERFLOW insert(E3REntry *d, E3RTNode **sn);
                                        // inserts d recursively, if there
                                        // occurs a split, FALSE will be
                                        // returned and in sn a
                                        // pointer to the new node
	bool is_data_node() { return (level==0) ? TRUE : FALSE ;};
	void print();                       // prints rectangles
	void rangeQuery(float *mbr, SortedE3RLinList *res, char *rec, int *levelcount);
    void read_from_buffer(char *buffer);// reads data from buffer
	void split(E3RTNode *sn);             // splits directory page to *this and sn
	int split(float **mbr, int **distribution);
                                        // splits an Array of mbr's into 2
                                        // and returns in distribution
                                        // which *m mbr's are to be
                                        // moved
    void write_to_buffer(char *buffer); // writes data to buffer
};

class E3RTree : public AbsRTree
{
public:
    friend class E3RTNode;

    
	bool *re_level;                      // if re_level[i] is TRUE,
                                         // there was a reinsert on level i
    bool root_is_data;                   // TRUE, if root is a data page
	char *header;
	int akt;                             // # of actually got data (get_next)
    int dimension;                       // dimension of the data's
    int num_of_data;	                 // # of stored data
    int num_of_dnodes;	                 // # of stored data pages
    int num_of_inodes;	                 // # of stored directory pages
	int root;                            // block # of root node
	int *splitcount; 
	  //This variable is an array, which stores the number of splits for
	  //each dimension.  So the size of the array is the same as the dimension

    BlockFile *file;	                 // storage manager for harddisc blocks
    Cache *cache;                        // LRU cache for managing blocks in memory
	E3RLinList *re_data_cands;               // data entries to reinsert
	E3RLinList *deletelist;
	E3RTNode *root_ptr;                    // root-node

	//<---Functions--->
	E3RTree(char *fname, int _b_length, Cache* c, int _dimension);
    E3RTree(char *fname, Cache* c);
    E3RTree(char *inpname, char *fname, int _blength, Cache* c, int _dimension);
    virtual ~E3RTree();

	bool delete_entry(E3REntry *d);
	bool FindLeaf(E3REntry *e);
	int get_num()                        // returns # of stored data
		{ return num_of_data; }
	void insert(E3REntry *d);                // inserts new data into tree
    void load_root();                    // loads root_node into memory
	void rangeQuery(float *mbr, SortedE3RLinList *res, char *rec, int reclen, int *levelcount);
    void read_header(char *buffer);      // reads Rtree header
    void write_header(char *buffer);     // writes Rtree header
};

#endif

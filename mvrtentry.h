#ifndef __MVRENTRY_H
#define __MVRENTRY_H
//////////////////////////////////////////////////////////////////////////////
#include "mvrtnode.h"
#include "mvrtree.h"
#include "rtreeheader.h"
//////////////////////////////////////////////////////////////////////////////
class MVRTree;
class MVRTNode;
//////////////////////////////////////////////////////////////////////////////
class MVREntry : public AbsEntry
{
public:
	int block;
	int son;                            // block # of son
    int dimension;                      // dimension of the box
    int level;                          // which level this entry is in
	int sttime;                         // The starting time of this entry
	int edtime;                         // The ending time of this entry
	  //More words on the starting and ending time of entries
	  //If an entry is still alive, its ending time extend to the current time (i.e.
	  //now), and is represented by NOWTIME.  The starting time is when this entry was
	  //inserted.  Note that the negative edtime means unlimitedly close
	int insertpath[256];
	float *bounces;                     // pointer to box
    static int create_count;
	MVRTree *my_tree;                   // pointer to my R-tree
    MVRTNode *son_ptr;                  // pointer to son if in main mem.

	//<-----functions----->

    MVREntry(int dimension = MVRDIM, MVRTree *rt = NULL);
    virtual ~MVREntry();
    
	bool branchqual(MVREntry *e,bool asign, int level);  //This function checks if the specified entry can be
	  //inserted into this branch (so this function is meaningful for leaf entries only
	bool check_valid();                 // this node checks whether this entry is valid to operate on
	bool check_intersect (float *mbr);  //This function checks if an entry intersects with
	  //an entry with timespan
	void Enlarge(MVREntry *e1, MVREntry *e2);
	void GetBound(int ind, MVRTNode *rn);              //This funciton gets the bounding rectangle of all the etnries except the specified onef
    int get_size();                     // returns amount of needed buffer space
    MVRTNode *get_son();                // returns pointer to the son,
                                        // loads son if necessary
	void fill_in(MVRTNode *n);        // this function fills in the fields a.t. to the given node
	void printinfo();
    void read_from_buffer(char *buffer);// reads data from buffer
	SECTION section(float *mbr);        // tests if mbr intersects the box
    bool section_circle(float *center, float radius);
    void write_to_buffer(char *buffer); // writes data to buffer

    virtual MVREntry & operator = (MVREntry &_d);
	bool operator == (MVREntry &_d);
};

//////////////////////////////////////////////////////////////////////////////
#endif
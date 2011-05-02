#ifndef __MVRTNODE_H
#define __MVRTNODE_H
/////////////////////////////////////////////////////////////////////////

#include "mvrtree.h" 
#include "mvrtentry.h"
#include "st2.h"
#include "ist.h"
/////////////////////////////////////////////////////////////////////////
class SortedMVRLinList;
/////////////////////////////////////////////////////////////////////////
class MVRTNode : public AbsRTNode
{
public:
    bool dirty;                         // TRUE, if node has to be written
	char copysign;                      // This bit is used to indicate whether it should be counted
	  // in building the e3r tree.  Sign=0 means normal status.  1 means version copied.  2 means key
	  // split but with version copy.  Sign=3 means weak underflow.  Currently sing 0 and 3 mean it needs
	  // to be included.
	
	char level;                         // level of the node in the tree (0 means leaf)   
    unsigned char num_entries;                   // # of used entries (max. capacity 256 entries)
	char refcount;                      //This variable denotes how many references are there on this node
	int block;                          // disc block
    int capacity;                       // max. # of entries
    int dimension;

	static bool creating;
	static bool deleting;
    static int create_count;
	static int low_util_count;  //We dont need this any more ***
	static int strong_uf_count;  //We dont need this any more

    MVRTree *my_tree;                   // pointer to R-tree
    MVREntry *entries;                  // array of entries

	//<-----functions----->

	MVRTNode(MVRTree *rt);              //Use this constructor to allocate a new root
	//VERY IMPORTANT: The level field of the nodes are very important attributes.  However
	//this constructor is not supposed to fill in this field.  So WHENEVER you use this
	//constructor to spawn a node, you are responsible for filling in the level field 
	//yourself
    MVRTNode(MVRTree *rt, int _block);
	MVRTNode(int dim, int bsize);
    virtual ~MVRTNode();

	void add_reinsert_alive();               //This function will add all the alive entries
	  //into the reinsert_list and delete them from the current node
	bool CanDel(int ind);
	bool check_all_now();
    bool check_full();                  //This function checks if the node is full
	bool check_dead();			        //This function checks if the current node has any dead entries inside 
	bool check_to_full(int addnum);     //This function checks if the node will become full after the specified number
	  // of entries will be inserted.
	void check_ver();  //This function checks if the node satisfies the version condition.
	INSERTABLE CheckInstability(MVREntry *e, int old_b, bool asign);
	bool check_underflow();
	int choose_subtree2(MVREntry *e,int *cand, int &candsize, bool asign, bool reinserting);
	int count_alive();                  //This function counts how many entries
	  //are alive in the current node;
	bool DelCan(MVREntry *e, int *res, int &resnum, int query=-1);  //This function will add to res array all the entries
	  //that can be deleted without affecting the version condition.  this function can also answer if a queried entry
	  //can be deleted.  the specified e is the one to be inserted.
	R_DELETE delete_entry(MVREntry *e);     //This function deletes the entry e
	   //Note that the sttime field of e does not matter
	   //the edtime field shoulbe be the time e is deleted
	   //e must be currently an alive object
	void enter(MVREntry *de);           // inserts new entry
	bool FindBlock(int bno);
	bool FindLeaf(MVREntry *e);
	int get_header_size();              // This function will return the size of header for a node
	int get_height();                   // This function returns the height of the R-tree
	bool GetIntvMbr(float *mbr, int st, int ed);
    float *get_mbr(int alivesign);      // returns mbr enclosing whole page.  alivesign=0 means don't care
	  //son is negative or not.  alivesign=1 means care.
	int *get_timespan(int alivesign);   // returns the timespan of the whole page. alivesign=0 means don't care
	  //son is negative or not.  alivesign=1 means care.
	bool HandleToFull(int addnum, MVREntry *e1, MVREntry *e2);  //This function tries to reinsert the past data into older trees
	  //thus to avoid splitting on this node.
	bool HandleToFull2(int addnum, MVREntry *e1, MVREntry *e2);
	R_OVERFLOW insert2(int pst, MVREntry *d, MVRTNode **sn1, MVRTNode **sn2, bool force);
	   //This is another version of insert.  force indicates whether the entry will be inserted 
	   //even if split will be caused on the leaf node
	bool is_data_node() 
		{ return (level==0) ? TRUE : FALSE ;};
	void phydelentry(int index);        //this function physically deletes the entry specified
	  //by the inde.
    void rangeQuery(float *mbr, SortedMVRLinList *res, char *rec, char *dupblock);
	  //This function performs the range query.  Note that mbr is of size 6, as with
	  //the rangequery function in MVRTree.  res is used to hold the results
	  //rec used to avoid reporting the same oid twice
	bool Reinsert(MVREntry *e, int oblock,bool asign);
    void read_from_buffer(char *buffer);// reads data from buffer
	void SortCen(int *res, int &resnum);  //This function will sort the entries by their centers
	void SortDelCan(int *res, int resnum);  //This function sorts the result of the delete
	  //candidates according to the preferred priority of reinsertion
	void SortDelCan2(int *res, int resnum); //This function will sort the result by the area
	  //decrement 
	void SortDelCan3(int *res, int resnum); //This function will sort the result by the distance
	  //from the center
	void split(MVRTNode *sn);           // splits directory page to *this and sn
    int split(float **mbr, int **distribution);
                                        // splits an Array of mbr's into 2
                                        // and returns in distribution
                                        // which *m mbr's are to be
                                        // moved
    R_OVERFLOW TreatOverflow(int nver, MVRTNode **sn1, MVRTNode **sn2, int addnum);
	  //This function handles the overflow arising in the entry insertion
	  //The nver is the sttime of the parent node.  sn1 and sn2 are the two
	  //new nodes likely created.
	  //addnum is the number of new entries added
	  //if nver is negative, no strong version overflow is checked
	int VersionCopy(MVRTNode *sn2, int nver);
	  //This function is called in the version split to copy all the
	  //present versions from this to sn2 
	  //nver is the most recent version
	  //The return value reports the # of entries copied
	  //addnum is the number of new entries added
    void write_to_buffer(char *buffer); // writes data to buffer
};
/////////////////////////////////////////////////////////////////////////
#endif
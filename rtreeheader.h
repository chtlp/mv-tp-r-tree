//////////////////////////////////////////////////////////////////////////////
//This file contains ancestor of all R-trees.  It provides a way for inheritance
//It is coded by TAO Yufei, 2000
//////////////////////////////////////////////////////////////////////////////
#ifndef __RTREEHEADER
#define __RTREEHEADER
//////////////////////////////////////////////////////////////////////////////
#include "blk_file.h"
//////////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////////
#define MAXREAL 9.99e20
#define MINREAL -9.99e20
#define MININT -99999999
#define MAXINT 99999999
#define FLOATZERO 1e-20
#define MAX_DIMENSION 256

#ifndef HRTREEDIMENSION
#define HRTREEDIMENSION 2
#endif  

#ifndef E3RTREEDIMENSION
#define E3RTREEDIMENSION 3
#endif

#ifndef MVRDIM
#define MVRDIM 2
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif  

#define min(a, b) (((a) < (b))? (a) : (b)  )
#define max(a, b) (((a) > (b))? (a) : (b)  )

//////////////////////////////////////////////////////////////////////////////
//These are the functions needed in various operations
//////////////////////////////////////////////////////////////////////////////
float area(int dimension, float *mbr);
float margin(int dimension, float *mbr);
float overlap(int dimension, float *r1, float *r2);
float* overlapRect(int dimension, float *r1, float *r2);
float objectDIST(float *p1, float *p2, int dim);
float MINMAXDIST(float *Point, float *bounces, int dim);
float MINDIST(float *Point, float *bounces, int dim);
bool inside(float &p, float &lb, float &ub);
void enlarge(int dimension, float **mbr, float *r1, float *r2);
bool is_inside(int dimension, float *p, float *mbr);
bool section(int dimension, float *mbr1, float *mbr2);
bool section_c(int dimension, float *mbr1, float *center, float radius, int dim);
int sort_lower_mbr(const void *d1, const void *d2);
int sort_upper_mbr(const void *d1, const void *d2);
int sort_center_mbr(const void *d1, const void *d2);
int sortmindist(const void *element1, const void *element2);
void error(char *t, bool ex);
int roundup(float num);
//////////////////////////////////////////////////////////////////////////////
// externals
//////////////////////////////////////////////////////////////////////////////
//extern float page_access;

//////////////////////////////////////////////////////////////////////////////
//enums / types
//Since it has been defined here, there is no need to define them
//again any elsewhere.
//////////////////////////////////////////////////////////////////////////////
enum SECTION {OVERLAP, INSIDE, S_NONE};
enum R_OVERFLOW {VSPLITSVOF, VSPLIT, KSPLIT, KSPLITNN,NONE,SPLIT,
   REINSERT,STRONGUNDER, TOSPLIT};
  //VSPLITSVOF stands for version split and strong version overflow
  //VSPLIT stands for version split but now strong version overflow
  //KSPLIT stands for key split
  //KSPLITNN stands for key split but new node created
  //NONE stands for no overflow occurred
  //STRONGUNDER stands for strong underflow
enum R_DELETE {NOTFOUND, NORMAL,WUNDERFLOW,NERASED};
typedef float *floatptr;
struct SortMbr
{
    int dimension;
    float *mbr;
    float *center;
    int index;
};

struct DataStruct
{
    int dimension;
    float *Data;
};

//////////////////////////////////////////////////////////////////////////////
//Note that to make the codes look more neatly, the functions are ordered
//in the way that constructor and destructor are put first, the operators last,
//and all other functions are sorted alphabetically.
//Also note that the fields defined in these classes are to make common classes
//like BlockFile, Cache, Linlist, etc work
//////////////////////////////////////////////////////////////////////////////
class AbsEntry
{
public: 
  AbsEntry(){};
};

class AbsRTNode
{
public:
  AbsRTNode(){};
};

class AbsRTree
{
public:
  BlockFile *file;


  AbsRTree(){};
};

#endif

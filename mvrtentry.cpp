#include "mvrtentry.h"

//////////////////////////////////////////////////////////////////////////////
// MVREntry
//////////////////////////////////////////////////////////////////////////////

MVREntry::MVREntry(int _dimension, MVRTree *rt)
{
    dimension = _dimension;
    my_tree = rt;
    bounces = new float[2*dimension];
    son_ptr = NULL;
    son = MININT;
	level=-1;
	//Now we are to initialize the starting time and ending time
	//Uses are supposed to fill in the value for starting time.
	//The ending time is set to MAXINT, meaning that it extends to 'now'
	sttime=-1;
	edtime=NOWTIME;
	MVREntry::create_count++;
}

//////////////////////////////////////////////////////////////////////////////

MVREntry::~MVREntry()
{
	MVREntry::create_count--;
	delete [] bounces;
    if (son_ptr != NULL)
		my_tree->DelMVRNode(&son_ptr);
	    //delete son_ptr;
}

//////////////////////////////////////////////////////////////////////////////
bool MVREntry::branchqual(MVREntry *e, bool asign, int level)
  //asign indicates whether it is allowed to insert a dead entry into an alive entry
{
	if (level==e->level+1 && !asign && e->edtime!=NOWTIME)
		if (edtime==NOWTIME) return false;
	if (sttime>e->sttime)
		return false;
	if (edtime<e->edtime)
		return false;
	return true;
}
//////////////////////////////////////////////////////////////////////////////

bool MVREntry::check_intersect (float *mbr)
{
	SECTION s;
	//We first check if the spatial intersects
	s =section(mbr);
	//This is a clever way as the section function makes use of 2 dimension only
	if (s == INSIDE || s == OVERLAP)
		//Ok, the spatial overlaps
	{
		int lt = mbr[4];
		int ht = mbr[5];
		int minht,maxlt;
		//minht is the minimum higher bound of entry and mbr
		//maxlt is the maximum lower bound of entry and mbr
		minht = min(edtime-1,ht);
		maxlt = max(sttime,lt);
		if (maxlt <= minht)
			return true;
		else 
			return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////

bool MVREntry::check_valid()
{
	if (son==MININT || son<0)
	{	
		printinfo();
		error("check valid failed--son has not been filled\n",true);
	}
	if (my_tree==NULL)
		error("check vaild failed--my_tree is not filled\n",true);
	if (level==-1)
	{
		printinfo();
		error("check valid failed--level is not filled\n",true);
	}
	
	for (int i=0;i<MVRDIM;i++)
	{
		if (bounces[2*i]>bounces[2*i+1])
		{
			printinfo();
			error("mvrentry::checkvalid--bounce not properly set\n", true);
			return false;
		}
	}
	if (sttime >= edtime)
	//*****************************Subject to change*************************
	//Change this line to sttime >= edtime if we adopt the "open" range
	{
		printinfo();
		error("mvrentry::checkvalid--Entry's time not properly set\n",true);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
void MVREntry::Enlarge(MVREntry *e1, MVREntry *e2)
  //Note that the resulting entry should be treated as an auxiliary one
  //because we only set the bounces and not the other attributes
{
	for (int i=0;i<dimension;i++)
	{
		bounces[2*i]=min(e1->bounces[2*i],e2->bounces[2*i]);
		bounces[2*i+1]=max(e1->bounces[2*i+1],e2->bounces[2*i+1]);
	}
	sttime=min(e1->sttime,e2->sttime);
	edtime=max(e1->edtime,e2->edtime);
}
//////////////////////////////////////////////////////////////////////////////
int MVREntry::get_size()
{
    return 2*dimension * sizeof(float) + sizeof(int) + sizeof(int) + sizeof(int);
	// 2*dimension floats + son int + sttime int + edtime int
}

//////////////////////////////////////////////////////////////////////////////

void MVREntry::fill_in(MVRTNode *no)
{
	  //We fill in the necessary fields of newe
	  float *mbr;
	  mbr=no->get_mbr(0);	
	  memcpy(bounces,mbr,sizeof(float)*2*MVRDIM);
	  delete [] mbr;
	  //fill in the timespan
	  int *time=no->get_timespan(0);
	  if (sttime<time[0])
		  //sttime > time[0] can happen only after a deletion
	      sttime=time[0];
	  if (edtime>time[1])
		  edtime=time[1];
	  delete [] time;
	  //fill in the son
	  son=no->block;
	  //fill in the level
	  level=no->level;
	  //Note that we also fill in the pointer field.  So deleting
	  //this entry also deletes the node.  In case such fill in is
	  //unnecessary, you should handle this specially.
	  son_ptr=no;
}

//////////////////////////////////////////////////////////////////////////////

MVRTNode* MVREntry::get_son()
{
    if (son_ptr == NULL)
		son_ptr=my_tree->NewMVRNode(abs(son));
    return son_ptr;
}
//////////////////////////////////////////////////////////////////////////////
void MVREntry::GetBound(int ind, MVRTNode *rn)
{
	for (int i=0;i<dimension;i++)
	{bounces[2*i]=MAXREAL; bounces[2*i+1]=MINREAL;
	 sttime=MAXINT; edtime=MININT;}

	for (int i=0;i<rn->num_entries;i++)
	{
		if (i!=ind)
		{
			for (int j=0;j<dimension;j++)
			{
				bounces[2*j]=min(bounces[2*j],rn->entries[i].bounces[2*j]);
				bounces[2*j+1]=max(bounces[2*j+1],rn->entries[i].bounces[2*j+1]);
				sttime=min(sttime,rn->entries[i].sttime); edtime=max(edtime,rn->entries[i].edtime);
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////
void MVREntry::printinfo()
{
	printf("\n\tEntry son: %d  at level %d\n", son, level);
	for (int i=0;i<dimension;i++)
		printf("\tDimension %d: [%f, %f]\n",i+1,bounces[2*i],bounces[2*i+1]);
	printf("\tSttime and Edtime: [%d,%d)\n",sttime,edtime);
}
//////////////////////////////////////////////////////////////////////////////
void MVREntry::read_from_buffer(char *buffer)
{
    int i;

    i = 2*dimension*sizeof(float);
    memcpy(bounces, buffer, i);
    memcpy(&son, &buffer[i], sizeof(int));
    i += sizeof(int);
	memcpy(&sttime,&buffer[i],sizeof(int));
	i += sizeof(int);
	memcpy(&edtime,&buffer[i],sizeof(int));
	i+=sizeof(int);
}

//////////////////////////////////////////////////////////////////////////////

SECTION MVREntry::section(float *mbr)
{
    bool inside;
    bool overlap;
    int i;

    overlap = TRUE;
    inside  = TRUE;
    for (i = 0; i < dimension; i++)
    {
	if (mbr[2*i]     > bounces[2*i + 1] ||
	    mbr[2*i + 1] < bounces[2*i])
	    overlap = FALSE;
	if (mbr[2*i]     < bounces[2*i] ||
	    mbr[2*i + 1] > bounces[2*i + 1])
	    inside = FALSE;
    }
    if (inside)
		return INSIDE;
    else if (overlap)
		return OVERLAP;		
    else
		return S_NONE;
}

//////////////////////////////////////////////////////////////////////////////

bool MVREntry::section_circle(float *center, float radius)
{
 float r2;

 r2 = radius * radius;

 if ((r2 - MINDIST(center,bounces, MVRDIM)) < 1.0e-8)
	return TRUE;
 else
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

void MVREntry::write_to_buffer(char *buffer)
{
    int i;

    i = 2*dimension*sizeof(float);
    memcpy(buffer, bounces, i);
    memcpy(&buffer[i], &son, sizeof(int));
    i += sizeof(int);
	memcpy(&buffer[i],&sttime,sizeof(int));
	i+=sizeof(int);
	memcpy(&buffer[i],&edtime,sizeof(int));
	i+=sizeof(int);
}

//////////////////////////////////////////////////////////////////////////////

bool MVREntry::operator == (MVREntry &_d)
//Note that this function compares two entries based on their spatial
//extents only
{
	if (son!=_d.son) return false;
	if (dimension!=_d.dimension) return false;
	for (int i=0;i<2*dimension;i++)
		if ((bounces[i]-_d.bounces[i])>FLOATZERO) return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////

MVREntry& MVREntry::operator = (MVREntry &_d)
{
	block = _d.block;
    dimension = _d.dimension;
    son_ptr = _d.son_ptr;
    my_tree = _d.my_tree;
	level=_d.level;
	 
    son = _d.son;
	memcpy(bounces, _d.bounces, sizeof(float) * 2 * dimension);
	sttime=_d.sttime;
	edtime=_d.edtime;
	
	for (int i=0;i<5;i++)
		insertpath[i]=_d.insertpath[i];
    return *this;
}
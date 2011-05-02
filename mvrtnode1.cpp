#include "mvrtnode.h"
//////////////////////////////////////////////////////////////////////////////
//These lines have been inspected by TAO Yufei, 2 November.
//////////////////////////////////////////////////////////////////////////////
// MVRTNode
//////////////////////////////////////////////////////////////////////////////
MVRTNode::MVRTNode(MVRTree *rt)
	//Use this constructor to gene
{
	char *b;
	int header_size;
	int i;
	MVREntry * d;
	//All the node creation work will be done by my_tree->NewMVRNode
	//Not allowed to call constructors directly
	if (MVRTNode::creating==false)
		error("Trying to call block constructor illegally\n",false);

	copysign    = 0;
	dimension   = rt->dimension;
	dirty       = TRUE;
	my_tree     = rt;
	num_entries = 0;
	refcount    = 0;

	d           = new MVREntry();
	header_size = get_header_size();  // level + num_entries
	capacity    = (rt->file->get_blocklength() - header_size) / d->get_size();
	delete d;

	entries = new MVREntry[capacity];
	for (i=0; i<capacity;i++)
	{
		entries[i].dimension = my_tree->dimension; 
		entries[i].my_tree   = my_tree;
	}

	b     = new char[rt->file->get_blocklength()];
	block = rt->file->append_block(b);
	delete [] b;

	my_tree->num_of_data++;

	MVRTNode::create_count++;
}

//////////////////////////////////////////////////////////////////////////////

MVRTNode::MVRTNode(MVRTree *rt, int _block)
{

	char *b;
	int header_size;
	int i;
	MVREntry * d;

	if (MVRTNode::creating==false)
		error("Trying to call block constructor illegally\n",true);

	dimension   = rt->dimension;
	dirty       = FALSE;
	my_tree     = rt;
	num_entries = 0;

	d           = new MVREntry();
	header_size = get_header_size();
	capacity    = (rt->file->get_blocklength() - header_size) / d->get_size();
	delete d;

	entries = new MVREntry[capacity];
	for (i=0; i<capacity;i++)
	{
		entries[i].dimension = my_tree->dimension;
		entries[i].my_tree   = my_tree;
	}

	block = _block;
	b = new char[rt->file->get_blocklength()];
	if (rt->cache == NULL) // no cache
		rt->file->read_block(b, block);
	else
		rt->cache->read_block(b, block, rt);

	read_from_buffer(b);
	for (i=0;i<num_entries;i++)
		entries[i].level=level;
	delete [] b;

	MVRTNode::create_count++;
}

//////////////////////////////////////////////////////////////////////////////

MVRTNode::MVRTNode(int dim, int bsize)
{
	char *b;
	int header_size;
	MVREntry * d;
	int i;

	copysign    = 0;
	dirty       = false;
	dimension   = dim;
	my_tree     = NULL;
	num_entries = 0;
	refcount    = 0;

	d           = new MVREntry();
	header_size = get_header_size();
	capacity    = (bsize - header_size) / d->get_size();
	delete d;

	entries = new MVREntry[capacity];
	for (i=0; i<capacity;i++)
	{
		entries[i].dimension = dim;
		entries[i].my_tree = my_tree;
	}

	MVRTNode::create_count++;
}

//////////////////////////////////////////////////////////////////////////////

MVRTNode::~MVRTNode()
{
	char *b;

	if (MVRTNode::deleting==false)
		error("Trying to call block destructor illegally\n",false);

	if (dirty)
	{
		b = new char[my_tree->file->get_blocklength()];
		write_to_buffer(b);
		if (my_tree->cache == NULL) // no cache
			my_tree->file->write_block(b, block);
		else
			my_tree->cache->write_block(b, block, my_tree);
		delete [] b;
	}
	delete [] entries;
	create_count--;
}

//////////////////////////////////////////////////////////////////////////////

void MVRTNode::add_reinsert_alive()
{
	for (int i=0;i<num_entries;i++)
	{
		if (entries[i].edtime==NOWTIME)
		{
			if (entries[i].son_ptr)
			{
				my_tree->DelMVRNode(&entries[i].son_ptr);
				entries[i].son_ptr=NULL;
			}
			MVREntry *e       = new MVREntry(MVRDIM,my_tree);
			(*e)              = entries[i];
			e->sttime         = my_tree->most_recent_time;
			e->edtime         = NOWTIME;
			entries[i].edtime = my_tree->most_recent_time;
			e->son_ptr        = NULL;
			e->level          = level;

			if (level>0 || !USEPOOLING)
				my_tree->reinsert_list->insert(e);
			else
			{
				my_tree->addtoetable(e);
				delete e;
			}
			if (entries[i].sttime == my_tree->most_recent_time)
			{
				phydelentry(i); i--;
			}
		}
	}
	dirty = true;
	if (num_entries > 0 && level == 0)
		my_tree->leafvc++;
	else if (num_entries > 0 && level > 0)
		my_tree->nonleafvc++;
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::CanDel(int ind)
{
	int *cand = new int[num_entries];
	int candsize;
	bool q = DelCan(NULL,cand,candsize,ind);
	delete [] cand;
	return q;
}
//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::check_all_now()
{
	for (int i = 0;i < num_entries;i++)
		if (entries[i].sttime != my_tree->most_recent_time)
			return false;
		else if (entries[i].edtime != NOWTIME)
			error("Integrity error detected by check_all_now (mvrtnode)!\n",true);
	return true;
}
//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::check_full()                   
	//This function checks if the node is full
{
	if (num_entries<capacity - 1) 
		return false; 
	else 
		return true;
	//if num_entries==capcity - 1, it is time to split
}
//////////////////////////////////////////////////////////////////////////////

bool MVRTNode::check_dead()
{
	int i;
	for (i=0;i<num_entries;i++)
	{
		if (entries[i].edtime !=NOWTIME)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::check_to_full(int addnum)
{
	int newnum=num_entries+addnum;
	if (newnum<capacity - 1)
		return false;
	else
		return true;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::check_ver()
{
	int minvernum;
	if (level!=get_height())
		minvernum = roundup(capacity*PHYUF);
	else
		if (level==0)  //if this is the root and is a leaf then return
			return;
		else
			minvernum = 2;
	IndexSt *bound;
	int num = 2*num_entries;
	bound   =new IndexSt(num);
	for (int i = 0;i < num_entries;i++)
	{
		float st = entries[i].sttime, ed = entries[i].edtime;
		if (bound->Check(st)==-1)
			bound->Add(st,0,0,0);
		if (bound->Check(ed)==-1)
			bound->Add(ed,0,0,0);
	}
	for (int i = 0;i < num_entries;i++)
	{
		int stnum = bound->Check(entries[i].sttime);
		if (stnum==-1) error("in delcan no reason for stnum to be -1!\n",true);
		int ednum=bound->Check(entries[i].edtime);
		if (ednum==-1) error("in delcan no reason for ednum to be -1!\n",true);
		for (int j=stnum;j<ednum;j++)
			bound->items[j].v2 += 1;
	}

	for (int i=0;i<bound->effnum;i++)
		if (bound->items[i].v2<minvernum && bound->items[i].v2!=0)
		{
			printf("Node %d--", block);
			error("Weak version does not hold in the node\n",true);
		}

		delete bound;
}
//////////////////////////////////////////////////////////////////////////////
INSERTABLE MVRTNode::CheckInstability(MVREntry *e, int old_b, bool asign)
{
	e->insertpath[level]=-1;
	if (level==0)
	{
		if (check_to_full(1))
			return FULL;
		if (old_b == block)  //This is the same node where the entry currently is
			return NO;
		e->insertpath[level] = block;
		return YES;
	}
	else
	{
		if (level==e->level)
		{
			if (check_to_full(1))
				return FULL;
			if (old_b == block)
				return NO;
			e->insertpath[level] = block;
			return YES;
		}

		int *cand = new int[capacity];
		int candsize;
		choose_subtree2(e,cand,candsize,asign,true);
		//setting the reinserting tag to be true to indicate that we should
		//choose the subtree subject to the "goodvalue" constraint.
		if (candsize == 0)
		{
			delete [] cand;
			return NOBRANCH;
		}
		for (int i = 0;i < candsize;i++)
		{
			int follow      = cand[i];
			MVRTNode *succ  = entries[follow].get_son();
			INSERTABLE cres = succ->CheckInstability(e, old_b, asign);
			if (cres == YES)
			{
				delete [] cand;
				e->insertpath[level] = block;
				return YES;
			}
		}
		delete [] cand;
		return NO;
	}
}
//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::check_underflow()
{
	int count=count_alive();
	if (count<(capacity*PHYUF) && count!=0 && level!=get_height())
		return true;
	else 
		return false;
}
//////////////////////////////////////////////////////////////////////////////

int MVRTNode::choose_subtree2(MVREntry *e,int *cand, int &candsize, bool asign, bool reinserting)
{
	int nowv     = my_tree->most_recent_time;
	int futureed = nowv + FUTURE;  //This is useful when COMPUTER3D is true, indicating how far we should
	//consider into the future.
	int i, j;
	int follow   = -1;             //This is the number of the subtree
	IndexSt *ist = new IndexSt(capacity);     // This is used to sort the area enlargement of the entries
	float a, old_a, f, o, old_o;	       //a: area, f: area enlargement, o:overlap

	if (level == 1)   		       // son_is_data and the criterion will be based on the minimum overlap
	{
		for (i = 0; i < num_entries; i++)
		{
			if (entries[i].branchqual(e,asign,level))
			{
				if (COMPUTE3D)
					old_a     = Area3D(&(entries[i]), futureed);
				else
					old_a     = area(dimension,entries[i].bounces);
				MVREntry *tmp = new MVREntry(dimension,NULL);
				tmp->Enlarge(&entries[i],e);
				if (COMPUTE3D)
					a = Area3D(tmp,futureed);
				else
					a = area(dimension,tmp->bounces);
				f     = a - old_a;
				if (f < 0) error("hey, no reason for f to be less than 0\n",true);
				// calculate overlap before enlarging entry_i
				old_o = o = 0.0;
				for (j = 0; j < num_entries; j++)
				{
					if (j != i)
					{
						if (COMPUTE3D)
						{
							old_o += Overlap3D(&(entries[i]),&(entries[j]),futureed);
							o     += Overlap3D(tmp,&(entries[j]),futureed);
						}
						else
						{
							old_o += overlap(dimension, entries[i].bounces, entries[j].bounces);
							o     += overlap(dimension,entries[j].bounces,tmp->bounces);
						}
					}
				}
				delete tmp;
				o -= old_o;
				if (o < 0)  error("hey, no reason for o to be less than 0\n",true);

				if (o/old_o <= GOODRATIO || !reinserting)
					ist->Add(o,f,a,i);
			}
		}
		ist->Abandon(DETRATIO);  //We only accept those branches which are within a certain
		//range from the optimal one
	}
	else // son is not a data node and the criterion will be based on the minimum area enlargement
	{
		for (i = 0; i < num_entries; i++)
		{
			if (entries[i].branchqual(e,asign,level))
			{
				if (COMPUTE3D)
					old_a       = Area3D(&(entries[i]), futureed);
				else
					old_a       = area(dimension,entries[i].bounces);
				MVREntry *tmp = new MVREntry(dimension,NULL);
				tmp->Enlarge(&entries[i],e);
				if (COMPUTE3D)
					a = Area3D(tmp,futureed);
				else
					a = area(dimension,tmp->bounces);
				f   = a - old_a;
				if (f<0) error("hey, no reason for f to be less than 0\n",true);
				// calculate overlap before enlarging entry_i
				delete tmp;

				if (f/old_a <= GOODRATIO || !reinserting)
					ist->Add(f,a,0,i);
			}
		}
		ist->Abandon(DETRATIO);
	}
	int inum=ist->GetNum();
	candsize=0;
	if (inum==0) follow=-1;  //No qualified entry selected
	else
	{
		for (int i=0;i<inum;i++)
		{
			float tmp[3];
			int tmpind;
			ist->GetItem(tmp,tmpind,i);
			cand[candsize]=tmpind;
			candsize++;
		}
		follow=cand[0];
	}
	delete ist;
	if (!TRYMORESUBTREE && candsize>1)
		candsize=1;
	return follow;
}
//////////////////////////////////////////////////////////////////////////////
int MVRTNode::count_alive()
{
	int count=0;
	for (int i=0;i<num_entries;i++)
	{
		if (entries[i].edtime==NOWTIME)
			count++;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::DelCan(MVREntry *e, int *res, int &resnum, int query)
{
	resnum=0;
	int minvernum;
	if (level != get_height()) 
		minvernum = roundup(capacity * PHYUF);  
	else
		minvernum = 2;
	//eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeexplanation
	//The minimum number of entries that a node should have is upper(capacity*PHYUF)
	//So the condition for checking underflow should be < capacity * PHYUF
	//eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeexplanation
	IndexSt *bound;
	int num;
	if (e)
		num = 2*num_entries+2;
	else
		num = 2*num_entries;
	bound   = new IndexSt(num);

	for (int i = 0; i < num_entries; i++)
	{
		float st = entries[i].sttime, ed = entries[i].edtime;
		if (bound->Check(st) == -1)
			bound->Add(st,0,0,0);
		if (bound->Check(ed) == -1)
			bound->Add(ed,0,0,0);
	}
	if (e)
	{
		float st=e->sttime, ed=e->edtime;
		if (bound->Check(st) ==-1)
			bound->Add(st,0,0,0);
		if (bound->Check(ed) == -1)
			bound->Add(ed,0,0,0);
	}

	for (int i = 0; i < num_entries; i++)
	{
		int stnum = bound->Check(entries[i].sttime);
		if (stnum == -1) error("in delcan no reason for stnum to be -1!\n",true);
		int ednum = bound->Check(entries[i].edtime);
		if (ednum == -1) error("in delcan no reason for ednum to be -1!\n",true);
		for (int j = stnum; j < ednum; j++)
			bound->items[j].v2 += 1;
	}
	if (e)
	{
		int stnum = bound->Check(e->sttime);
		int ednum = bound->Check(e->edtime);
		for (int j = stnum; j < ednum; j++)
			bound->items[j].v2 += 1;
	}
	bool q = false;
	for (int i = 0; i < num_entries; i++)
	{
		int stnum = bound->Check(entries[i].sttime);
		int ednum = bound->Check(entries[i].edtime);
		int yes=true;
		for (int j = stnum; j < ednum; j++)
			if (bound -> items[j].v2 == minvernum)
				yes=false;
			else if (bound -> items[j].v2 < minvernum)
				error("Weak underflow checked in this node by DelCan!\n",true);
		if (yes)
		{ 
			res[resnum] = i; resnum++;
			if (i == query) q=true;
		}
	}
	SortDelCan(res,resnum);
	delete bound;
	return q;
}
//////////////////////////////////////////////////////////////////////////////
R_DELETE MVRTNode::delete_entry(MVREntry *e)
	//In this function we will use ERASED to indicate it's necessary
	//to reinsert the entries
{
	if (level == 0)	//This is a leaf node
	{
		for (int i = 0; i < num_entries; i++)
		{
			//Now we check if the entry in the leaf is the object requested
			//First it has to be an alive entry (its not allowed to delete an old entry
			if (entries[i].edtime == NOWTIME && entries[i] == *e)
			{
				if (entries[i].sttime==my_tree->most_recent_time)  //We need to do a physical deletion here
				{
					phydelentry(i);
					int count=count_alive();
					if (check_underflow())
					{
						add_reinsert_alive();
						if (num_entries>0)
							return WUNDERFLOW;
						else
							return NERASED;  //Since it is possible that all entries in this code are reinserted
						//we need to handle the physical deletion here
					}
					return NORMAL;
				}
				else
				{
					//Now we fill in the ending time
					entries[i].edtime = my_tree->most_recent_time;
					if (INSERTAFTERDEL && CanDel(i) && my_tree->CheckInstability(&entries[i],block,false))
					{
						if (!INSERTAFTERDEL)
							error("The current setting says no insertion after deletion",true);
						MVREntry *e = new MVREntry(dimension,NULL);
						(*e) = entries[i]; e->level = 0; e->block = block;
						for (int j = 0; j <= get_height(); j++)
							e->insertpath[j] = entries[i].insertpath[j];
						phydelentry(i);
						my_tree->Reinsert(e,block,false);

					}
					dirty = true;

					if (check_underflow())
					{
						add_reinsert_alive();
						return WUNDERFLOW;  //Note that here we dont need to handle physical deletion here (think why?...)
					}
					return NORMAL;
				}
			}
		}
		//We have searched all entries and e is not found
		return NOTFOUND;
	}
	else
		//This is a non-leaf node
	{
		for (int i=0;i<num_entries;i++)
		{
			//For each alive branch we check if it can contain e
			float *mbr=new float[6];
			//*****Code changed by TAO Yufei********
			//memcpy(mbr,entries[i].bounces,2*dimension*sizeof(float));
			//mbr[4]=entries[i].sttime;
			//mbr[5]=entries[i].edtime;
			memcpy(mbr,e->bounces,2*dimension*sizeof(float));
			//if (entries[i].edtime==NOWTIME && entries[i].check_intersect(mbr))
			SECTION ints=entries[i].section(mbr);
			delete [] mbr;
			if (entries[i].edtime==NOWTIME && ints!=S_NONE)
				//**************************************
			{
				MVRTNode *succ;
				succ=entries[i].get_son();
				R_DELETE ret=succ->delete_entry(e);
				//if (ret!=NERASED || succ->num_entries>0)
				//	CheckMVRNode(succ);
				switch (ret)
				{
				case NOTFOUND:
					//continue searching in the next branch
					break;
				case NORMAL:
				case WUNDERFLOW:
					{
						if (ret==WUNDERFLOW)
							my_tree->wunder++;
						entries[i].fill_in(succ);
						if (entries[i].edtime==NOWTIME && ret==WUNDERFLOW)
							error("hey, the edtime should not be nowtime here after weak version underflow\n",true);
						if (entries[i].sttime == entries[i].edtime)
							phydelentry(i);
						dirty=true;

						if (check_underflow())
						{
							add_reinsert_alive();
							if (num_entries>0)
								return WUNDERFLOW;
							else
								return NERASED;
						}
						return NORMAL;
						break;
					}
				case NERASED:
					MVRTNode::strong_uf_count++;
					my_tree->nerase++;
					if (level==1) 
						my_tree->num_of_dnodes--;
					else
						my_tree->num_of_inodes--;
					succ->refcount--;
					phydelentry(i);

					if (check_underflow())
					{
						add_reinsert_alive();
						if (num_entries>0)
							return WUNDERFLOW;
						else
							return NERASED;
					}
					return NORMAL;
					break;
				}//end of switch

			}//end of if
		}//end of for
		//We have searched all branches and none contains e
		return NOTFOUND;
	}
}

//////////////////////////////////////////////////////////////////////////////

void MVRTNode::enter(MVREntry *de)
{
	if (num_entries > (capacity-1))
		error("MVRTNode::enter: called, but node is full.\n", TRUE);
	entries[num_entries] = *de;
	num_entries++;
	de->son_ptr = NULL;
	delete de;
	dirty = true;
}

//////////////////////////////////////////////////////////////////////////////

bool MVRTNode::FindLeaf(MVREntry *e)
{
	MVRTNode *succ;
	if (level>0)
	{
		for (int i=0;i<num_entries;i++)
		{
			float *f;
			f=new float[6];
			memcpy(f,e->bounces,2*MVRDIM*sizeof(float));
			f[4]=e->sttime;f[5]=e->edtime;
			bool intersect=entries[i].check_intersect(f);
			delete [] f;
			if (intersect)
			{
				succ=entries[i].get_son();
				bool find;
				find=succ->FindLeaf(e);
				if (find)
				{
					/*
					printf("Son %d %d %d %d\n", block, entries[i].sttime,entries[i].edtime,i);
					int *t;
					t=get_timespan(0);
					printf("This %d %d %d\n",block,t[0],t[1]);
					delete [] t;*/
					//printf("block:%d, ",block);
					return true;
				}
			}
		}
		return false;
	}
	else
	{
		for (int i=0;i<num_entries;i++)
		{
			//if (entries[i]==(*e) && e->sttime==entries[i].sttime && e->edtime==entries[i].edtime)
			//if (entries[i]==(*e) && e->sttime==entries[i].sttime)
			if (entries[i]==(*e) && entries[i].edtime==NOWTIME)
				//&& entries[i].sttime<=e->sttime && e->sttime<=entries[i].edtime)
			{
				/*
				int *t;
				t=get_timespan(0);
				printf("This %d %d %d\n",block,t[0],t[1]);
				delete [] t;*/
				//printf("found in block %d, ",block);
				return true;
			}
		}
		return false;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////

bool MVRTNode::FindBlock(int bno)
{
	MVRTNode *succ;
	if (bno==block)
		return true;
	if (level>0)
	{
		for (int i=0;i<num_entries;i++)
		{
			succ=entries[i].get_son();
			bool find;
			find=succ->FindBlock(bno);
			if (find)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return false;
	}
	return false;
}
#include "mvrtree.h"

//////////////////////////////////////////////////////////////////////////////
// implementation
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// MVRTree
//////////////////////////////////////////////////////////////////////////////
MVRTree::MVRTree(char *fname, char *rtfile, int _b_length, Cache *c, int _dimension)
{
	blockmem=new BlockMem(MAXBLOCK);
	//Set the file and cache
    file = new BlockFile(fname, _b_length);
    cache = c;
	//Now we set the root table file name
    rtafname=new char[strlen(rtfile)+1];
	strcpy(rtafname,rtfile);
	//Set the reinsert link list
    reinsert_list = new MVRLinList();
	delete_list=new MVRLinList();
	//Set the header buffer
    header = new char [file->get_blocklength()];
    dimension = _dimension;
    root = 0;
    root_ptr = NULL;
    num_of_data = num_of_inodes = num_of_dnodes = 0;

	//Here we will create the first tree for version 0
	//root_ptr = new MVRTNode(this);
	root_ptr=NewMVRNode(-1);
    increase_node_count(0);
    root_ptr->level = 0;
    root = root_ptr->block;
	//Initialize the roottable
	rtable.rtnum=1;
	rtable.rno[0]=0;
	rtable.bno[0]=root;

	most_recent_time=-1;
	purekey = purever = verkey = keyver = nerase 
		= wunder = verunder = asplit = leafvc = nonleafvc = 0;
	entrytable=new TmpEntry[OBJECTNUM];
	etablecount=0;
	ResetEntrytable();
	flushing=false;

	char defname[500];
	strcpy(defname,fname);
	strcat(defname,".def");
	def=fopen(defname,"w");
}

//////////////////////////////////////////////////////////////////////////////

MVRTree::MVRTree(char *fname, char *rtfile, Cache *c)
{
	blockmem=new BlockMem(MAXBLOCK);
    //Initialize the file and cache
    file  = new BlockFile(fname, 0); // changed by me
    cache = c;
	//Initialize the reinsert link list
    reinsert_list = new MVRLinList();
	delete_list=new MVRLinList();
    //Initialize the header
    header = new char [file->get_blocklength()];
    file->read_header(header);
    read_header(header);
    root_ptr = NULL;

	//Now we set the root table file name
    rtafname=new char[strlen(rtfile)+1];
	strcpy(rtafname,rtfile);
	//Now we are to initialize the roottable
	read_root_table();

	wasted=0;
	most_recent_time=-1;

	purekey = purever = verkey = keyver = nerase 
		= wunder = verunder = asplit = leafvc = nonleafvc = 0;

	entrytable=new TmpEntry[OBJECTNUM];
	etablecount=0;
	ResetEntrytable();
	flushing=false;
	def=NULL;	
}

//////////////////////////////////////////////////////////////////////////////

MVRTree::~MVRTree()
{
	
    write_header(header);
    file->set_header(header);
    delete [] header;

    if (root_ptr != NULL)
    {
        //delete root_ptr;
		DelMVRNode(&root_ptr);
        root_ptr = NULL;
    }

	if (cache)
      delete cache;

    delete file;
    delete reinsert_list;
	delete delete_list;

	write_root_table();
	//delete the rtafname
	delete [] rtafname;
	delete [] entrytable;
	delete blockmem;
	

	
	if (def)
	{
		fprintf(def,"This R-Tree contains %d internal, %d data nodes and %d data\n",
		   num_of_inodes, num_of_dnodes, num_of_data);
		fprintf(def,"*****************************************\n");
		fprintf(def,"AVOIDLEAFSPLIT: %d\n",AVOIDLEAFSPLIT);
		fprintf(def,"DOSTRONGOVER: %d\n",DOSTRONGOVER);
		fprintf(def,"DOSTRONGUNDER: %d\n",DOSTRONGUNDER);
		fprintf(def,"USEPOOLING: %d\n",USEPOOLING);
		fprintf(def,"TRYMORESUBTREE: %d\n",TRYMORESUBTREE);
		fprintf(def,"INSERTAFTERDEL: %d\n",INSERTAFTERDEL);
		fprintf(def,"3DCOMPUTE: %d\n",COMPUTE3D);
		fprintf(def,"DELAREASORT: %d\n",DELAREASORT);
		fprintf(def,"DELDISTSORT: %d\n",DELDISTSORT);
		fprintf(def,"MULTIREINSERT: %d\n", MULTIREINSERT);
		fprintf(def,"*****************************************\n");
		fprintf(def,"FUTURE: %d\n",FUTURE);
		fprintf(def,"STRONGVERSIONPARA: %f\n",STRONGVERSIONPARA);
		fprintf(def,"STRONGVERUNDER: %f\n",STRONGVERUNDER);
		fprintf(def,"WVERSIONUF: %f\n",WVERSIONUF);
		fprintf(def,"PHYUF: %f\n",PHYUF);
		fprintf(def,"DETRATIO: %f\n",DETRATIO);
		fprintf(def,"GOODRATIO: %f\n",GOODRATIO);
		fprintf(def,"MULTI: %f\n",MULTI);
		fprintf(def,"*****************************************\n");
		fprintf(def,"pure key split: %d\n",purekey);
		fprintf(def,"pure version split: %d\n",purever);
		fprintf(def,"version split + key split: %d\n",verkey);
		fprintf(def,"key split after version copy: %d\n",keyver);
		fprintf(def,"weak version underflow: %d\n",wunder);
		fprintf(def,"strong version underflow: %d\n",verunder);
		fprintf(def,"*****************************************\n");
		fprintf(def,"nerase: %d\n",nerase);
		fprintf(def,"avoided split: %d\n", asplit);
		fprintf(def,"*****************************************\n");
		fprintf(def,"Total version copies: %d\n",purever+verkey+keyver+wunder+verunder);
		fprintf(def,"Total version copies on the leaves: %d\n", leafvc);
		fprintf(def,"Total version copies on the nonleaf nodes: %d\n", nonleafvc);
		
		fclose(def);
	}
	printf("Tree deleted\n");
}

//////////////////////////////////////////////////////////////////////////////

bool MVRTree::delfrometable(MVREntry *e)
{
	int ind=e->son;
	if (entrytable[ind].son==-1)
		return false;
	for (int i=0;i<2*dimension;i++)
		if ((entrytable[ind].bounces[i]-e->bounces[i])>FLOATZERO)
			error("The entry specified for deletion does not match!\n",true);
	entrytable[ind].son=-1;
	etablecount--;
}

//////////////////////////////////////////////////////////////////////////////
void MVRTree::DelMVRNode(MVRTNode **mr)
{
	MVRTNode::deleting=true;
	int b=(*mr)->block;
	blockmem->DelBlock(b);
	(*mr)=NULL;
	MVRTNode::deleting=false;
}
//////////////////////////////////////////////////////////////////////////////
bool MVRTree::FindLeaf(MVREntry *e)
{
	//load_root(e->sttime);
	load_root(rtable.rno[rtable.rtnum-1]);
	return root_ptr->FindLeaf(e);
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::FlushEtable()
{
	if (!USEPOOLING)
		return;
	flushing=true;
	for (int i=0;i<OBJECTNUM;i++)
	{
	  if (entrytable[i].son!=-1)
	  {
		MVREntry *e=new MVREntry(dimension,this);
		for (int j=0;j<2*dimension;j++)
		{
			e->bounces[j]=entrytable[i].bounces[j];
		}
		e->sttime=entrytable[i].sttime;
		e->edtime=entrytable[i].edtime;
		e->son=entrytable[i].son;
		e->level=0;
		e->son=i;
		e->son_ptr=NULL;
		e->my_tree=this;
		printf("flushing object %d from entry table\n",e->son);
		insert(e,false);
		entrytable[i].son=-1;
		etablecount--;
	  }
	}
	flushing=false;
	if (etablecount!=0)
		error("why etablecount not equal to zero after all the entries are flushed?\n",true);
}

//////////////////////////////////////////////////////////////////////////////

int MVRTree::get_root_ts()
{
	if (root_ptr==NULL)
		return -1;
	for (int i=0;i<rtable.rtnum;i++)
	{
		if (rtable.bno[i]==root)
			return rtable.rno[i];
	}
	//The root is not found
	return -1;
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::insert(MVREntry* de, bool reinsert_after_del)
{
	if (!reinsert_after_del)
	{
		if (de->sttime>most_recent_time)
		{
			most_recent_time=de->sttime;
			if (etablecount>0)
				error("you forgot to flush the entry table before advancing to a new timestamp!",true);
		}
		else if (de->sttime<most_recent_time)
			error("Cannot insert an entry of old version.\n",true);
		//first we check if d's edtime has been set to now
		if (de->edtime!=NOWTIME)
		{
			error("MVRTree::insert--The entry to be inserted should have 'now' as ending time.\n",true);
			return;
		}
	}
	
	if (!reinsert_after_del)
	{
		delete reinsert_list;
		reinsert_list=new MVRLinList();
		MVREntry *tmpe=new MVREntry(dimension, NULL);
		(*tmpe)=(*de);
		reinsert_list->insert(tmpe);
		delete de;
	}
	int first = true;
	int num;
	re_level=new bool [256];
	for (int i=0;i<256;i++)
		re_level[i]=false;
	while ((num=reinsert_list->get_num())>0)
	{
		de=reinsert_list->get_first();
		if (!first)
		{
			if (de->level>0)
				printf("Reinserting block %d...",de->son);
			else
				printf("Reinserting object %d...",de->son);
		}
		else
			first = false;
		
		MVREntry *d=new MVREntry(dimension,this);
		(*d)=(*de);
		d->my_tree=this;
		reinsert_list->erase();
		// load root into memory
		load_root(d->sttime);

		R_OVERFLOW ret;
		MVRTNode *new1=NULL, *new2=NULL;
		int st=d->sttime;
		ret=root_ptr->insert2(0,d,&new1,&new2,false);
		//CheckMVRNode(root_ptr);
		switch (ret)
		{
		case KSPLIT: //There has been a key split in the root.  In this case
					 //we create a new root for d and new1
			{
					 MVRTNode *newroot;
					 //newroot=new MVRTNode(this);
					 newroot=NewMVRNode(-1);
					 newroot->level=root_ptr->level+1;
					 increase_node_count(1);
					 MVREntry *ne;
					 ne=new MVREntry(MVRDIM,this);
					 ne->fill_in(root_ptr);
					 newroot->enter(ne);
					 ne=new MVREntry(MVRDIM,this);
					 ne->fill_in(new1);
					 newroot->enter(ne);
					 //We have filled in the two entries in the new root
					 //Now we are to change the root block number info
					 update_root_table(st,newroot->block);
					 newroot->dirty=true;
					 //Now we will change the root and root_ptr
					 root=newroot->block;
					 root_ptr=newroot;
					 break;
			}
		case VSPLIT: //There has been a root of a new version created (new1).
					 //we will set new1 as the new root
					 update_root_table(st,(-1)*new1->block);
					 //Now we change the root and root_ptr
					 //delete root_ptr;
					 DelMVRNode(&root_ptr);
					 root=new1->block;
					 root_ptr=new1;
					 break;
		case KSPLITNN:
		case VSPLITSVOF:
			{

				     MVRTNode *newroot=NewMVRNode(-1);
					 MVREntry *ne1,*ne2;
					 ne1=new MVREntry(MVRDIM,this); ne2=new MVREntry(MVRDIM,this);
					 ne1->fill_in(new1); ne2->fill_in(new2);
					 newroot->enter(ne1); newroot->enter(ne2);
					 update_root_table(most_recent_time,(-1)*newroot->block);
					 //delete root_ptr;
					 DelMVRNode(&root_ptr);
					 root=new1->block;
					 root_ptr=new1;

					 printf("Now we are going to have some unforseen split on the root\n");
					 getchar();

					 break;
			}
		}
	}
	delete [] re_level;
	re_level=NULL; 
}

//////////////////////////////////////////////////////////////////////////////
void MVRTree::InsertDelList(bool asign)
{
	if (delete_list->get_num()==0) return;
	while (delete_list->get_num()>0)
	{
		MVREntry *e=delete_list->get_first();
		if (e->level>0)
			printf("Past inserting block %d\n",e->son);
		else
			printf("Past inserting object %d\n",e->son);
		MVREntry *newe=new MVREntry (dimension,this);
		(*newe)=(*e);
		delete_list->erase();
		newe->my_tree=this;
		bool res=Reinsert(newe,newe->block,asign);
		if (res==false)
			error("Caution: Can't insert an entry that was determined to be insertable!\n",true);
	}
}
//////////////////////////////////////////////////////////////////////////////
void MVRTree::load_root(int ts)
{
	//First of all we will search the root table for tree with the greatest
	//but smaller than ts timestamp
	int croot=-1;   //the candidate croot found so far
	for (int i=rtable.rtnum-1;i>=0;i--)
	{
		//Since the roots are sorted in descending order, so once we find
		//a root that is smaller or equal to ts, we get what we want
		if (rtable.rno[i]<=ts)
		{
			croot=i;
			i=-1;
		}
	}

	if (croot==-1)
		//meaning that the ts requested is illegal
	  return;
    
    if (root_ptr != NULL)
		//Another tree has already be loaded
	{
        //comapre the block of croot with root.  load the new one if they are different
		if (rtable.bno[croot]!=root)
		{
			//Delete the old tree first
			//delete root_ptr;
			DelMVRNode(&root_ptr);
			root=rtable.bno[croot];
            //root_ptr = new MVRTNode(this, root);
			root_ptr=NewMVRNode(root);
		}
	}
	else
		//no tree has been loaded before
	{
		root=rtable.bno[croot];
		//root_ptr=new MVRTNode(this, root);
		root_ptr=NewMVRNode(root);
	}
}
 
//////////////////////////////////////////////////////////////////////////////

void MVRTree::AddNodetoEtable(MVRTNode *rn)
{
	for (int i=0;i<rn->num_entries;i++)
		addtoetable(&(rn->entries[i]));
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::addtoetable(MVREntry *e)
{
	int ind=e->son;
	printf("Inserting oid %d to entry table\n",ind);
	if (entrytable[ind].son!=-1)
		error("the etable has not been flushed or entry is inserted twice!\n",true);
	entrytable[ind].son=e->son;
	entrytable[ind].sttime=e->sttime;
	entrytable[ind].edtime=e->edtime;
	memcpy(entrytable[ind].bounces,e->bounces,2*e->dimension*sizeof(float));
	etablecount++;
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTree::CheckInstability(MVREntry *e, int old_b, bool asign)
{
	//load_root(e->sttime);
	int ts=e->sttime;
	int croot=-1;   //the candidate croot found so far
	for (int i=rtable.rtnum-1;i>=0;i--)
	{
		if (rtable.rno[i]<=ts)
		{	croot=i; i=-1;
		}
	}

	if (croot==-1)
		//meaning that the ts requested is illegal
	  return false;
    
	int rootnum=rtable.bno[croot];
	MVRTNode *tmproot;
	if (rootnum!=root)
		//tmproot=new MVRTNode(this,rootnum);
		tmproot=NewMVRNode(rootnum);
	else
		tmproot=root_ptr;
    
	MVREntry *tmp=new MVREntry(dimension,NULL);
	tmp->fill_in(tmproot);
	tmp->son_ptr=NULL;
	bool res=false;
	if (tmp->branchqual(e,asign,0))
	{
		INSERTABLE cres=tmproot->CheckInstability(e,old_b, asign);
		if (cres==YES)
			res=true;
	}
	delete tmp;
	if (tmproot!=root_ptr) 
		//delete tmproot;
		DelMVRNode(&tmproot);
	return res;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTree::delete_entry(MVREntry *e)
//Before calling this function, the edtime field of e must be set to its deletion time
{
	if (e->edtime>most_recent_time)
	{
		most_recent_time=e->edtime;
		if (etablecount>0)
				error("you forgot to flush the entry table before advancing to a new timestamp!",true);
	}
	if (delfrometable(e))
		return;
	//we need to reset the reinsert_list
	delete reinsert_list;
	reinsert_list=new MVRLinList();
	delete delete_list;
	delete_list=new MVRLinList();
	//Since we only allow deletion of the most recent entries,
	//the appropriate root must be the last one in the roottable;
	load_root(rtable.bno[rtable.rtnum-1]);
	R_DELETE ret=root_ptr->delete_entry(e);
	CheckMVRNode(root_ptr);
	if (ret==NOTFOUND)
		error("Warning: Requested entry for deletion not found\n",true);
	if (root_ptr->num_entries==1)
		error("Handle root physical deletion!\n",true);
	int count=root_ptr->count_alive();
	if (count==1)
		error("Handle root logical deletion!\n",true);
	//Check(this,NULL);
	//InsertDelList(false);
	if (reinsert_list->get_num()>0)
		//We need to reinsert entries
	{
		insert(NULL,true);
	}
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::increase_node_count(int sign)
{
	if (sign==0)
		num_of_dnodes++;
	else
		num_of_inodes++;
}

//////////////////////////////////////////////////////////////////////////////
MVRTNode * MVRTree::NewMVRNode(int reqblock)
{
	MVRTNode *res;
	MVRTNode::creating=true;
	if (reqblock==-1)
	{	
		res=new MVRTNode(this);
		blockmem->AddBlock(res,res->block);
	}
	else
	{
		res=blockmem->LoadBlock(reqblock);
		if (res==NULL)
		{
			res=new MVRTNode(this,reqblock);
			blockmem->AddBlock(res,res->block);
		}
	}
	MVRTNode::creating=false;
	return res;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTree::rangeQuery(float *mbr, SortedMVRLinList *res, char *rec, char *dupblock)
{
    int st=mbr[4];  //the starting time
	int ed=mbr[5];  //the ending time
	//Now the range query algorithm is in the interval basis, meaning that we no longer
	//follow the dumb way of searching timestamp by timestamp.  Instead, we search tree 
	//by tree.  So first of all, we need to decide which trees need to be searched.
	int qb=-1, qe=-1;
	  //qb (query begin) is the ranking number of the first tree in the root table
	  //qe (query end) is the ranking number of the last tree in the root table
	for (int i=1; i<rtable.rtnum; i++)
		//Note that we start searching the roottable from 1
	{
		if (rtable.rno[i]>st && qb==-1) qb=i-1;
		if (rtable.rno[i]>ed && qe==-1) qe=i-1;
	}

	if (qb==-1)
		qb=rtable.rtnum-1;
	if (qe==-1)
		qe=rtable.rtnum-1;

	for (int i=0;i<10000;i++)
		dupblock[i]=0;
	for (int i=qb;i<=qe;i++)
	{
		load_root(rtable.rno[i]);
		if (i==qe)
			mbr[6]=ed;  //meaning that this is the last search
		else
			mbr[6]=-1;  //meaning that this is not the last search
		root_ptr->rangeQuery(mbr,res,rec,dupblock);

		DelMVRNode(&root_ptr);
		root_ptr=NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::read_header(char *buffer)
{
    int i;

    memcpy(&dimension, buffer, sizeof(dimension));
    i = sizeof(dimension);

    memcpy(&num_of_data, &buffer[i], sizeof(num_of_data));
    i += sizeof(num_of_data);

    memcpy(&num_of_dnodes, &buffer[i], sizeof(num_of_dnodes));
    i += sizeof(num_of_dnodes);

    memcpy(&num_of_inodes, &buffer[i], sizeof(num_of_inodes));
    i += sizeof(num_of_inodes);

}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::read_root_table()
{
	FILE *f;
	f=fopen(rtafname,"r");
	//the first integer is the number of roots
	fscanf(f,"%d\n",&(rtable.rtnum));
	for (int i=0;i<rtable.rtnum;i++)
	{
		//First read the root number
		fscanf(f,"%d\n",&(rtable.rno[i]));
		//Second read in the block number for this root
		fscanf(f,"%d\n",&(rtable.bno[i]));
	}
	fclose(f);
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTree::Reinsert(MVREntry *e, int oblock, bool asign)
  //This function can be called when (1) a deletion occurred (2) trying to
  //avoid splitting.
{
	//load_root(e->sttime);
	int ts=e->sttime;
	int croot=-1;   //the candidate croot found so far
	for (int i=rtable.rtnum-1;i>=0;i--)
	{
		if (rtable.rno[i]<=ts)
		{	croot=i; i=-1;
		}
	}
	if (croot==-1)
		//meaning that the ts requested is illegal
	  return false;
   
	int rootnum=rtable.bno[croot];
	MVRTNode *tmproot;
	if (rootnum!=root)
		//tmproot=new MVRTNode(this,rootnum);
		tmproot=NewMVRNode(rootnum);
	else
		tmproot=root_ptr;

	MVREntry *tmp=new MVREntry(dimension,NULL);
	tmp->fill_in(tmproot);
	tmp->son_ptr=NULL;
	bool res=false;
	if (tmp->branchqual(e,asign,0))
		res=tmproot->Reinsert(e,oblock,asign);
	delete tmp;
	//CheckMVRNode(tmproot);
	if (tmproot!=root_ptr)
		DelMVRNode(&tmproot);
	return res;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTree::ResetEntrytable()
{
	for (int i=0;i<OBJECTNUM;i++)
	{
		entrytable[i].son=-1;
	}
}


//////////////////////////////////////////////////////////////////////////////


void MVRTree::update_root_table(int ts, int bno)
{
	if (bno<0)
		//A root for a new timestamp has been created
	{
		//note that ts must be greater than the greatest of current root versions
		rtable.bno[rtable.rtnum]=-1*bno;
		rtable.rno[rtable.rtnum]=ts;
		rtable.rtnum++;
	}
	else
		//We are to update the root number for an already existed
		//root
	{
		//First of all we will search the root table for tree with the greatest
		//but smaller than ts timestamp
		int croot=-1;   //the candidate croot found so far
		for (int i=rtable.rtnum-1;i>=0;i--)
		{
			//Since the roots are sorted in descending order, so once we find
			//a root that is smaller or equal to ts, we get what we want
			if (rtable.rno[i]<=ts)
			{
				croot=i;
				i=-1;
			}
		}

		if (croot==-1)
			//Meaning that the requested root does not exist
		{
		    error("MVRTree::update_root_table--Requested root invalid\n",true);
			return;
		}

		rtable.bno[croot]=bno;
		//Note that we do not change the root and root_ptr here.  This is 
		//addressed by the caller
	}
}


//////////////////////////////////////////////////////////////////////////////

void MVRTree::write_header(char *buffer)
{
    int i;

    memcpy(buffer, &dimension, sizeof(dimension));
    i = sizeof(dimension);

    memcpy(&buffer[i], &num_of_data, sizeof(num_of_data));
    i += sizeof(num_of_data);

    memcpy(&buffer[i], &num_of_dnodes, sizeof(num_of_dnodes));
    i += sizeof(num_of_dnodes);

    memcpy(&buffer[i], &num_of_inodes, sizeof(num_of_inodes));
    i += sizeof(num_of_inodes);
}

//////////////////////////////////////////////////////////////////////////////

void MVRTree::write_root_table()
{
	FILE *f;
	f=fopen(rtafname,"w");

	fprintf(f,"%d\n",rtable.rtnum);
	for (int i=0;i<rtable.rtnum;i++)
	{
		//Write the root number
		fprintf(f,"%d\n",rtable.rno[i]);
        //Write the block number for this root
		fprintf(f,"%d\n",rtable.bno[i]);
	}

	fclose(f);
}

int MVRTNode::create_count=0;
int MVREntry::create_count=0;
int MVRTNode::low_util_count=0;
int MVRTNode::strong_uf_count=0;
bool MVRTNode::creating=false;
bool MVRTNode::deleting=false;

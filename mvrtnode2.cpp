#include "mvrtnode.h"

//////////////////////////////////////////////////////////////////////////////

int MVRTNode::get_header_size()
{
	return sizeof(char) + sizeof(char) + sizeof(char) + sizeof(char);
	//the size for the following: level, num_of_entries, copysign, refcount
}

//////////////////////////////////////////////////////////////////////////////

int MVRTNode::get_height()
{
	if (my_tree==NULL)
	{
		error("MVRTNode::get_height--my_tree equals to NULL\n", true);
		return -1;
	}

	if (my_tree->root_ptr==NULL)
	{
		error("MVRTNode::get_height--root not loaded\n", true);
		return -1;
	}

	return my_tree->root_ptr->level;
}

//////////////////////////////////////////////////////////////////////////////

bool MVRTNode::GetIntvMbr(float *mbr, int st, int ed)
{
	bool res = false;
	for (int i = 0; i < dimension; i++)
	{
		mbr[2*i]   = MAXREAL;
		mbr[2*i+1] = MINREAL;
	}
	for (int i = 0; i < num_entries; i++)
	{
		int minht,maxlt;
		minht = min(entries[i].edtime-1,ed);
		maxlt = max(entries[i].sttime,st);
		if (maxlt <= minht)
		{
			res=true;
			for (int j = 0; j < dimension; j++)
			{
				mbr[2*j] = min(mbr[2*j], entries[i].bounces[2*j]);
				mbr[2*j+1] = max(mbr[2*j+1], entries[i].bounces[2*j+1]);
			}
		}
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////
float* MVRTNode::get_mbr(int alivesign)
{
    int i, j;
    float *mbr;

    mbr = new float[2*dimension];
    for (i = 0; i < 2*dimension; i +=2 )
	{
        mbr[i]   = MAXREAL;
		mbr[i+1] = MINREAL;
	}	

	if (alivesign == 0)
	{
		for (j = 0; j < num_entries; j++)
		{
    		for (i = 0; i < 2*dimension; i += 2)
    		{
    			mbr[i]   = min(mbr[i],   entries[j].bounces[i]);
    			mbr[i+1] = max(mbr[i+1], entries[j].bounces[i+1]);
			}
		}
	}
	else
	{
		for (j = 0; j < num_entries; j++)
		{
            if (entries[j].son>=0)
			{
    			for (i = 0; i < 2*dimension; i += 2)
    			{
    				mbr[i]   = min(mbr[i],   entries[j].bounces[i]);
    				mbr[i+1] = max(mbr[i+1], entries[j].bounces[i+1]);
				}
			}
		}
	}
    return mbr;
}

//////////////////////////////////////////////////////////////////////////////

int * MVRTNode::get_timespan(int alivesign)
{
	int *ts = new int [2];
	int n   = num_entries;
	int i;
	ts[0]   = MAXINT;
	ts[1]   = MININT;

	if (num_entries == 0)
		ts[0] = ts[1] = -1;

	if (alivesign == 0)
	{
		for (i = 0; i < n; i++)
		{
		  if (entries[i].sttime < entries[i].edtime)
		  {
			if (ts[0] > entries[i].sttime)
				ts[0] = entries[i].sttime;
			if (ts[1] < entries[i].edtime)
				ts[1] = entries[i].edtime;
		  }
		}
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			if (entries[i].son >= 0)
			{
			  if (entries[i].sttime <= entries[i].edtime)
			  {
				if (ts[0] > entries[i].sttime)
					ts[0] = entries[i].sttime;
				if (ts[1] < entries[i].edtime)
					ts[1] = entries[i].edtime;
			  }
			}
		}
	}
	return ts;
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::HandleToFull(int addnum,MVREntry *e1, MVREntry *e2)
{
	if (MULTIREINSERT)
		return HandleToFull2(addnum,e1,e2);
	if (!AVOIDLEAFSPLIT)
		error("The current setting says no leaf split avoidance.\n",true);
	if (addnum != 1)
		error("Sorry...The cuurent HandleToFull only handles addnum=1!",true);
	if (level == get_height())
		return false;
	if (check_all_now())
		return false;

	int *res = new int[capacity];
	int resnum;
	DelCan(e1,res,resnum);
	for (int i = 0; i < resnum; i++)
	{
		int index = res[i];  //index is the deletion candidate being considered
		if (my_tree->CheckInstability(&entries[index],block,false))
		{
			MVREntry *e=new MVREntry(dimension,my_tree);
			(*e)=entries[index]; e->level=0; e->block=block;
			for (int j=0;j<=get_height();j++)
				e->insertpath[j]=entries[index].insertpath[j];
			phydelentry(index);
			dirty=true;
			//*******Code changed by TAO Yufei*******
			//my_tree->Reinsert(e,block,true);
			my_tree->Reinsert(e,block,false);
			//***************************************
			printf("Avoided a split\n");
			my_tree->asplit++;
			delete [] res;
			return true;
		}
	}
	delete [] res;
	return false;
}
//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::HandleToFull2(int addnum, MVREntry *e1, MVREntry *e2)
{
	if (!AVOIDLEAFSPLIT)
		error("The current setting says no leaf split avoidance.\n",true);
	if (addnum != 1)
		error("Sorry...The cuurent HandleToFull only handles addnum=1!",true);
	if (!MULTIREINSERT)
		error("The current setting says you should calll HandleToFull()\n",true);
	if (level==get_height())
		return false;
	if (check_all_now())
		return false;

	int *res         = new int[capacity];
	int resnum       = 0;
	bool result      = false;
	bool lastsuccess = true;
	
	DelCan(e1,res,resnum);
	while (resnum > 0 && num_entries > MULTI*capacity && lastsuccess)
	{
		lastsuccess = false;
		for (int i  = 0; i < resnum; i++)
		{
			int index = res[i];
			if (my_tree->CheckInstability(&entries[index],block,false))
			{
				MVREntry *e = new MVREntry(dimension,my_tree);
				(*e) = entries[index]; e->level = 0; e->block = block;
				for (int j = 0; j <= get_height(); j++)
					e->insertpath[j] = entries[index].insertpath[j];
				phydelentry(index);
				dirty = true;
				//*****Code changed by TAO Yufei*****
				//my_tree->Reinsert(e,block,true);
				my_tree->Reinsert(e,block,false);
				//***********************************
				printf("Reinsert success\n");
				my_tree->asplit++;
				result = true; i = resnum; lastsuccess=true;
			}
		}
		if (lastsuccess)
			DelCan(e1,res,resnum);
	}
	delete [] res;
	return result;
}
//////////////////////////////////////////////////////////////////////////////
R_OVERFLOW MVRTNode::insert2(int pst, MVREntry *d, MVRTNode **sn1, MVRTNode **sn2, bool force)
{
	//We check if d is valid
	if ((d->check_valid()) == false)
		error("MVRTNode::insert--Something wrong with entry d\n",true);
    if (level == 0)
	{
		int st = d->sttime;
		if (AVOIDLEAFSPLIT && !force && !check_all_now())
			//force means this step has been tried and no candidate entry is found
			//so this time we will insert the entry ordinarily and split if necessary
		{
			if (check_to_full(1))
				if (!HandleToFull(1,d,NULL))
					return TOSPLIT;
		}
		enter(d);
		dirty = true;
		//Note that when this function returns, entry d will have been deleted
		if (check_full())
			//block overflow
		{
			R_OVERFLOW ret;
			ret   = TreatOverflow(pst,sn1,sn2,1);
			dirty = true;
			return ret;
		}
		else
			return NONE;
	}
	else  //Now we come to handle the non-leaf case
	{
		int st = d->sttime;
		if (level == d->level)
        {
			enter(d);
			dirty = true;
			if (check_full())
			{
			  R_OVERFLOW ret;
			  ret   = TreatOverflow(pst, sn1, sn2,1);
			  dirty = true;
			  return ret;
			}
			return NONE;
		}

		int *cand = new int[num_entries];
		int candsize;
		int follow;  //which branch we shall go into next to insert the node?
	    follow = choose_subtree2(d,cand,candsize,true,false);
		if (candsize > 1 && !TRYMORESUBTREE)
			error("The current setting says no trying more subtrees.\n",true);
		if (follow == -1)
			error("no subtree can be found\n",true);
		if (level > 1)  //We adopt the multireinsert policy for the leaves only
			candsize=1;
		if (follow >= num_entries)
			error("Hey, the subtree selected is out of num_entries!\n",true);
		bool forceleaf = false;
		for (int i = 0; i < candsize; i++)
		{
			follow = cand[i];
			R_OVERFLOW ret;
			//Now we are to traverse into the children
			MVRTNode *succ = entries[follow].get_son();
			MVRTNode *new1 = NULL, *new2 = NULL;
			ret = succ->insert2(entries[follow].sttime,d,&new1,&new2,forceleaf);
			//if (ret!=REINSERT || succ->num_entries>0)
		    //		CheckMVRNode(succ);
			if (ret != TOSPLIT)
			{
				delete [] cand;
				entries[follow].fill_in(succ);
				//This entry will change only after a KSPLIT
				dirty = true;
				if (entries[follow].sttime == entries[follow].edtime || ret==REINSERT)
				{
					succ->refcount--;
					phydelentry(follow);
				}
			}
			switch (ret)
			{
			case VSPLIT:
			case KSPLIT:  //for these two cases, only one entry needs to be inserted into this node
						  //because of the node split of the child node.  In this case, only new1 is 
						  //meaningful
						  MVREntry *ne;
						  ne = new MVREntry(dimension, my_tree);
						  ne->fill_in(new1);
						  if (ret == VSPLIT && ne->sttime != my_tree->most_recent_time)
							  error("mvrtnode::insert--hey, why sttime does not equal to most recent time?\n",true);
						  
						  enter(ne);
						  dirty = true;
					  		  //Note that newe will be deleted by enter().  So don't worry
						  if (check_full())
							  //This node is full
						  {
							  R_OVERFLOW nret;
							  nret = TreatOverflow(pst, sn1, sn2,1);
							  if (num_entries==capacity-1)
							  {
								  error("num_entries exceeds capacity (but may not be a problem)",false);
								  //char c=getchar();
							  }
							  return nret;
						  }
						  else
							  return NONE;
						  break;
			case VSPLITSVOF: {
							  //for this case, two entries will be inserted.  new1 and new2 are both meaningful
						      if (!DOSTRONGOVER)
								  error("The current setting says no strong overflow.\n",true);
							  MVREntry *ne1 = new MVREntry(MVRDIM, my_tree);
							  MVREntry *ne2 = new MVREntry(MVRDIM, my_tree);
							  ne1->fill_in(new1);
							  ne2->fill_in(new2);
							  if (ne1->sttime!=my_tree->most_recent_time ||
								  ne2->sttime!=my_tree->most_recent_time)
								  error("hey, why ne1 or ne2's sttime not equal to most_recent_time in mvrtnode::insert?",true);
							  enter(ne1);
							  enter(ne2);
							  dirty = true;
							  if (check_full())
							  {
								  R_OVERFLOW nret;
								  nret  = TreatOverflow(pst,sn1,sn2,2);
								  return nret;
							  }
							  else
								  return NONE;
							  break;
							 }
			case KSPLITNN: //In this case key split was performed but a new
						   //node has been created because there are entries with earlier
					       //starting time
							  
							dirty=true;
							MVREntry *ne1;
							MVREntry *ne2;
							ne1 = new MVREntry(MVRDIM, my_tree);
							ne2 = new MVREntry(MVRDIM, my_tree);
							ne1->fill_in(new1);
							ne2->fill_in(new2);
							enter(ne1);	enter(ne2);
							if (check_full())
							{
							  R_OVERFLOW nret;
							  nret  = TreatOverflow(pst, sn1, sn2,2);
							  return nret;
							}
							else	
							  return NONE;
							break;
			case STRONGUNDER:
			case REINSERT:
			case NONE:  //Notice that it is likely the physical deletion will cause underflow.  In this
						//case we will reinsert all the alive entries in this node
				        if (!DOSTRONGUNDER && ret == STRONGUNDER)
							printf("The current setting says no strong underflow.\n");
						if (check_underflow())
						{
							add_reinsert_alive();
							if (num_entries>0)
								return NONE;
							else
								return REINSERT;
						}
						return NONE;
			case TOSPLIT:
						 if (i == candsize-1) //meaning that all possible branches have been tried
						 {
							 forceleaf = true;
							 i         = -1;
						 }
						 break;
			}
		}
	}

	return NONE;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::phydelentry(int index)
{
	if (level == 0)
		printf("Physically deleting oid %d\n",entries[index].son);
	else
		printf("Physically deleting block %d\n",entries[index].son);
	
	if (entries[index].son_ptr != NULL)
	  my_tree->DelMVRNode(&entries[index].son_ptr);
	int n = num_entries;
	int i;
	for (i = index; i < n - 1; i++)
	{
		entries[i] = entries[i+1];
	}
	for (i = n-1; i < capacity; i++)
	{
		entries[i].son_ptr = NULL;
	}
	num_entries--;
    dirty=true;
}

//////////////////////////////////////////////////////////////////////////////

void MVRTNode::rangeQuery(float *mbr, SortedMVRLinList *res,char *rec, char *dupblock)
{
    int i;
	
	int st=mbr[4],ed=mbr[5],nt=mbr[6];
	//st: starting query time
	//ed: ending query time
	//nt: the query time now
    
	int n;      //num_entries of this node
    n = num_entries;
	if (dupblock[block]==1) return;
	dupblock[block]=1;
    for (i = 0; i < n; i++)
    {
       if (level==0)
	   {
		  if (entries[i].check_intersect(mbr))
		  {
				if (rec[abs(entries[i].son)]==0)
				//rec[entries[i].son]=0 means this oid has not been reported
				{
					MVREntry *copy;
					copy = new MVREntry();
        			*copy = entries[i];
        			res->insert(copy);
					rec[abs(entries[i].son)]=1;
				}
          }
	   }
       else
       {   ///*
		   int bnum=entries[i].son;

		   //if (dupblock[bnum]==0 && entries[i].check_intersect(mbr))
		   if (entries[i].check_intersect(mbr))
		   {
			   MVRTNode *succ;
			   succ=entries[i].get_son();
			   succ->rangeQuery(mbr,res,rec,dupblock);
		   }
		   //*/
		   /*
		   if (entries[i].check_intersect(mbr))
		   {
				if (ed==nt)
					//if nt is the last time then go search anyway
				{
					MVRTNode *succ;
					succ=entries[i].get_son();
					succ->rangeQuery(mbr,res,rec,dupblock);
				}
				else
				{
					MVRTNode *succ;
					succ=entries[i].get_son();
					int *span;
					span=succ->get_timespan(0);
					int tmp=span[1];
					delete [] span;
					if (entries[i].edtime>=tmp)
						succ->rangeQuery(mbr,res,rec,dupblock);
					else
						my_tree->wasted++;
				}
		   }
		   */
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
bool MVRTNode::Reinsert(MVREntry *e, int oblock, bool asign)
{
	if (block != e->insertpath[level])
		error("Warning: the actual block number does agree with the pre-determined one!\n",true);
	if (level == 0)
	{
		if (check_full())
			return false;
		if (block == oblock)
			return false;
		enter(e);
		dirty = true;
		return true;
	}
	else
	{
		if (level == e->level)
		{
			if (check_full())
				return false;
			if (oblock == block)
				return false;
			enter(e);
			dirty = true;
			return true;
		}
		int tarblock = e->insertpath[level-1];
		int follow   = -1;
		for (int i = 0; i < num_entries; i++)
			if (entries[i].son == tarblock)
			{
				follow = i; i = num_entries;
			}
		if (follow == -1)
			error("The block in the insert path cannot be found!\n",true);
		if (!entries[follow].branchqual(e,asign,level))
			error("The path is no longer valid!\n",true);
	    MVRTNode *succ = entries[follow].get_son();
		if (succ->Reinsert(e,oblock,asign))
		{
			//CheckMVRNode(succ);
			int old = entries[follow].edtime;
			entries[follow].fill_in(succ);
			if (old == NOWTIME && entries[follow].edtime != NOWTIME)
				error("Something wrong with the ending time of this entry in mvrtnode::reinsert\n",true);
			dirty=true;
			return true;
		}
		return false;
	}
}
//////////////////////////////////////////////////////////////////////////////	
void MVRTNode::read_from_buffer(char *buffer)
{
    int i, j, s;
    memcpy(&level, buffer, sizeof(char));
    j = sizeof(char);
    memcpy(&num_entries, &(buffer[j]), sizeof(char));
    j += sizeof(char);
	memcpy(&copysign, &(buffer[j]), sizeof(char));
	j += sizeof(char);
	memcpy(&refcount,&(buffer[j]),sizeof(char));
	j += sizeof(char);
    s = entries[0].get_size();
    for (i = 0; i < num_entries; i++)
    {
    	entries[i].read_from_buffer(&buffer[j]);
    	j += s;
    }
}

//////////////////////////////////////////////////////////////////////////////
void MVRTNode::SortCen(int *res, int &resnum)
{
	IndexSt *ist = new IndexSt(num_entries);
	MVREntry *e  = new MVREntry(dimension,NULL);
	e->GetBound(-1,this);
	for (int i = 0; i < num_entries; i++)
	{
		int ind = i;
		float dist = EntryCenDistance(e,&entries[ind]);
		//dist*=-1;  Since we sort the entries in ascending order we delete this line.
		ist->Add(dist,0,0,ind);
	}
	delete e;
	if (ist->effnum != num_entries)
		error("mvrtnode::sortcen--why this number not equal to num_entries?\n",true);
	resnum = num_entries;
	for (int i = 0; i < num_entries;i++)
	{
		float f[3]; int ind;
		ist->GetItem(f,ind,i);
		res[i] = ind;
	}
	delete ist;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::SortDelCan(int *res, int resnum)
{
	if (DELAREASORT)
	{
		SortDelCan2(res,resnum);
		return;
	}
	if (DELDISTSORT)
    {
		SortDelCan3(res,resnum);
		return;
	}
	int nowv   = my_tree->most_recent_time;
	if (resnum <= 0) return;
	int *tmp   = new int[resnum];
	int num    = 1;
	tmp[0]     = res[0];
	for (int i = 1; i < resnum; i++)
	{
		int len = min(nowv,entries[res[i]].edtime)-entries[res[i]].sttime;
		bool alive;
		if (entries[res[i]].edtime == NOWTIME) alive = true;
		else alive = false;
		if (alive == true)
		{
			int j = 0;
			bool goon = true;
			while (j < num && entries[tmp[j]].edtime != NOWTIME)
				j++;
			while (j < num && goon)
			{
				int len2 = min(nowv,entries[tmp[j]].edtime) - entries[tmp[j]].sttime;
				if (len2 <= len) goon = false;
				else j++;
			}
			for (int k = num; k > j; k--)
				tmp[k] = tmp[k-1];
			tmp[j]     = res[i];
			num ++;
		}
		else
		{
			int j     = 0;
			bool goon = true;
			while (j < num && entries[tmp[j]].edtime != NOWTIME && goon)
			{
				int len2 = min(nowv,entries[tmp[j]].edtime) - entries[tmp[j]].sttime;
				if (len2 >= len) goon = false;
				else j++;
			}
			for (int k = num;k > j; k--)
				tmp[k] = tmp[k-1];
			tmp[j]     = res[i];
			num++;
		}
	}
	for (int i = 0; i < num; i++)
		res[i] = tmp[i];
	delete [] tmp;
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::SortDelCan2(int *res, int resnum)
{
	int ed   = FUTURE+my_tree->most_recent_time;
	int nowv = my_tree->most_recent_time;
	if (!DELAREASORT)
		error("The current setting says no sorting by area in deletion candidates\n",true);
	IndexSt *ist = new IndexSt(resnum);
	MVREntry *e  = new MVREntry(dimension,NULL);
	e->GetBound(-1,this);
	float old_area;
	if (COMPUTE3D)
		old_area = Area3D(e,ed);
	else
		old_area = area(dimension,e->bounces);
	delete e;
	for (int i = 0; i < resnum; i++)
	{
		int ind     = res[i];
		MVREntry *e = new MVREntry(dimension,NULL);
		float narea;

		e->GetBound(ind,this);
		if (COMPUTE3D)
			narea = Area3D(e,ed);
		else
			narea = area(dimension,e->bounces);
		delete e;
		float darea = narea-old_area;

		int alive = 0;
		if (entries[ind].edtime == NOWTIME) alive = 1;
		int len = min(nowv,entries[ind].edtime) - entries[ind].sttime;

		ist->Add(darea,alive,len,ind);
	}
	for (int i = 0; i < resnum; i++)
	{
		float f[3]; int ind;
		ist->GetItem(f,ind,i);
		res[i] = ind;
	}
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::SortDelCan3(int *res, int resnum)
{
	int nowv = my_tree->most_recent_time;
	if (!DELDISTSORT)
		error("The current setting says no sorting by area in deletion candidates\n",true);
	IndexSt *ist = new IndexSt(resnum);
	MVREntry *e  = new MVREntry(dimension,NULL);
	e->GetBound(-1,this);
	for (int i = 0; i < resnum; i++)
	{
		int ind    = res[i];
		float dist = EntryCenDistance(e,&entries[ind]);
		dist       *= -1;
		int alive  = 0;
		if (entries[ind].edtime == NOWTIME) alive=1;
		int len    = min(nowv,entries[ind].edtime) - entries[ind].sttime;

		ist->Add(dist,alive,len,ind);
	}
	delete e;
	for (int i = 0; i < resnum; i++)
	{
		float f[3]; int ind;
		ist->GetItem(f,ind,i);
		res[i] = ind;
	}
}
//////////////////////////////////////////////////////////////////////////////
void MVRTNode::split(MVRTNode *sn)
// splittet den aktuellen Knoten so auf, dass m mbr's nach sn verschoben
// werden
{
    int i, *distribution, dist, n;
    float **mbr_array;
    MVREntry *new_entries1, *new_entries2;

#ifdef SHOWMBR
	split_000++;
#endif

    // wieviele sind denn nun belegt?
    n = num_entries;

    // mbr_array allokieren und belegen
    mbr_array = new floatptr[n];
    for (i = 0; i < n; i++)
       	mbr_array[i] = entries[i].bounces;

    // Verteilung berechnen
    dist = split(mbr_array, &distribution);

    // neues Datenarray erzeugen
    // --> siehe Konstruktor
    new_entries1 = new MVREntry[capacity];
    new_entries2 = new MVREntry[capacity];

    for (i = 0; i < dist; i++)
    {
       	new_entries1[i] = entries[distribution[i]];
    }

    for (i = dist; i < n; i++)
    {
       	new_entries2[i-dist] = entries[distribution[i]];
    }

    // Datenarrays freigeben
    // da die Nachfolgeknoten aber nicht geloescht werden sollen
    // werden vor dem Aufruf von delete noch alle Pointer auf NULL gesetzt
    for (i = 0; i < n; i++)
    {
       	entries[i].son_ptr = NULL;
       	sn->entries[i].son_ptr = NULL;
    }
    delete [] entries;
    delete [] sn->entries;

    // neue Datenarrays kopieren
    entries = new_entries1;
    sn->entries = new_entries2;

    // Anzahl der Eintraege berichtigen
    num_entries = dist;
    sn->num_entries = n - dist;  // muss wegen Rundung so bleiben !!

    delete [] mbr_array;
	delete [] distribution;
}

///////////////////////////////////////////////////////////////////////////

int MVRTNode::split(float **mbr, int **distribution)
{
    bool lu;
    int i, j, k, l, s, n, m1, dist, split_axis;
    SortMbr *sml, *smu;
    float minmarg, marg, minover, mindead, dead, over,
	*rxmbr, *rymbr;

    n = num_entries;

	if (!DOSTRONGUNDER)
	{
		m1 = (int)((float)capacity * WVERSIONUF);
		if ((float)m1<capacity*WVERSIONUF)
			m1++;
	}
	else
	{
		m1 = (int)((float)capacity *STRONGVERUNDER);
		if ((float)m1<capacity*STRONGVERUNDER)
			m1++;
	}

    // sort by lower value of their rectangles
    sml = new SortMbr[n];
    smu = new SortMbr[n];
    rxmbr = new float[2*dimension];
    rymbr = new float[2*dimension];

    // choose split axis
    minmarg = MAXREAL;
    for (i = 0; i < dimension; i++)
    // for each axis
    {
        for (j = 0; j < n; j++)
        {
            sml[j].index = smu[j].index = j;
            sml[j].dimension = smu[j].dimension = i;
            sml[j].mbr = smu[j].mbr = mbr[j];
        }

        // Sort by lower and upper value perpendicular axis_i
      	qsort(sml, n, sizeof(SortMbr), sort_lower_mbr);
        qsort(smu, n, sizeof(SortMbr), sort_upper_mbr);

        marg = 0.0;
            for (k = 0; k < n - 2*m1 + 1; k++)
        {
            // now calculate margin of R1
			marg+=SortMbrMargin(0,m1+k-1,dimension,sml);
            // now calculate margin of R2
			marg+=SortMbrMargin(m1+k,n-1,dimension,sml);
        }
        // for all possible distributions of smu
       	for (k = 0; k < n - 2*m1 + 1; k++)
        {
            // now calculate margin of R1
			marg+=SortMbrMargin(0,m1+k-1,dimension,smu);
            // now calculate margin of R2
            marg+=SortMbrMargin(m1+k,n-1,dimension,smu);
        }
        if (marg < minmarg)
        {
            split_axis = i;
            minmarg = marg;
        }

    }
    // choose best distribution for split axis
    for (j = 0; j < n; j++)
    {
		sml[j].index = smu[j].index = j;
		sml[j].dimension = smu[j].dimension = split_axis;
		sml[j].mbr = smu[j].mbr = mbr[j];
    }

    // Sort by lower and upper value perpendicular split axis
    qsort(sml, n, sizeof(SortMbr), sort_lower_mbr);
    qsort(smu, n, sizeof(SortMbr), sort_upper_mbr);
    minover = MAXREAL;
    mindead = MAXREAL;
    // for all possible distributions of sml and snu
    for (k = 0; k < n - 2*m1 + 1; k++)
    {
        dead = 0.0;
		dead+=SortMbrArea(0,m1+k-1,dimension,sml);
		for (l = 0; l < m1+k; l++)
		{
			dead -= area(dimension, sml[l].mbr);
		}
		dead+=SortMbrArea(m1+k,n-1,dimension,sml);
		for (l=m1+k; l < n; l++)
		{
            dead -= area(dimension, sml[l].mbr);
		}
		over = SortMbrOverlap(0,m1+k,n-1,dimension,sml);
        if ((over < minover) ||
	    (over == minover) && dead < mindead)
        {
            minover = over;
            mindead = dead;
            dist = m1+k;
            lu = TRUE;
        }

        dead = 0.0;
		dead+=SortMbrArea(0,m1+k-1,dimension,smu);
		for (l = 0; l<m1+k; l++)
		{
			dead -= area(dimension, smu[l].mbr);
		}
		dead+=SortMbrArea(m1+k,n-1,dimension,smu);

		for (l=m1+k; l<n; l++)
		{
			dead -= area(dimension, smu[l].mbr);
		}
		over = SortMbrOverlap(0,m1+k,n-1,2,smu);
        if ((over < minover) ||
	    (over == minover) && dead < mindead)
        {
            minover = over;
            mindead = dead;
            dist = m1+k;
            lu = FALSE;
        }
    }

    // calculate best distribution
    *distribution = new int[n];
    for (i = 0; i < n; i++)
    {
        if (lu)
            (*distribution)[i] = sml[i].index;
        else
            (*distribution)[i] = smu[i].index;
    }
    delete sml;
    delete smu;
    delete rxmbr;
    delete rymbr;
    return dist;
}

//////////////////////////////////////////////////////////////////////////////

R_OVERFLOW MVRTNode::TreatOverflow(int nver, MVRTNode **sn1, MVRTNode **sn2, int addnum)
//When this function is called, ther is an overflow occurred.
//nver is the now version.  To call this function, sn1 and sn2 should not have been inited
//as we don't know how many of them will be created
{
	if (num_entries < capacity - 1)
	  return NONE;  
	bool obo = check_dead();
	int nowt = my_tree->most_recent_time;
	int i;
	if (obo)
	{
		(*sn1)        = my_tree->NewMVRNode(-1);
		(*sn1)->level = level;
		(*sn1)->dirty = true;
		my_tree->increase_node_count(level);
		int copycount = 0;  //This field counts the number of entries copied
		copycount     = VersionCopy(*sn1,nowt);
		if (DOSTRONGOVER && 
			copycount >= roundup(STRONGVERSIONPARA * (float)capacity))
		{
			(*sn2)        = my_tree->NewMVRNode(-1);
			(*sn2)->level = level;
			(*sn2)->dirty = true;
			my_tree->increase_node_count(level);
			//sn1 split ==> sn1 & sn2
			(*sn1)->split(*sn2);
			my_tree->verkey++;
			return VSPLITSVOF;
		}
		else
			if (DOSTRONGUNDER && USEPOOLING 
				&& copycount < (STRONGVERUNDER * (float)capacity) 
				&& level == 0 && level != get_height() 
				&& !my_tree->flushing)
			{
				my_tree->AddNodetoEtable(*sn1);
				for (int i = 0; i < (*sn1)->num_entries; i++)
					(*sn1)->entries[i].son_ptr = NULL;
				my_tree->DelMVRNode(sn1);
				(*sn1) = NULL;
				my_tree->verunder++;
				return NONE;
			}
			else 
				if (DOSTRONGUNDER && 
				    copycount < (STRONGVERUNDER * (float)capacity) 
					&& level != get_height())
			{
				for (int i = 0; i < (*sn1)->num_entries; i++)
				{
					MVREntry *e = new MVREntry(dimension,my_tree);
					*e          = (*sn1)->entries[i];
					e->level    = level;
					e->son_ptr  = NULL;
					my_tree->reinsert_list->insert(e);
					(*sn1)->entries[i].son_ptr=NULL;
				}
				my_tree->DelMVRNode(sn1);
				(*sn1) = NULL;
				my_tree->verunder++;
				return STRONGUNDER;
			}
		my_tree->purever++;
		return VSPLIT;
	}
	else
	{
		bool allnow = true;
		for (int i  = 0; i < num_entries; i++)
			if (entries[i].sttime != my_tree->most_recent_time)
			{
				allnow = false; i = num_entries;
			}
		if (refcount == 0 && allnow)
		{
			if (my_tree->re_level[level]  == false && 
				KEYSPLITREINSERT && level != get_height())
			{
				my_tree->re_level[level] = true;
				int listnum; int *list   = new int[num_entries];
				SortCen(list,listnum);
				int lastcand             = (float)num_entries*MULTI;
				MVREntry *new_entries    = new MVREntry[capacity];
				for (int i = 0; i < lastcand; i++)
				{
					new_entries[i]       = entries[list[i]];
					entries[list[i]].son_ptr = NULL;
				}
				for (int i = lastcand; i < num_entries; i++)
				{
					int ind     = list[i];
					MVREntry *e = new MVREntry(dimension,my_tree);
					(*e)        = entries[ind]; 
					e->level    = level; 
					e->son_ptr  = NULL;
					my_tree->reinsert_list->insert(e);
				}
				delete [] entries;
				entries     = new_entries;
				num_entries = lastcand;
				return NONE;
			}
			(*sn1)        = my_tree->NewMVRNode(-1);
			(*sn1)->level = level;
			my_tree->increase_node_count(level);
			split((*sn1));
			dirty         = true;
			(*sn1)->dirty = true;
			my_tree->purekey++;
			return KSPLIT;
		}
		else
		{
			(*sn1)        = my_tree->NewMVRNode(-1);
			(*sn1)->level = level;
            my_tree->increase_node_count(level);
			VersionCopy((*sn1),my_tree->most_recent_time);
			//remember that we still need to split the new node
			//(*sn2)=new MVRTNode(my_tree);
			(*sn2)        = my_tree->NewMVRNode(-1);
			my_tree->increase_node_count(level);
			(*sn1)->split(*sn2);
			(*sn2)->level = level;
			(*sn1)->dirty = (*sn2)->dirty = true;
	        copysign = 2;
			dirty=true;
			my_tree->keyver++;
			return KSPLITNN;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

int MVRTNode::VersionCopy(MVRTNode *sn1, int nowt)
//Before calling this function, there must have been an overflow
//in this node.  Remember to initialize sn1 before use.
{
	int i;
	int copycount = 0;

	sn1->dirty    = true;

	for (i = 0; i < num_entries; i++)
	{
		if (entries[i].edtime == NOWTIME && entries[i].sttime != nowt)
		{
			int n           = sn1->num_entries;
			sn1->entries[n] = entries[i];
			sn1->entries[n].sttime = nowt;
			  //Since we have a copy for sn2 before nver time, we can
			  //shorten the life span in this entry, which will shorten
			  //the life span of the parent nodes
			entries[i].edtime = nowt;
			
			if (level >= 1)
			{
				float *nmbr     = new float[2*dimension];
				MVRTNode *succ = entries[i].get_son();
				succ->refcount++;
				succ->dirty    = true;
				if (succ->GetIntvMbr(nmbr, sn1->entries[n].sttime, sn1->entries[n].edtime))
				{
					memcpy(sn1->entries[n].bounces,nmbr,sizeof(float)*2*dimension);
				}
				else
					error("why there is no entries in this range?\n",true);
				delete [] nmbr;
			}
            if (entries[i].son_ptr) //We don't need this condition check now
			{
				my_tree->DelMVRNode(&entries[i].son_ptr);
				entries[i].son_ptr      = NULL;
				sn1->entries[n].son_ptr = NULL;
			} //we need to delete because now 2 pointers pointing to the child
			sn1->num_entries++;
			copycount++;
		}
		else if (entries[i].edtime == NOWTIME && entries[i].sttime == nowt)
		{
			int n                  = sn1->num_entries;
			sn1->entries[n]        = entries[i];
			sn1->entries[n].sttime = nowt;
			  //Since we have a copy for sn2 before nver time, we can
			  //shorten the life span in this entry, which will shorten
			  //the life span of the parent nodes
			entries[i].edtime      = nowt - 1;
            if (entries[i].son_ptr)
			{
				my_tree->DelMVRNode(&entries[i].son_ptr);
				entries[i].son_ptr = NULL;
				sn1->entries[n].son_ptr = NULL;
			} //we need to delete because now 2 pointers pointing to the child
			phydelentry(i);  //This is the difference of this case from the previous case
			i--;
			sn1->num_entries++;
			copycount++;
		}
	}
	dirty=true;
	copysign=1;
	if (copycount < (capacity*WVERSIONUF) && level!=get_height())
		error("why are there so few alive entries in this node?\n",true);
	if (num_entries > 0 && level == 0)
		my_tree->leafvc++;
	else if (num_entries > 0 && level > 0)
		my_tree->nonleafvc++;
	return copycount;
}

//////////////////////////////////////////////////////////////////////////////

void MVRTNode::write_to_buffer(char *buffer)
{
    int i, j, s;

    memcpy(buffer, &level, sizeof(char));
    j = sizeof(char);
    memcpy(&(buffer[j]), &num_entries, sizeof(char));
    j += sizeof(char);
	memcpy(&(buffer[j]), &copysign, sizeof(char));
    j += sizeof(char);
	memcpy(&(buffer[j]),&refcount,sizeof(char));
	j += sizeof(char);
    s = entries[0].get_size();
    for (i = 0; i < num_entries; i++)
    {
    	entries[i].write_to_buffer(&buffer[j]);
       	j += s;
    }
}
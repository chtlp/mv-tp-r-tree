#include "st2.h"
#include "math.h"
////////////////////////////////////////////////////////////////////

float Area3D(MVREntry *e, int maxed)
{
	int dimension=e->dimension;
	float *mbr=new float[2*dimension+2];
	memcpy(mbr,e->bounces,2*dimension*sizeof(float));
	mbr[2*dimension]=e->sttime;
	mbr[2*dimension+1]=min(maxed,e->edtime);
	//float res=area(dimension+1,mbr);
	float product=1;
	for (int i=0;i<dimension+1;i++)
	{
		float len=mbr[2*i+1]-mbr[2*i];
		if (i==dimension) len=sqrt(len);
		product*=len;
	}
	delete [] mbr;
	return product;
}

//////////////////////////////////////////////////////////////////////////////

void CheckMVRNode(MVRTNode *rn)
{
	for (int i=0;i<rn->num_entries;i++)
	{
		if ((rn->entries[i].check_valid())==false)
			error("Integrity check failed\n",true);
		/*
		if (rn->level>=1)
		{
			MVRTNode *succ=rn->entries[i].get_son();
			float *mbr;
			float nmbr[4];
			//bool res=GetIntvMbr(succ,nmbr,rn->entries[i].sttime,rn->entries[i].edtime);
			mbr=succ->get_mbr(0);
			//if (res=false)
			//	error("why the subtree has no qualifying entry?\n",true);
			if (succ->refcount==0)
			{
				for (int j=0;j<3;j++)
					if (mbr[j]!=rn->entries[i].bounces[j])
						error("failed on bounces\n",true);
			}
			else
			{
				for (int j=0;j<1;j++)
				{
					if (mbr[2*i]>rn->entries[i].bounces[2*j])
						error("failed on bounces\n",true);
					if (mbr[2*j+1]<rn->entries[i].bounces[2*j+1])
						error("failed on bounces\n",true);
				}
			}
			delete [] mbr;
		}
		*/
	}
	rn->check_ver();
}

////////////////////////////////////////////////////////////////////
int CombSingleQ(E3RTree *e3r, MVRTree *mv, float *mbr, int *time, int &count)
  //This function handles the query together with both trees and return the 
  //io cost.  
  //please note that mbr should have been initialized with size 2*dimension+2
{
	int st=time[0], midt=time[1], ed=time[2];
	int oldio=mv->file->iocount;
	int dim=mv->dimension;
	

	char *rec=new char[OBJECTNUM];
	for (int i=0;i<OBJECTNUM;i++)
		rec[i]=0;
	char *dup=new char[MAXBLOCK];
	for (int i=0;i<MAXBLOCK;i++)
		dup[i]=0;

	SortedMVRLinList *res=new SortedMVRLinList();
	mbr[2*dim]=st; mbr[2*dim+1]=midt;
	//************
	mv->rangeQuery(mbr,res,rec,dup);
	//************
	count=res->get_num();
	delete res;
	int newio=mv->file->iocount;
	int totalio=newio-oldio;

	SortedE3RLinList *eres=new SortedE3RLinList();
	char *erec=new char[OBJECTNUM];
	for (int i=0;i<OBJECTNUM;i++)
		erec[i]=0;
	int *level=new int [10];
	for (int i=0;i<9;i++)
		level[i]=0;
	int eoldio=e3r->file->iocount;
	mbr[2*dim]=midt+1; mbr[2*dim+1]=ed;
	for (int i=0;i<2*dim;i++)
		mbr[i]*=E3RSCALE;
	//*************
	e3r->rangeQuery(mbr,eres,erec,OBJECTNUM,level);
	//*************
	delete [] level; delete [] erec;
	int neweio=e3r->file->iocount;
	totalio+=neweio-eoldio;

	while (eres->get_num()>0)
	{
		E3REntry *e=eres->get_first();
		int b=e->son;
		eres->erase();
		if (dup[b]==0)
		{
			dup[b]=1;  //mark this node as visited
			MVRTNode *mvr=mv->NewMVRNode(b);
			if (mvr->level!=0) printf("The fetched block is not a leaf node!\n");
			totalio++;
			for (int i=0;i<mvr->num_entries;i++)
			{
				int son=mvr->entries[i].son;
				if (rec[son]==0)
				{
					float *mbr2=new float[2*E3RTREEDIMENSION];
					mbr2[0]=mvr->entries[i].bounces[0]*E3RSCALE;
					mbr2[1]=mvr->entries[i].bounces[1]*E3RSCALE;
					mbr2[2]=mvr->entries[i].bounces[2]*E3RSCALE;
					mbr2[3]=mvr->entries[i].bounces[3]*E3RSCALE;
					mbr2[4]=mvr->entries[i].sttime;
					mbr2[5]=mvr->entries[i].edtime-1;
					float *overlap;
					overlap=overlapRect(E3RTREEDIMENSION,mbr,mbr2);
					if (overlap!=NULL)
					{
						rec[son]=1;
						count++;
						delete [] overlap;
						overlap=NULL;
					}
					delete [] mbr2;
					mbr2=NULL;
				}
			}
			mv->DelMVRNode(&mvr);
		}
	}

	delete [] dup; delete [] rec;
	return totalio;
}
////////////////////////////////////////////////////////////////////
float EntryCenDistance(MVREntry *e1, MVREntry *e2)
{
	int dim    = e1->dimension;
	float *f1  = new float[dim], *f2 = new float[dim];
	for (int i =0; i < dim; i++)
	{
		f1[i] = (e1->bounces[2*i+1] + e1->bounces[2*i]) / 2;
		f2[i] = (e2->bounces[2*i+1] + e2->bounces[2*i]) / 2;
	}
	float sum = MBRDistance(f1,f2,dim);
	delete [] f1; delete [] f2;
	return sum;
}
////////////////////////////////////////////////////////////////////

bool GetIntvMbr(MVRTNode *rtn, float *mbr, int st, int ed)
{
	int res=false;
	mbr[0]=mbr[2]=MAXREAL;
	mbr[1]=mbr[3]=MINREAL;
	for (int i=0;i<rtn->num_entries;i++)
	{
		int minht,maxlt;
		minht=min(rtn->entries[i].edtime-1,ed);
		maxlt=max(rtn->entries[i].sttime,st);
		//if (maxlt<=minht)
		{
			res=true;
			mbr[0]=min(mbr[0],rtn->entries[i].bounces[0]);
			mbr[1]=max(mbr[1],rtn->entries[i].bounces[1]);
			mbr[2]=min(mbr[2],rtn->entries[i].bounces[2]);
			mbr[3]=max(mbr[3],rtn->entries[i].bounces[3]);
		}
	}
	return res;
}

////////////////////////////////////////////////////////////////////
float MBRDistance(float *f1, float *f2, int dimension)
{
	float sum=0;
	for (int i=0;i<dimension;i++)
		sum+=(f1[i]-f2[i])*(f1[i]-f2[i]);
	return sum;
}
////////////////////////////////////////////////////////////////////
float Overlap3D(MVREntry *e1, MVREntry *e2, int maxed)
{
	int dimension=e1->dimension;
	float *mbr1=new float[2*dimension+2];
	memcpy(mbr1,e1->bounces,2*dimension*sizeof(float));
	mbr1[2*dimension]=e1->sttime;
	mbr1[2*dimension+1]=min(maxed,e1->edtime);
	float *mbr2=new float[2*dimension+2];
	memcpy(mbr2,e2->bounces,2*dimension*sizeof(float));
	mbr2[2*dimension]=e2->sttime;
	mbr2[2*dimension+1]=min(maxed,e2->edtime);
	//float res=overlap(dimension+1,mbr1,mbr2);
	float product=1;
	for (int i=0;i<dimension+1;i++)
	{
		float lt=max(mbr1[2*i],mbr2[2*i]);
		float ht=min(mbr1[2*i+1],mbr2[2*i+1]);
		if (lt>=ht)
		{ product=0; i=dimension+1;}
		float len=ht-lt;
		if (i==dimension)
			len=sqrt(len);
		product*=len;
	}

	delete [] mbr1; delete [] mbr2;
	return product;
}

////////////////////////////////////////////////////////////////////

void printQueryResult(SortedMVRLinList *res,int sign,char *resfname)
//sign=1 => print to screen
//sign=0 => print to file
//sign=2 => no printing
{
  if (sign==2)
	  return;
  if (sign==0)
  {
	   FILE *f;
	   f=fopen(resfname,"w");
	   MVREntry *e;
	   e=res->get_first();
	   while (e!=NULL)
	   {
		   fprintf(f,"%d %f %f %f %f %d %d %d\n",e->son,e->bounces[0],e->bounces[1],e->bounces[2],e->bounces[3],e->sttime,e->edtime,e->level);
		   e=res->get_next();
	   }
	   fclose(f);
	   return;
  }
  else
  {
	  MVREntry *e;
      e=res->get_first();
      while (e!=NULL)
	  {
		   printf("%d %f %f %f %f %d %d %d\n",e->son,e->bounces[0],e->bounces[1],e->bounces[2],e->bounces[3],e->sttime,e->edtime,e->level);
		   e=res->get_next();
	  }
	  return;
  }
}

////////////////////////////////////////////////////////////////////
float SortMbrMargin(int stnum, int ednum, int dim, SortMbr *sm)
{
	if (stnum>ednum)
		error("Hey, stnum is greater than ednum in SortMbrMargin",true);
	float *rmbr=new float [2*dim];
	int i,j;
	float marg;

	for (i = 0; i < 2*dim; i += 2)
	{
		rmbr[i] =    MAXREAL;
		rmbr[i+1] = -MAXREAL;
	}
    for (i = stnum; i <= ednum; i++)
    {
		for (j = 0; j < 2*dim; j += 2)
		{
			rmbr[j] =   min(rmbr[j],   sm[i].mbr[j]);
			rmbr[j+1] = max(rmbr[j+1], sm[i].mbr[j+1]);
		}
    }
	marg = margin(dim, rmbr);
	delete [] rmbr;
	return marg;
}

//////////////////////////////////////////////////////////////////

float SortMbrArea(int stnum, int ednum, int dim, SortMbr *sm)
{
	if (stnum>ednum)
		error("Hey, stnum is greater than ednum in SortMbrMargin",true);
	float *rmbr=new float [2*dim];
	int i,j;
	float marg;

	for (i = 0; i < 2*dim; i += 2)
	{
		rmbr[i] =    MAXREAL;
		rmbr[i+1] = -MAXREAL;
	}
    for (i = stnum; i <= ednum; i++)
    {
		for (j = 0; j < 2*dim; j += 2)
		{
			rmbr[j] =   min(rmbr[j],   sm[i].mbr[j]);
			rmbr[j+1] = max(rmbr[j+1], sm[i].mbr[j+1]);
		}
    }
	marg = area(dim, rmbr);
	delete [] rmbr;
	return marg;
}

//////////////////////////////////////////////////////////////////

float SortMbrOverlap(int stnum, int midnum, int ednum, int dim, SortMbr *sm)
{
	float *rxmbr=new float [2*dim];
	float *rymbr=new float [2*dim];
	float over;
	int i,j;
	for (i = 0; i < 2*dim; i += 2)
	{
	    rxmbr[i]=rymbr[i]=MAXREAL;
	    rxmbr[i+1]=rymbr[i+1]=-MAXREAL;
	}
	for (i=stnum;i<midnum;i++)
	{
	    for (j = 0; j < 2*dim; j += 2)
	    {
			rxmbr[j] =   min(rxmbr[j],   sm[i].mbr[j]);
			rxmbr[j+1] = max(rxmbr[j+1], sm[i].mbr[j+1]);
	    }
	}
	for (i=midnum;i<=ednum;i++)
	{
	    for (j=0; j<2*dim; j+=2)
	    {
			rymbr[j] =   min(rymbr[j],   sm[i].mbr[j]);
			rymbr[j+1] = max(rymbr[j+1], sm[i].mbr[j+1]);
	    }
	}
	over = overlap(dim, rxmbr, rymbr);
	delete [] rxmbr;
	delete [] rymbr;
	return over;
}

//////////////////////////////////////////////////////////////////
void testRangeQuery(char *fname, char *rtfname, char *resfname, int dm)
{
   MVRTree *rt=new MVRTree(fname,rtfname,NULL);

   float mbr[7];
   mbr[0]=0.6;
   mbr[1]=1.0;
   mbr[2]=0.6;
   mbr[3]=1.0;
   mbr[4]=10;
   mbr[5]=10;

   char *rec;
   rec=new char[NOWTIME];
   for (int i=0;i<NOWTIME;i++)
	   rec[i]=0;
   SortedMVRLinList *res;
   res=new SortedMVRLinList();
   char *dupblock=new char[MAXBLOCK];
   char *dup=new char[MAXBLOCK];
   rt->rangeQuery(mbr,res,rec,dup);
   delete [] dup;
   delete [] dupblock;
   printf("Retrieved %d answers\n",res->get_num());

   printQueryResult(res,dm,resfname);
   
   delete [] rec;
   delete res;
   delete rt;
}

////////////////////////////////////////////////////////////////////

void testE3RRangeQuery(char *fname, char *rtfname, char *efname, char *resfname, int wfile)
{
   MVRTree *mv=new MVRTree(fname,rtfname,NULL);
   E3RTree *e3r=new E3RTree(efname,NULL);
   int i;

   float mbr[7];
   mbr[0]=0.6*E3RSCALE;
   mbr[1]=1.0*E3RSCALE;
   mbr[2]=0.6*E3RSCALE;
   mbr[3]=1.0*E3RSCALE;
   mbr[4]=10;
   mbr[5]=10;

   char *rec;
   rec=new char[NOWTIME];
   for (i=0;i<NOWTIME;i++)
	   rec[i]=0;
   SortedE3RLinList *res;
   res=new SortedE3RLinList();
   SortedMVRLinList *mvrres;
   mvrres=new SortedMVRLinList();

   int *levelcount=new int [256];
   e3r->rangeQuery(mbr,res,rec,30000, levelcount);
	//delete the root to disable the cache effect for the next query
   delete e3r->root_ptr;
   e3r->root_ptr=NULL;
	//Since levelcount serves simply as a place-taker, so we delete it now.
   delete [] levelcount;
	//Now we are to check each entry of the list and will go into the hr tree to
	//get the exact answers
   delete [] rec;	
   E3REntry *en;

   en=res->get_first();
   char *dup=new char[30000];
   for (i=0;i<30000;i++)
		dup[i]=0;
   while (en!=NULL)
   {
		if (en->son>=0)
		{
			int bno=abs(en->son);
			//Now we read in the node of HR-tree
			//MVRTNode *rn=new MVRTNode(mv,bno);
			MVRTNode *rn=mv->NewMVRNode(bno);
			//We accumulate the io
			if (rn->level!=0)
				printf("E3RTimeQuery--Something wrong\n");
			for (int i=0;i<rn->num_entries;i++)
			{
				if (rn->entries[i].sttime<=rn->entries[i].edtime && rn->entries[i].son>=0)
				{
					//Now for each of the entries, we check if the entry overlaps with
					//the query rectangle.
					float *mbr2=new float[2*E3RTREEDIMENSION];
					mbr2[0]=rn->entries[i].bounces[0]*E3RSCALE;
					mbr2[1]=rn->entries[i].bounces[1]*E3RSCALE;
					mbr2[2]=rn->entries[i].bounces[2]*E3RSCALE;
					mbr2[3]=rn->entries[i].bounces[3]*E3RSCALE;
					//The entry's time span is the node's time span
					mbr2[4]=rn->entries[i].sttime;
					mbr2[5]=rn->entries[i].edtime;
								
					float *overlap;
					overlap=overlapRect(E3RTREEDIMENSION,mbr,mbr2);

					if (overlap!=NULL && dup[abs(rn->entries[i].son)]==0)
						//intersect, and we find an answer
					{
						dup[abs(rn->entries[i].son)]=1;
						delete [] overlap;
						overlap=NULL;
						MVREntry *men=new MVREntry(MVRDIM, NULL);
					    (*men)=rn->entries[i];
						//men->son=abs(men->son);
					    mvrres->insert(men);
					}
					delete [] mbr2;
					mbr2=NULL;
				}
			}
			delete rn;
			rn=NULL;
		}
		else
		{
			if (dup[abs(en->son)]==0)
			{
				dup[abs(en->son)]=1;
				MVREntry *men=new MVREntry(MVRDIM, NULL);
				men->son=en->son;
				memcpy(men->bounces,en->bounces,4*sizeof(float));
				men->sttime=en->bounces[4];
				men->edtime=en->bounces[5];
				men->level=0;
				//men->son=abs(men->son);
				mvrres->insert(men);
			}
		}

		en=res->get_next();
	}
   printf("Retrieved %d answers\n",mvrres->get_num());

   printQueryResult(mvrres,wfile,resfname);
   
   delete res;
   delete mvrres;
   delete [] dup;
   delete mv;
   delete e3r;
}

////////////////////////////////////////////////////////////////////

void testFindleaf()
{
	MVRTree *rt=new MVRTree(".\\trees\\5k2.mvr",".\\trees\\5k2.rta",NULL);
	MVREntry *e=new MVREntry(MVRDIM, rt);
	e->bounces[0]=0.233030;
	e->bounces[1]=0.234880;
	e->bounces[2]=0.256170;
	e->bounces[3]=0.309950;
	e->sttime=25;
	e->edtime=30;
	e->son=1318;
    rt->FindLeaf(e);
	delete rt;
}

////////////////////////////////////////////////////////////////////
void TightenNode(MVRTNode *rn)
{
	if (rn->level==0)
		return;
	else
	{
		for (int i=0;i<rn->num_entries;i++)
		{
			MVRTNode *succ=rn->entries[i].get_son();
			TightenNode(succ);
			float *mbr=new float [2*rn->dimension];
			succ->GetIntvMbr(mbr,rn->entries[i].sttime,rn->entries[i].edtime-1);
			rn->dirty=true;
			memcpy(rn->entries[i].bounces,mbr,2*rn->dimension*sizeof(float));
		}
	}
}
////////////////////////////////////////////////////////////////////
void TightenTree(MVRTree *rt)
{
   int count=rt->rtable.rtnum;
   for (int i=0;i<count;i++)
   {
	   printf("Tightening the %dth tree!\n",i);
	   rt->load_root(rt->rtable.rno[i]);
	   TightenNode(rt->root_ptr);
   }
}
////////////////////////////////////////////////////////////////////
void TraverseNode(MVRTNode *rn, char *rec)
{
	int b;
	b=rn->block;
	if (rec[b]==1) return;
	rec[b]=1;
	MVRTNode *succ;
	if (rn->level>0)
	{
		CheckMVRNode(rn);
		for (int i=0;i<rn->num_entries;i++)
		{
			int b=rn->entries[i].son;
			if (rec[b]==0)
			{
				succ=rn->entries[i].get_son();
				TraverseNode(succ,rec);
			}
		}
	}
	else
	{
		CheckMVRNode(rn);
		for (int i=0;i<rn->num_entries;i++)
		{
		}
		return;
	}
}

////////////////////////////////////////////////////////////////////

void TraverseTree(MVRTree *rt, int tno, char *rec)
//tfname: the file name of the tree
//rtname: the root table file name
//tno: the number of the tree to be traversed in the root table
{
	rt->load_root(rt->rtable.rno[tno]);
	TraverseNode(rt->root_ptr,rec);
}

////////////////////////////////////////////////////////////////////

int TraverseE3RNode(E3RTNode *rn)
{
	E3RTNode *succ;

	if (rn->level>0)
	{
		for (int i=0;i<rn->num_entries;i++)
		{
			int bno;
			succ=rn->entries[i].get_son();
			if ((bno=TraverseE3RNode(succ))!=-1)
			{
				printf("%d   pointed by the %dth at the previous level\n",bno,i);
				return rn->block;
			}
		}
		return -1;
	}
	else
	{
		for (int i=0;i<rn->num_entries;i++)
			if (rn->entries[i].son==790)
			{
				return rn->block;
			}
		return -1;
	}
}

////////////////////////////////////////////////////////////////////

void TraverseE3RTree(char *tfname)
//tfname: the file name of the tree
//rtname: the root table file name
//tno: the number of the tree to be traversed in the root table
{
	E3RTree *rt=new E3RTree(tfname,NULL);
    printf("%d\n",TraverseE3RNode(rt->root_ptr));
	printf("Total IO cost in the traversal: %d\n",rt->file->get_iocount());
	delete rt;
}

////////////////////////////////////////////////////////////////////
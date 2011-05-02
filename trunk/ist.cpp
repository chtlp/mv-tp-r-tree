#include "ist.h"
//////////////////////////////////////////////////////////////////
IndexSt::IndexSt(int num)
{
	maxnum=num;
	items=new IstItem[maxnum];
	for (int i=0;i<maxnum;i++)
		items[i].index=-1;
	effnum=0;
}
//////////////////////////////////////////////////////////////////
IndexSt::~IndexSt()
{
	delete [] items;
	items=NULL;
}
//////////////////////////////////////////////////////////////////
void IndexSt::Abandon(float detratio)
{
	if (effnum<=1) return;
	for (int i=1;i<effnum;i++)
	{
		if (items[i].v1>(1+detratio)*items[0].v1)
		{
			del(i); 
			i--;
		}
	}
}
//////////////////////////////////////////////////////////////////
void IndexSt::Add(float v1, float v2, float v3, int index)
{
	int i=0;
	while (i<effnum)
	{
		if (items[i].v1>v1 ||
			(fabs(items[i].v1-v1)<FLOATZERO && items[i].v2>v2) ||
			(fabs(items[i].v1-v1)<FLOATZERO && fabs(items[i].v2-v2)<FLOATZERO && items[i].v3>=v3))
		{
			insert(v1,v2,v3,index,i);
			return;
		}
		i++;
	}
	insert(v1,v2,v3,index,effnum);
	return;
}
//////////////////////////////////////////////////////////////////
int IndexSt::Check(float v1)
{
	for (int i=0;i<effnum;i++)
	{
		if (fabs(items[i].v1-v1)<FLOATZERO)
			return i;
		if (items[i].v1>v1)
			return -1;
	}
	return -1;
}
//////////////////////////////////////////////////////////////////
void IndexSt::del(int pos)
{
	if (effnum==0)
		error("Hey, no entry to delete.  What do you want to do?\n",true);
	for (int i=pos;i<effnum-1;i++)
	{
		items[i].v1=items[i+1].v1;
		items[i].v2=items[i+1].v2;
		items[i].v3=items[i+1].v3;
		items[i].index=items[i+1].index;
	}
	items[effnum].index=-1;
	effnum--;
}
//////////////////////////////////////////////////////////////////
void IndexSt::error(char *msg, bool ex)
{
	printf(msg);
	if (ex) exit(1);
	return;
}
//////////////////////////////////////////////////////////////////
void IndexSt::GetItem(float *fv, int &ind, int pos)
{
	if (pos<0 || pos>=effnum) 
		error("Illegal attempt to retrive index item!\n",true);	
	fv[0]=items[pos].v1; fv[1]=items[pos].v2; fv[2]=items[pos].v3;
	ind=items[pos].index;
}
//////////////////////////////////////////////////////////////////
int IndexSt::GetNum()
{
	return effnum;
}
//////////////////////////////////////////////////////////////////
void IndexSt::insert(float v1, float v2, float v3, int index, int pos)
{
	if (effnum==maxnum)
		error("The maximum number of entries has been reached!\n",true);
	if (pos>effnum)
		error("You should insert the entries compactly.\n",true);
	for (int i=effnum;i>=pos+1;i--)
	{
		items[i].v1=items[i-1].v1;
		items[i].v2=items[i-1].v2;
		items[i].v3=items[i-1].v3;
		items[i].index=items[i-1].index;
	}
	items[pos].v1=v1; items[pos].v2=v2; items[pos].v3=v3; items[pos].index=index;
	effnum++;
}
//////////////////////////////////////////////////////////////////
void IndexSt::Print()
{
	for (int i=0;i<effnum;i++)
	{
		printf("Value1: %f, Value2: %f, Value3: %f, Index: %d\n",
			items[i].v1, items[i].v2, items[i].v3, items[i].index);
	}
}


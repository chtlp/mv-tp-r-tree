#ifndef __IST_H
#define __IST_H
//////////////////////////////////////////////////////////////////
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
//////////////////////////////////////////////////////////////////
//This file contains the class IndexSort.  This class sort the index
//entries by two values (first by the major value and then the second
//value.  Furthermore, you can use this class to prune off some entries
//that are worse than the best one to a certain extent
//////////////////////////////////////////////////////////////////
#ifndef FLOATZERO
#define FLOATZERO 1e-10
#endif
//////////////////////////////////////////////////////////////////
struct IstItem
{
	float v1;  //This is the major value
	float v2;  //This is the second vaue
	float v3;
	int index;
};
//////////////////////////////////////////////////////////////////
class IndexSt
{
public:
	int maxnum;  //the capacity of the items
	int effnum;  //the number of items in use
	IstItem* items;  //the array storing all the items
	
public:
	void del(int pos);
	void error(char *msg, bool ex);
	void insert(float v1, float v2, float v3, int index, int pos);
public:
	IndexSt(int num);
	~IndexSt();
	void Abandon(float detratio);
	void Add(float v1, float v2, float v3, int index);
	int Check(float v1);
	void GetItem(float *fv, int &ind, int pos);
	int GetNum();
	void Print();
};
//////////////////////////////////////////////////////////////////
#endif
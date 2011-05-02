///////////////////////////////////////////////////////////////////
//This file contains the necessary functions for using MVR-Tree
//Coded by TAO Yufei.
//Sep 2000
///////////////////////////////////////////////////////////////////
#ifndef __ST_H
#define __ST_H
///////////////////////////////////////////////////////////////////
#include "rtreeheader.h"
#include "mvrtree.h"
#include "e3rtree.h"
#include "st2.h"
///////////////////////////////////////////////////////////////////
struct SetData
{
	float extent[4];
	int timestamp;
};
///////////////////////////////////////////////////////////////////
//Function prototypes
void BuildMVRTree(char *fname, char *rtname, char *txtname, int csize, int blen);
void BuildE3RMVRTree(char *fname,char *rtname, char *efname, int csize,int blen);
void Check(MVRTree *rt, SetData *sd);
void CheckMVRTree(MVRTree *rt);
void CheckTree(MVRTree *rt);
int CombSingleQ(E3RTree *e3r, MVRTree *mv, float *mbr, int *time, int &count);
int CombTimeQuery(E3RTree *e3r, MVRTree *mv, char *qfile, int *time, int &count);
int E3RTimeQuery(E3RTree *e3r, MVRTree *mv, char *qfile, int &count);
void GetMVRRecord(FILE *file, float *fdata, int *idata, int fnum, int dnum);
void inserttoe3r(MVRTNode *mv,E3RTree *rt,char *rec);
int MVRTimeQuery(MVRTree *mvr_tree, char *query_fname, int &count);
void MVREntryDel(MVRTree *rt, float *mbr, int time, int oid);
void MVREntryInsert(MVRTree *rt, float *mbr, int time, int oid,int count);
///////////////////////////////////////////////////////////////////
#endif
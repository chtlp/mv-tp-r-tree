#ifndef __ST2_H
#define __ST2_H
////////////////////////////////////////////////////////////////////
#include "st.h"
////////////////////////////////////////////////////////////////////
class SortedMVRLinList;
class E3RTNode;
////////////////////////////////////////////////////////////////////
float Area3D(MVREntry *e, int maxed);
void CheckMVRNode(MVRTNode *rn);
float EntryCenDistance(MVREntry *e1, MVREntry *e2);
bool GetIntvMbr(MVRTNode *rn, float *mbr, int st, int ed);
float MBRDistance(float *f1, float *f2, int dimension);
float Overlap3D(MVREntry *e1, MVREntry *e2, int maxed);
void printQueryResult(SortedMVRLinList *res,int sign,char *resfname);
float SortMbrMargin(int stnum, int ednum, int dim, SortMbr *sm);
float SortMbrArea(int stnum, int ednum, int dim, SortMbr *sm);
float SortMbrOverlap(int stnum, int midnum, int ednum, int dim, SortMbr *sm);
void testRangeQuery(char *fname, char *rtfname, char *resfname, int dm);
void testE3RRangeQuery(char *fname, char *rtfname, char *efname, char *resfname, int wfile);
void testFindleaf();
void TightenNode(MVRTNode *rn);
void TightenTree(MVRTree *rt);
void TraverseNode(MVRTNode *rn, char *rec);
void TraverseTree(MVRTree *rt, int tno, char *rec);
int TraverseE3RNode(E3RTNode *rn);
void TraverseE3RTree(char *tfname);

////////////////////////////////////////////////////////////////////
#endif
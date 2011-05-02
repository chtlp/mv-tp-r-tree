////////////////////////////////////////////////////////////////////
//This is the implementation of E3R-tree + MVR-tree V1.2
//Adapted by TAO Yufei
//Last Modified 3 Nov, 2000
////////////////////////////////////////////////////////////////////
//These are the tuning parameters
//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "mvrtree.h"
#include "e3rtree.h"
#include "st.h"
#include "st2.h"
#include "ist.h"
////////////////////////////////////////////////////////////////////
float Area3D(MVREntry *e);
float Overlap3D(MVREntry *e1, MVREntry *e2, int maxed);
void test1();
void test3();
void test4();
void test5();
////////////////////////////////////////////////////////////////////
void test1()
{
	MVRTree *mvr_tree = new MVRTree("D:\\Linpeng-Tang\\database-project\\experiment\\tree1.mvr", "D:\\Linpeng-Tang\\database-project\\experiment\\root1.RTA",NULL);
	//E3RTree *e3r_tree = new E3RTree(".\\trees\\m5k13.e3r", NULL);

	int retrieve_count;

	MVRTimeQuery(mvr_tree, "D:\\Linpeng-Tang\\database-project\\experiment\\query1.txt", retrieve_count);
	//MVRTimeQuery(mvr_tree, ".\\query_set\\interval.txt", retrieve_count);
	//E3RTimeQuery(e3r_tree, mvr_tree, ".\\query_set\\halftimeinterval.txt", retrieve_count);

	delete mvr_tree;
	//delete e3r_tree;
}

////////////////////////////////////////////////////////////////////
void test4()
{
	BuildMVRTree("D:\\Linpeng-Tang\\database-project\\experiment\\tree1.mvr",
		"D:\\Linpeng-Tang\\database-project\\experiment\\root1.RTA",
		"D:\\Linpeng-Tang\\database-project\\experiment\\data1.txt",
		0, 1024);
	//BuildMVRTree("J:\\Experiments\\MVR-tree\\mvr-trees\\RAN_3.MVR",
	//	"J:\\Experiments\\MVR-tree\\mvr-trees\\RAN_3.RTA",
	//	"J:\\Experiments\\MVR-tree\\Data_Set\\RAN50K3.TXT", 0, 1024);

	//BuildE3RMVRTree(".\\trees\\test.mvr", ".\\trees\\test.rta", 
	//	".\\trees\\test.e3r", 0, 1024);
}
////////////////////////////////////////////////////////////////////
void test5()
{
	BuildMVRTree(".\\trees\\str_over.mvr", ".\\trees\\str_over.rta", "..\\..\\dataset\\random\\dr5k.txt",
		0, 1024);
	BuildE3RMVRTree(".\\trees\\str_over.mvr", ".\\trees\\str_over.rta", ".\\trees\\str_over.e3r",
		0, 1024);

}
////////////////////////////////////////////////////////////////////
void test6()
{
	BuildMVRTree(".\\test\\test.mvr", ".\\test\\test.rta", "..\\..\\dataset\\random\\dr5k.txt", 0 ,1024);
}
////////////////////////////////////////////////////////////////////
void test8()
{
	MVRTree *rt=new MVRTree(".\\trees\\als+so+ms\\test.mvr",".\\trees\\als+so+ms\\test.rta",NULL);
	TightenTree(rt);
	delete rt;
}
////////////////////////////////////////////////////////////////////
void main(int argc, char* argv[])
{
	test1();
	return;   
}

#include "st.h"
#include "para.h"
#include "..\\common\\readrecord.h"
///////////////////////////////////////////////////////////////////
void Check(MVRTree *rt, SetData *sd)
{
	///*
	printf("looking for the entry...");
	MVREntry *e=new MVREntry(MVRDIM,NULL);
	e->sttime=23;
	e->edtime=NOWTIME;
	e->bounces[0]=0.171910 ;
	e->bounces[1]=0.185650 ;
	e->bounces[2]=0.363000 ;
	e->bounces[3]=0.376490 ;
	e->son=4615;
	e->son_ptr=NULL;
	int co=rt->rtable.rtnum;
	rt->load_root(rt->rtable.rno[co-1]);
	if (rt->FindLeaf(e)==false)
		error("Can't find the entry\n",true);
	printf("found\n");
	delete e;
}
///////////////////////////////////////////////////////////////////
void CheckMVRTree(MVRTree *rt)
{
	char *rec=new char[20000];
	int i;
	for (i=0;i<20000;i++)
		rec[i]=0;
	printf("Checking integrity of the tree...");
	int count=rt->rtable.rtnum;
	for (i=0;i<count;i++)
	{
		TraverseTree(rt,i,rec);
	}
	printf("Integrity check passed\n");
	delete [] rec;
}

/************************************
BuildMVRTree: This function will creates an MVR-tree from the specified data set
Para:
tree_fname  : The tree file name
root_table_fname : The file name of the root table
data_set_fname: The data set file
cache_size  : The cache size (currently disabled)
block_len   : the block length
Precondition:
No files with the same names as the ones to be created should exist.
But the data set file should exist.
Postcondition
Tree file and the root table data set file will be created
*************************************/

void BuildMVRTree(char *tree_fname, char *root_table_fname, char *data_set_fname, 
	int cache_size, int block_len)
{
	int i,j;

	MVRTree *mvr_tree;
	FILE *data_set_file;

	data_set_file = fopen(tree_fname, "r");  //Here we use the data_set_file to detect if
	//the tree file already exists
	if (data_set_file != NULL)
	{
		fclose(data_set_file);
		error("The tree file already exists\n",true);
		return;
	}

	data_set_file = fopen(data_set_fname, "r");
	if (data_set_file == NULL)
	{
		error("The dataset txt file does not exist\n",true);
		return;
	}

	mvr_tree = new MVRTree(tree_fname, root_table_fname, block_len, 
		NULL, MVRDIM);

	//we use sd to store the previous version of each entry
	SetData *previous_ver_obj = new SetData[OBJECTNUM];

	for (i = 0; i < OBJECTNUM; i++)
	{
		for (j = 0; j < 4; j++)
			previous_ver_obj[i].extent[j]=0;

		previous_ver_obj[i].timestamp=-1;
	}

	int record_count=0;

	float *f_data = new float[5];  //Used to store the float data read
	int *i_data   = new int[2];    //Used to store the int data read
	int f_num, i_num;              //Number of data read for each type

	int oid;                       //Object ID read
	int time;                      //Timestamp read 

	while (read_GSTD_record(data_set_file, f_data, f_num, i_data, i_num))
	{
		printf("MVRtree's current rtable num:%d\n", mvr_tree -> rtable.rtnum);

		record_count++;

		printf("Count = %d\n", record_count);

		time = int ((f_data[0] + 0.0000001) * SCALETIME);
		oid  = i_data[0];

		if (time > mvr_tree->most_recent_time)
		{
			mvr_tree -> FlushEtable();
			mvr_tree -> most_recent_time = time;
		}

		if (previous_ver_obj[oid].timestamp != -1)
			MVREntryDel(mvr_tree, previous_ver_obj[oid].extent,
			mvr_tree -> most_recent_time, oid);

		MVREntryInsert(mvr_tree, f_data+1, mvr_tree->most_recent_time,
			oid, record_count);

		previous_ver_obj[oid].extent[0] = f_data[1];
		previous_ver_obj[oid].extent[1] = f_data[3];
		previous_ver_obj[oid].extent[2] = f_data[2];	
		previous_ver_obj[oid].extent[3] = f_data[4];
		previous_ver_obj[oid].timestamp = mvr_tree -> most_recent_time;

		if (record_count / 10000 * 10000 == record_count)
			mvr_tree -> DelMVRNode(& mvr_tree -> root_ptr);    //This is to prevent the memory from being exhausted
	}

	mvr_tree -> FlushEtable();

	delete mvr_tree;

	mvr_tree = new MVRTree(tree_fname,root_table_fname,NULL);

	CheckMVRTree(mvr_tree);

	fclose(data_set_file);

	delete [] f_data;
	delete [] i_data;
	delete [] previous_ver_obj;
}

///////////////////////////////////////////////////////////////////

void BuildE3RMVRTree(char *fname,char *rtname,char *efname,int csize,int blen)
{
	//First we check if the e3r tree already exists
	FILE *f;
	f=fopen(efname,"r");
	if (f!=NULL)
	{
		fclose(f);
		printf("The tree already exisits\n");
		return;
	}
	MVRTree *mv=new MVRTree(fname,rtname,NULL);
	E3RTree *rt=new E3RTree(efname,blen,NULL,3);
	//we don't implement cache in this version
	char *rec=new char [30000];
	int i;
	for (i=0;i<30000;i++)
		rec[i]=0;
	for (i=0;i<mv->rtable.rtnum;i++)
	{
		mv->load_root(mv->rtable.rno[i]);
		printf("Building from the %dth tree\n",i);
		inserttoe3r(mv->root_ptr,rt,rec);
	}
	delete mv;
	delete rt;
	delete [] rec;
}

///////////////////////////////////////////////////////////////////
int CombTimeQuery(E3RTree *e3r, MVRTree *mv, char *qfile, int *intv, int &count)
{
	FILE *f;
	f=fopen(qfile,"r");
	if (f==NULL)
		error("Cann't open txt file.\n",true);
	int oid,tmp;
	float rtime,x0,y0,x1,y1;
	float *mbr=new float[6];
	int totalio=0;
	int qcount=count=0;
	while (!feof(f))
	{
		fscanf(f,"%d %f %f %f %f %f %d\n",&oid,&rtime,&x0,&y0,&x1,&y1,&tmp);
		mbr[0]=x0;mbr[1]=x1;mbr[2]=y0;mbr[3]=y1;
		totalio+=CombSingleQ(e3r,mv,mbr,intv,qcount);
		count+=qcount; 
	}
	printf("%d entries fetched in %d IOs\n",count,totalio);
	return totalio;
}

/**************************************
E3RTimeQuery: This function will perform as a set of queries obtained from the 
query file specified using the 3D R-tree built underneath the MVR-tree.
Papa:
e3r-tree: The e3r-tree to be queried on.
mvr_tree: the mvr-tree on which the 3d R-tree is built underneath
count: the number of objects retrieved

return value: The number of I/O's attempted.
***************************************/
int E3RTimeQuery(E3RTree *e3r_tree, MVRTree *mvr_tree, char *query_fname, int &count)
	//This function conducts the query from the E3R-tree.  The number of IO will be returned.
	//The number of answers is returned as count
{
	FILE *query_file = fopen(query_fname, "r");

	if (query_file == NULL)
		error("Cann't open txt file.\n", true);

	float *f_data = new float [4];
	int   *i_data = new int   [4];
	int f_num, i_num;

	int total_io=0;
	int query_count = 0;
	int retrieve_count = 0;

	while (read_QueryGen_record(query_file, f_data, f_num, i_data, i_num))
	{
		query_count ++;

		int just_retrieve_count = 0;

		float *query_mbr = new float [6];
		query_mbr[0] = f_data [0];
		query_mbr[1] = f_data [1];
		query_mbr[2] = f_data [2];
		query_mbr[3] = f_data [3];
		query_mbr[4] = i_data [0];
		query_mbr[5] = i_data [1];

		for (int i = 0; i < 4; i++)
			query_mbr[i] *= E3RSCALE;

		int e3r_iocount_before_query = e3r_tree -> file -> get_iocount();

		SortedE3RLinList *e3r_result_list = new SortedE3RLinList();
		int *e3r_level_count = new int[256];
		char *e3r_retrieved_record = new char[OBJECTNUM];

		e3r_tree -> rangeQuery(query_mbr, e3r_result_list , e3r_retrieved_record, 
			OBJECTNUM, e3r_level_count);

		delete [] e3r_retrieved_record;
		delete [] e3r_level_count;

		int e3r_iocount_after_query = e3r_tree -> file -> get_iocount();
		int this_query_io = e3r_iocount_after_query - e3r_iocount_before_query;

		char *retrieved_record = new char[MAXBLOCK];
		for (int i = 0; i < MAXBLOCK; i++)
			retrieved_record[i] = 0;

		while (e3r_result_list -> get_num() > 0)
		{
			E3REntry *en = e3r_result_list -> get_first();

			int block_no = en -> son;

			e3r_result_list -> erase();

			MVRTNode *mvr_leaf = mvr_tree -> NewMVRNode(block_no);

			this_query_io++;

			if (mvr_leaf -> level != 0)
				error("E3RTimeQuery--Something wrong\n", true);

			for (int i = 0; i < mvr_leaf -> num_entries; i++)
			{
				if (retrieved_record[mvr_leaf -> entries[i].son]==0)
				{
					float *entry_mbr = new float[2*E3RTREEDIMENSION];
					entry_mbr[0] = mvr_leaf -> entries[i].bounces[0] * E3RSCALE;
					entry_mbr[1] = mvr_leaf -> entries[i].bounces[1] * E3RSCALE;
					entry_mbr[2] = mvr_leaf -> entries[i].bounces[2] * E3RSCALE;
					entry_mbr[3] = mvr_leaf -> entries[i].bounces[3] * E3RSCALE;
					entry_mbr[4] = mvr_leaf -> entries[i].sttime;
					entry_mbr[5] = mvr_leaf -> entries[i].edtime - 1;

					float *overlap;
					overlap = overlapRect(3, query_mbr, entry_mbr);
					if (overlap!=NULL)
					{
						retrieved_record[mvr_leaf -> entries[i].son] = 1;
						just_retrieve_count++;

						delete [] overlap;
						overlap = NULL;
					}
					delete [] entry_mbr;
					entry_mbr = NULL;
				}
			}
			mvr_tree -> DelMVRNode(&mvr_leaf);
		}
		delete [] retrieved_record;
		delete e3r_result_list;

		retrieve_count += just_retrieve_count;
		total_io       += this_query_io;
		printf("Query %d: %d records retrieved in %d I/O's\n", query_count, just_retrieve_count, 
			this_query_io);

		delete [] query_mbr;
	}

	printf("Total %d records retrieved in %d IOs\n", retrieve_count, total_io);

	fclose(query_file);
	delete [] f_data;
	delete [] i_data;

	count = retrieve_count;
	return total_io;
}

///////////////////////////////////////////////////////////////////

void GetMVRRecord(FILE *f, float *fdata, int *idata, int fnum, int dnum)
{
	fnum=dnum=0;
	fscanf(f,"%d ",idata+dnum);
	dnum++;
	for (int i=0;i<5;i++)
	{
		fscanf(f,"%f ",fdata+fnum);
		fnum++;
	}
	fscanf(f,"%d\n",idata+dnum);
}

/**************************
inserttoe3r:  This function inserts an MVR-tree leaf node into the e3r_tree.
If this is not a leaf node, then this function will try to insert the leaves
on this subtree.  Note that a leaf node will only be inserted once.
Para:
mvr_node: The nvr-tree node being considered
e3r_tree: The e3r-tree to which the node will be inserted into
block_read: This array is used to avoid inserting the same leaf node twice
***************************/

void inserttoe3r(MVRTNode *mvr_node, E3RTree *e3r_tree, char *block_read)
{
	if (mvr_node -> level == 0)
	{
		if (block_read[mvr_node -> block] == 0)
		{
			block_read[mvr_node -> block] = 1;

			E3REntry *en;
			en = new E3REntry(E3RTREEDIMENSION, e3r_tree);

			float *mvr_node_mbr = mvr_node -> get_mbr(0);

			en -> bounces[0] = mvr_node_mbr[0] * E3RSCALE;
			en -> bounces[1] = mvr_node_mbr[1] * E3RSCALE;
			en -> bounces[2] = mvr_node_mbr[2] * E3RSCALE;
			en -> bounces[3] = mvr_node_mbr[3] * E3RSCALE;

			delete [] mvr_node_mbr;

			int *mvr_node_timespan = mvr_node -> get_timespan(0);

			en -> bounces[4] = mvr_node_timespan[0];

			if (mvr_node_timespan[1] == NOWTIME)  //this is "now" data
				en -> bounces[5] = FINALVER;
			else
				en -> bounces[5] = mvr_node_timespan[1] - 1;

			delete [] mvr_node_timespan;

			en -> son   = mvr_node -> block;
			en -> level = 0;

			if (en -> bounces[4] <= en -> bounces[5])
				e3r_tree -> insert(en);

			delete en;
		}
	}
	else
	{
		for (int i = 0; i < mvr_node -> num_entries; i++)
		{
			MVRTNode *succ = mvr_node -> entries[i].get_son();
			inserttoe3r(succ, e3r_tree, block_read);
		}
		return;
	}
}

/********************************
MVRTimeQuery: This funciton to conduct a series of queries on the give MVR-tree
Para:
mvr_tree: The mvrtree to be queried on
query_fname: The query file name
count: The number of results retrieved

return value: The number of I/O's
Precondition: 
The mvr-tree should have been loaded and the query file exists
Postcondition:
None.
*********************************/

int MVRTimeQuery(MVRTree *mvr_tree, char *query_fname, int &count)
{

	FILE *query_file;
	query_file = fopen(query_fname, "r");
	if (query_file == NULL)
		error("The query file does not exist\n", true);

	float *f_data = new float [4];
	int   *i_data = new int   [4];
	int    f_num, i_num;

	int retrieve_count = 0;
	int query_count    = 0;

	int old_io_count = mvr_tree -> file -> get_iocount();

	while (read_QueryGen_record(query_file, f_data, f_num, i_data, i_num))
	{
		SortedMVRLinList *result_list = new SortedMVRLinList();

		char *retrieved_record = new char[OBJECTNUM];
		for (int i = 0; i < OBJECTNUM; i++)
			retrieved_record [i] = 0;

		float *query_mbr = new float [7];
		query_mbr [0] = f_data [0];
		query_mbr [1] = f_data [1];
		query_mbr [2] = f_data [2];
		query_mbr [3] = f_data [3];
		query_mbr [4] = i_data [0];
		query_mbr [5] = i_data [1];

		char *dup_block = new char [MAXBLOCK];
		for (int i = 0; i < OBJECTNUM; i++)
			retrieved_record [i] = 0;

		int io_count_before_query = mvr_tree -> file -> iocount;

		mvr_tree -> rangeQuery(query_mbr, result_list, retrieved_record, dup_block);

		int io_count_after_query = mvr_tree -> file -> iocount;

		int just_retrieved_count = result_list -> get_num();
		retrieve_count += just_retrieved_count;

		delete [] dup_block;
		delete [] query_mbr;
		delete [] retrieved_record;
		delete result_list;

		query_count   ++;

		printf("Query %d: %d records retrieved in %d I/O's\n", query_count, just_retrieved_count, 
			io_count_after_query - io_count_before_query);

	}

	int new_io_count = mvr_tree -> file -> get_iocount();

	printf("Total %d records retrieved in %d I/O's\n", retrieve_count, new_io_count - old_io_count);

	fclose(query_file);
	delete [] f_data;
	delete [] i_data;

	return new_io_count - old_io_count;
}

///////////////////////////////////////////////////////////////////

void MVREntryDel(MVRTree *rt, float *mbr, int time, int oid)
{
	//printf("Deleting record oid %d\n",oid);
	//printf("\t%f %f %f %f\n",mbr[0],mbr[1],mbr[2],mbr[3]);
	MVREntry *e;
	e=new MVREntry(MVRDIM,rt);
	e->level=0;
	e->son=oid;
	//We first need to delete the precious version
	e->bounces[0]=mbr[0];
	e->bounces[1]=mbr[1];
	e->bounces[2]=mbr[2];
	e->bounces[3]=mbr[3];
	e->edtime=time;
	rt->delete_entry(e);
	delete e;
}

///////////////////////////////////////////////////////////////////

void MVREntryInsert(MVRTree *rt,float *mbr, int time, int oid, int count)
{
	printf("Creating the %dth record...",count);
	printf("oid  %d  ", oid);
	printf("time %d\n",time);
	MVREntry *e;
	e=new MVREntry(MVRDIM,rt);
	e->level=0;
	e->son=oid;
	e->bounces[0]=mbr[0];
	e->bounces[1]=mbr[2];
	e->bounces[2]=mbr[1];
	e->bounces[3]=mbr[3];
	e->sttime=time;
	e->edtime=NOWTIME;
	rt->insert(e,false);
}
#ifndef __BLOCKMEM_H
#define __BLOCKMEM_H
////////////////////////////////////////////////////////////////////
#include "mvrtnode.h"
////////////////////////////////////////////////////////////////////
class MVRTNode;
////////////////////////////////////////////////////////////////////
extern struct BlockMemItem
{
	MVRTNode *add;
	int block;
	int ref;
};
////////////////////////////////////////////////////////////////////
extern class BlockMem
{
public:
	BlockMemItem *items;
	int num;

public:
	BlockMem(int _num);
	~BlockMem();

	void AddBlock(MVRTNode *_add, int _block);
	void DelBlock(int _block);
	void error(char *msg,bool ex);
	MVRTNode * LoadBlock(int _block);
};
////////////////////////////////////////////////////////////////////
#endif
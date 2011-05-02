#include "blockmem.h"
#include "stdio.h"
#include "stdlib.h"
////////////////////////////////////////////////////////////////////
BlockMem::BlockMem(int _num)
{
	num=_num;
	items=new BlockMemItem[num];
	for (int i=0;i<num;i++)
	{
		items[i].add=NULL; items[i].ref=0;
	}
}
////////////////////////////////////////////////////////////////////
BlockMem::~BlockMem()
{
	printf("Freeing the block memory...");
	for (int i=0;i<num;i++)
		if (items[i].ref>0)
		{
			printf("Warning: Block %d has not been freed\n",items[i].block);
			delete items[i].add;
			items[i].add=NULL; items[i].ref=0; 
		}
	delete [] items;
}
////////////////////////////////////////////////////////////////////
void BlockMem::AddBlock(MVRTNode *_add, int _block)
{
	if (items[_block].ref>0)
		error("Warning: The specified block number is already in use.\n",true);
	items[_block].ref=1;
	items[_block].add=_add;

}
////////////////////////////////////////////////////////////////////
void BlockMem::DelBlock(int _block)
{
	if (MVRTNode::deleting==false)
		error("Illegal delete request\n",true);
	items[_block].ref--;
	if (items[_block].ref<0)
		error("A block not allocated is being deallocated!\n",true);
	if (items[_block].ref==0)
	{
		delete items[_block].add;
		items[_block].add=NULL;
	}
}
////////////////////////////////////////////////////////////////////
void BlockMem::error(char *msg,bool ex)
{
	printf(msg);
	if (ex) exit(1);
}
////////////////////////////////////////////////////////////////////
MVRTNode * BlockMem::LoadBlock(int _block)
{
	if (MVRTNode::creating==false)
		error("Illegal create request\n",true);
	if (_block>=num || _block<0)
		error("Sorry the block you specified exceeds the maximum number.\n",true);
	if (items[_block].ref==0)
		return NULL;
	else
	{
		items[_block].ref++;
		return items[_block].add;
	}
}
////////////////////////////////////////////////////////////////////

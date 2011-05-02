#ifndef __CACHE
#define __CACHE

#include "blk_file.h"
#include "rtreeheader.h"


class AbsRTree;

class Cache
{
   enum uses {free,used,fixed};	// for fuf_cont
   int ptr;		        //current position in cache
   int cachesize;		// Dynamische Cachegroesse (>=0)
   int blocklength;
   int *cache_cont;	    // array of the indices of blocks that are in cache
   AbsRTree **cache_tree;  // array of ptrs to the correspondent RTrees where the blocks belong to
   uses *fuf_cont; 		//indicator array that shows whether one cache block is free, used or fixed
   int  *LRU_indicator; //indicator that shows how old (unused) is a page in the cache
   char **cache;   		// Cache

   // interne Funktionen:

   int next();		// liefert freies Feld, oder, wenn C. voll,
      // die freigemachte ptr-Pos., ptr wird dann hochgesetzt
      // ausgenommen sind fixed Positions
   int in_cache(int index, AbsRTree *rt);	// liefert Pos. im Cache oder -1

public:

   Cache(int blength, int csize);
   ~Cache();

   int page_faults;

   bool read_block(Block b, int i, AbsRTree *rt);
   bool write_block(const Block b, int i, AbsRTree *rt);

   bool fix_block(int i, AbsRTree *rt);	// Block immer im Cache halten
   bool unfix_block(int i, AbsRTree *rt);	// fix_block aufheben
   void unfix_all();			// fix_block insg. aufheben
   void set_cachesize(int s);
   void flush();			// speichert den gesamten Cache
};


#endif // __CACHE

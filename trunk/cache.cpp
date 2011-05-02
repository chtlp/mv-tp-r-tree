#include "cache.h"
/////////////////////////////////////////////////////////////////////////////
Cache::Cache(int csize, int blength)
{
	int i;

	ptr=0;
	blocklength = blength;
	if (csize>=0)
	    cachesize=csize;
	else
	    error("Cache::Cache: negative Cachesize",TRUE);

	cache_cont = new int[cachesize];
	cache_tree = new AbsRTree*[cachesize];
	fuf_cont = new uses[cachesize];
	LRU_indicator = new int[cachesize];

	for (i=0; i<cachesize; i++)
	{
    	cache_cont[i]=0;
    	cache_tree[i]=NULL;
    	fuf_cont[i]=free;
	}
	cache = new char*[cachesize];
	for (i=0; i<cachesize; i++)
	cache[i] = new char[blocklength];

	page_faults = 0;
}

Cache::~Cache()
{
	flush();
	delete[] cache_cont;
	delete[] fuf_cont;
	delete[] LRU_indicator;
	delete[] cache_tree;

	for (int i=0;i<cachesize;i++)
	    delete[] cache[i];
	delete[] cache;
}

int Cache::next()
// liefert freien Platz im Cache oder -1
// ptr rotiert ueber den vollen Cache, um diesen gleichmaessig
// 'auszulasten'
{
   int ret_val, tmp;

   if (cachesize==0)
       return -1;
   else
       if (fuf_cont[ptr]==free)
       {
    	   ret_val = ptr++;
    	   if(ptr==cachesize)
    		   ptr=0;
    	   return ret_val;
	   }
	   else
	   {
		 tmp= (ptr+1)%cachesize;

		 //find the first free block
		 while (tmp!=ptr && fuf_cont[tmp]!=free)
		    if(++tmp==cachesize)
		        tmp=0;

		 if (ptr==tmp)	//failed to find a free block
		 {
		     /*
    		 if(fuf_cont[ptr]==fixed)
    		 {
        		 tmp=ptr+1;
        		 if (tmp==cachesize)
        			 tmp=0;

        		 // suche unfixed Pos.
        		 while(tmp!=ptr && fuf_cont[tmp]!=used)
        			 if(++tmp==cachesize)
        			 //tmp rotiert ueber der Liste
        			 tmp=0;
        		 if (ptr==tmp)
        		 // alles fix
        			 return -1;
        		 else
        				 // tmp zeigt auf used
        			 ptr=tmp;
    	     }
    	     */

	         // select a victim page to be written back to disk
             int lru_index=0; // the index of the victim page
             for (int i=1; i<cachesize; i++)
                if (LRU_indicator[i]>LRU_indicator[lru_index])
                    lru_index=i;

             ptr = lru_index;
    		 cache_tree[ptr]->file->write_block(cache[ptr],cache_cont[ptr]-1);
    		 fuf_cont[ptr]=free;
    		 ret_val=ptr++;
    		 if (ptr==cachesize)
    		 ptr=0;
    		 return ret_val;
    	 }
    	 else
		    return tmp;
	 }
}

int Cache::in_cache(int index, AbsRTree *rt)
// liefert Pos. eines Blocks im Cache, sonst -1
{
   int i;

   for (i = 0; i<cachesize; i++)
	   if ((cache_cont[i] == index) && (cache_tree[i] == rt) && (fuf_cont[i] != free))
	   {
	       LRU_indicator[i]=0;
		   return i;
	   }
	   else if (fuf_cont[i] != free)
           LRU_indicator[i]++; // increase indicator for this block
   return -1;
}

bool Cache::read_block(Block block,int index, AbsRTree *rt)
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if(index<=rt->file->get_num_of_blocks() && index>0)
	{
    	if((c_ind=in_cache(index,rt))>=0)
    	{
    		memcpy(block,cache[c_ind],blocklength);
    	}
    	else // not in Cache
    	{
    	    page_faults++;
    		c_ind = next();
    		if (c_ind >= 0) // a block has been freed in cache
    		{
        		rt->file->read_block(cache[c_ind],index-1); // ext. Num.
        		cache_cont[c_ind]=index;
        		cache_tree[c_ind]=rt;
        		fuf_cont[c_ind]=used;
        		LRU_indicator[c_ind] = 0;
        		memcpy(block,cache[c_ind],blocklength);
    		}
    		else
    			// kein Cache mehr verfuegbar
    		    rt->file->read_block(block,index-1); // read-through (ext.Num.)
    	}
    	return TRUE;
	}
	else
	    return FALSE;
}

bool Cache::write_block(const Block block, int index, AbsRTree *rt)
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if(index <= rt->file->get_num_of_blocks() && index > 0)
	{
    	c_ind = in_cache(index, rt);
    	if(c_ind >= 0)	// gecached?
    		memcpy(cache[c_ind], block, blocklength);
    	else		// not in Cache
    	{
    		c_ind = next();
    		if (c_ind >= 0)
    			// im Cache was frei? (cachesize==0?)
    		{
        		memcpy(cache[c_ind],block,blocklength);
        		cache_cont[c_ind]=index;
        		cache_tree[c_ind]=rt;
        		fuf_cont[c_ind]=used;
        		LRU_indicator[c_ind] = 0;
    		}
    		else
    			// kein Cache verfuegbar
    			// write-through (ext.Num.)
    		rt->file->write_block(block,index-1);
	    }
	    return TRUE;
	}
	else
	    return FALSE;
}

bool Cache::fix_block(int index, AbsRTree *rt)
// gefixte Bloecke bleiben immer im Cache
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if (index<=rt->file->get_num_of_blocks() && index>0)
	{
    	if((c_ind=in_cache(index,rt))>=0) 	// gecached?
    		fuf_cont[c_ind]=fixed;
    	else		// nicht im C.
    		if((c_ind=next())>=0)	// im Cache was frei?
    		{
        		rt->file->read_block(cache[c_ind],index-1); // ext.Num.
        		cache_cont[c_ind]=index;
        		cache_tree[c_ind]=rt;
        		fuf_cont[c_ind]=fixed;
    		}
    		else	// kein Cache verfuegbar
    		    return FALSE;
    	return TRUE;
	}
	else
	    return FALSE;
}

bool Cache::unfix_block(int index, AbsRTree *rt)
// Fixierung eines Blocks durch fix_block wieder aufheben
{
	int i;

	i =0;
	index++;	// Externe Num. --> interne Num.
	if(index<=rt->file->get_num_of_blocks() && index>0)
	{
    	while(i<cachesize && (cache_cont[i]!=index || fuf_cont[i]==free))
    		i++;
    	if (i!=cachesize)
    		fuf_cont[i]=used;
    	return TRUE;
	}
	else
	    return FALSE;
}

void Cache::unfix_all()
   // Hebt alle Fixierungen auf
{
	int i;

	for (i=0; i<cachesize; i++)
	if (fuf_cont[i]==fixed)
		fuf_cont[i]=used;
}

void Cache::set_cachesize(int size)
// sichert und loescht alten Cache, und baut neuen auf (size>=0)
{
	int i;

	if (size>=0)
	{
    	ptr=0;
    	flush();
    	for(i=0; i<cachesize; i++)
    		delete[] cache[i];
    	delete[] cache;
    	delete[] cache_cont;
    	delete[] cache_tree;
    	delete[] fuf_cont;
    	delete[] LRU_indicator;
    	cachesize = size;
    	cache_cont = new int[cachesize];
    	cache_tree = new AbsRTree*[cachesize];
    	LRU_indicator = new int[cachesize];
    	fuf_cont = new uses[cachesize];
    	for (i=0; i<cachesize; i++)
    	{
    		cache_cont[i]=0;  // 0 bedeutet Cache unbenutzt (leer)
    		cache_tree[i]=NULL;
    		fuf_cont[i]=free;
    	}
    	cache = new char*[cachesize];
    	for (i=0; i<cachesize; i++)
    		cache[i] = new char[blocklength];
	}
	else
	    error("CachedBlockFile::set_cachesize : neg. cachesize",TRUE);
}

void Cache::flush()
// schreibt den  Cache auf Platte, gibt ihn nicht frei
{
	int i;

	for (i=0; i<cachesize; i++)
	if (fuf_cont[i]!=free)	// Cache besetzt?
		cache_tree[i]->file->write_block(cache[i], cache_cont[i]-1); // ext.Num.
}
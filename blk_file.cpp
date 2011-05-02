/* seq_file.cc :: Implementation einer sequentiellen Filestruktur durch
                  gecachede Bloecke.  Tobias Mayr(mayrt) 4/95
	          soll einmal Alternative zu SequentialList sein */

#include "blk_file.h"

void error(char *t, bool ex);
//================================================================
//===========================BlockFile============================
// Interne Funktionen:

void BlockFile::fwrite_number(int value)
   // schreibt an SEEK_CUR eine Zahl, siehe fread_number
{
   put_bytes((char *) &value,sizeof(int));
}

int BlockFile::fread_number()
   // liest von SEEK_CUR eine Zahl, siehe fwrite_number
{
   char ca[sizeof(int)];

   get_bytes(ca,sizeof(int));
   return *((int *)ca);
}

// Zugriffsfunktionen:-------------------------------------------------

BlockFile::BlockFile(char* name,int b_length)
{
   char *buffer;
   int l;

   filename = new char[strlen(name) + 1];
   strcpy(filename,name);
   blocklength = b_length;	// nur fuer neue Files relevant
   number = 0;
   iocount=0;

   if((fp=fopen(name,"rb+"))!=0)	// zum update oeffnen
   // bekanntes File
   {
      new_flag = FALSE;
      blocklength = fread_number();
//	printf("Read number: %d\n", blocklength);
      number = fread_number();	// Gesamtzahl der Bloecke lesen
   }
   else
   // unbekanntes File
   {
      if (blocklength < BFHEAD_LENGTH) {
    	error("BlockFile::BlockFile: Blocks zu kurz\n",TRUE);
	  }

      fp=fopen(filename,"wb+");
      if (fp == 0)
	  error("BlockFile::new_file: Schreibfehler",TRUE);

      new_flag = TRUE;
      fwrite_number(blocklength);

      // Gesamtzahl Bloecke ist 0
      fwrite_number(0);
	  buffer = new char[(l=blocklength-(int)ftell(fp))];
      memset(buffer, 0, sizeof(buffer));
      put_bytes(buffer,l);

      delete [] buffer;
   }
   fseek(fp,0,SEEK_SET);	// auf Anfang (Header) gehen
   act_block=0;			// aktueller Block ist der Header
}

BlockFile::~BlockFile()
{
   delete[] filename;
   fclose(fp);
}

void BlockFile::read_header(char* buffer)
{
   fseek(fp,BFHEAD_LENGTH,SEEK_SET);
   get_bytes(buffer,blocklength-BFHEAD_LENGTH);

   if(number<1)
   {
       fseek(fp,0,SEEK_SET);
       act_block=0;
   }
   else
       act_block=1;
}

void BlockFile::set_header(char* header)
{
   fseek(fp,BFHEAD_LENGTH,SEEK_SET);
   put_bytes(header,blocklength-BFHEAD_LENGTH);
   if(number<1)
   {
       fseek(fp,0,SEEK_SET);
       act_block=0;
   }
   else
       act_block=1;
}

bool BlockFile::read_block(Block b,int pos)
// pos bezieht sich auf externe Nummerierung beginnend bei 0 NACH dem Header
{
   // Externe Num. --> interne Num.
   pos++;
   if (pos<=number && pos>0)
       seek_block(pos);
   else
   {
	   error("BlockFile::read_block--The requested block number has not been allocated\n",true);
       return FALSE;
   }

   get_bytes(b,blocklength);
   if (pos+1>number)
   {
       fseek(fp,0,SEEK_SET);
       act_block=0;
   }
   else
       act_block=pos+1;

   iocount++;
   return TRUE;
}

 bool BlockFile::write_block(const Block block,int pos)
// pos bezieht sich auf externe Nummerierung beginnend bei 0 NACH dem Header
{
   // Externe Num. --> interne Num.
   pos++;

   if (pos<=number && pos>0)
       seek_block(pos);
   else
       return FALSE;

   put_bytes(block,blocklength);
   if (pos+1>number)
   {
       fseek(fp,0,SEEK_SET);
       act_block=0;
   }
   else
       act_block=pos+1;

   //iocount++;              I disabled this line to make sure we only count reading's in a query
   return TRUE;
}

int BlockFile::append_block(const Block block)
// Das Resulat bezieht sich auf externe Nummerierung
{
   fseek(fp,0,SEEK_END);
   put_bytes(block,blocklength);
   number++;
   fseek(fp,sizeof(int),SEEK_SET);
   fwrite_number(number);
   fseek(fp,-blocklength,SEEK_END);

   // -1 zur Umrechnung in externe Num.
   return (act_block=number)-1;
}

bool BlockFile::delete_last_blocks(int num)
{
   if (num>number)
      return FALSE;
#ifdef UNIX
   if (truncate(filename,(number-num+1)*blocklength) != 0)
	  error("BlockFile::delete_last_blocks : truncate-Fehler",TRUE);
#endif
   number -= num;
   fseek(fp,sizeof(int),SEEK_SET);
   fwrite_number(number);
   fseek(fp,0,SEEK_SET);
   act_block=0;
   return TRUE;
}


//================================================================
//========================CachedBlockFile=========================

// Interne Funktionen:--------------------------------------------

int CachedBlockFile::next()
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
    		 BlockFile::write_block(cache[ptr],cache_cont[ptr]-1);
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

int CachedBlockFile::in_cache(int index)
// liefert Pos. eines Blocks im Cache, sonst -1
{
   int i;

   for (i = 0; i<cachesize; i++)
	   if (cache_cont[i] == index && fuf_cont[i] != free)
	   {
	       LRU_indicator[i]=0;
		   return i;
	   }
	   else if (fuf_cont[i] != free)
                LRU_indicator[i]++; // increase indicator for this block
   return -1;
}

// Zugriffsfunktionen:-------------------------------------------------

CachedBlockFile::CachedBlockFile(char* name,int blength, int csize)
   : BlockFile(name,blength)
{
	int i;
    pagefault=0;
	ptr=0;
	if (csize>=0)
	cachesize=csize;
	else
	error("CachedBlockFile::CachedBlockFile: neg. Cachesize",TRUE);
	cache_cont = new int[cachesize];
	fuf_cont = new uses[cachesize];
	LRU_indicator = new int[cachesize];

	for (i=0; i<cachesize; i++)
	{
	cache_cont[i]=0;  // 0 bedeutet Cache unbenutzt (leer)
	fuf_cont[i]=free;
	}
	cache = new char*[cachesize];
	for (i=0; i<cachesize; i++)
	cache[i] = new char[get_blocklength()];
}

CachedBlockFile::~CachedBlockFile()
{
	flush();
	delete[] cache_cont;
	delete[] fuf_cont;
	delete[] LRU_indicator;

	for (int i=0;i<cachesize;i++)
	delete[] cache[i];
	delete[] cache;
	printf("Total I/O: %d\n",pagefault);
}

bool CachedBlockFile::read_block(Block block,int index)
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if(index<=get_num_of_blocks() && index>0)
	{
	if((c_ind=in_cache(index))>=0)
		// gecached?
	{
		memcpy(block,cache[c_ind],get_blocklength());
	}
	else
		// nicht im Cache
	{
		pagefault++;
		c_ind = next();
		if (c_ind >= 0)
			// Ist im Cache noch was frei?
		{
		BlockFile::read_block(cache[c_ind],index-1); // ext. Num.
		cache_cont[c_ind]=index;
		fuf_cont[c_ind]=used;
		LRU_indicator[c_ind] = 0;
		memcpy(block,cache[c_ind],get_blocklength());
		}
		else
			// kein Cache mehr verfuegbar
		BlockFile::read_block(block,index-1); // read-through (ext.Num.)
	}
	return TRUE;
	}
	else
	return FALSE;
}

bool CachedBlockFile::write_block(const Block block,int index)
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if(index <= get_num_of_blocks() && index > 0)
	{
	c_ind = in_cache(index);
	if(c_ind >= 0)	// gecached?
		memcpy(cache[c_ind], block, get_blocklength());
	else		// nicht im C.
	{
		c_ind = next();
		if (c_ind >= 0)
			// im Cache was frei? (cachesize==0?)
		{
		memcpy(cache[c_ind],block,get_blocklength());
		cache_cont[c_ind]=index;
		fuf_cont[c_ind]=used;
		}
		else
			// kein Cache verfuegbar
			// write-through (ext.Num.)
		BlockFile::write_block(block,index-1);
	}
	return TRUE;
	}
	else
	return FALSE;
}

bool CachedBlockFile::fix_block(int index)
// gefixte Bloecke bleiben immer im Cache
{
	int c_ind;

	index++;	// Externe Num. --> interne Num.
	if (index<=get_num_of_blocks() && index>0)
	{
	if((c_ind=in_cache(index))>=0) 	// gecached?
		fuf_cont[c_ind]=fixed;
	else		// nicht im C.
		if((c_ind=next())>=0)	// im Cache was frei?
		{
		BlockFile::read_block(cache[c_ind],index-1); // ext.Num.
		cache_cont[c_ind]=index;
		fuf_cont[c_ind]=fixed;
		}
		else	// kein Cache verfuegbar
		return FALSE;
	return TRUE;
	}
	else
	return FALSE;
}

bool CachedBlockFile::unfix_block(int index)
// Fixierung eines Blocks durch fix_block wieder aufheben
{
	int i;

	i =0;
	index++;	// Externe Num. --> interne Num.
	if(index<=get_num_of_blocks() && index>0)
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

void CachedBlockFile::unfix_all()
   // Hebt alle Fixierungen auf
{
	int i;

	for (i=0; i<cachesize; i++)
	if (fuf_cont[i]==fixed)
		fuf_cont[i]=used;
}

void CachedBlockFile::set_cachesize(int size)
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
	delete[] fuf_cont;
	cachesize = size;
	cache_cont = new int[cachesize];
	fuf_cont = new uses[cachesize];
	for (i=0; i<cachesize; i++)
	{
		cache_cont[i]=0;  // 0 bedeutet Cache unbenutzt (leer)
		fuf_cont[i]=free;
	}
	cache = new char*[cachesize];
	for (i=0; i<cachesize; i++)
		cache[i] = new char[get_blocklength()];
	}
	else
	error("CachedBlockFile::set_cachesize : neg. cachesize",TRUE);
}

void CachedBlockFile::flush()
// schreibt den  Cache auf Platte, gibt ihn nicht frei
{
	int i;

	for (i=0; i<cachesize; i++)
	if (fuf_cont[i]!=free)	// Cache besetzt?
		BlockFile::write_block(cache[i], cache_cont[i]-1); // ext.Num.
}












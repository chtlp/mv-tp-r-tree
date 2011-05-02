/* blk_file.h :: Implementation einer sequentiellen Filestruktur durch
   	         gecachede Bloecke (zur Implementation eines Index) (mayrt)*/
#ifndef __BLK_FILE
#define __BLK_FILE

#define BFHEAD_LENGTH (sizeof(int)*2) // interner Teil des Headerblocks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char Block[];
#define TRUE 1
#define FALSE 0

/******* Added these 3 lines ***********/
#define SEEK_CUR 1
#define SEEK_SET 0
#define SEEK_END 2
/******* end of addition ********/


//-------------------------------------------------------------------------
class BlockFile
   // File mit blockweisem Zugriff
   // Der erste Block ist ein Header-Block,der die Blocklaenge und die
   // Blockanzahl enthaelt. Der Header wird dabei nicht mitgezaehlt.
   // Die Userblocks beginnen also beim zweiten Block
   // (intern Block 1/extern fuer den Benutzer Block 0 !)
{
public:
   FILE* fp;			// Pointer auf das File
   char* filename;		// fuer evtl. truncate noetig
   int blocklength;	        // Laenge eines Blocks in Byte
   int act_block; 	        // interne Nummer des Blocks auf den
				// SEEK_CUR zeigt
   int number;		        // Gesamtzahl der Bloecke (exklusive Header)
   bool new_flag;		// in dieser Sitzung erzeugtes File?
   int iocount;

   
   
   
   void put_bytes(const char* bytes,int num)  // schreibt num Bytes ins File
   { fwrite(bytes,num,1,fp); }
   void get_bytes(char* bytes,int num)	      // liest num Bytes nach bytes
   { fread(bytes,num,1,fp); }
   void fwrite_number(int num);	// schreibt Nummer
   int fread_number();		// liest Nummer
   void seek_block(int bnum)    // setzt SEEK_CUR unkontrolliert
   { fseek(fp,(bnum-act_block)*blocklength,SEEK_CUR); }
   //   friend void test_it(); // nur zum Testen
   BlockFile(char* name, int b_length);
   			        // Filename und Blocklaenge (fuer neue Files)
   ~BlockFile();

   void read_header(char * header);	// liest Headerblock ohne
   					// Blockfile-internen Header
   void set_header(char* header);	// setzt Blockfileheader  ohne
   					// Blockfile-internen Header
   // Genau die naechsten drei Funktionen benutzen externe Nummerierung
   // Der Block nach dem Header, also der zweite, ist dabei Block 0
   bool read_block(Block b,int i);	        // liest
   bool write_block(const Block b,int i);	// schreibt Block
   int append_block(const Block b);	// liefert Blocknummer

   bool delete_last_blocks(int num);	// loescht die letzten num Bloecke

   bool file_new()			// TRUE nur fuer ein in dieser Sitzung
   { return new_flag; }			// erzeugtes File
   int get_blocklength()	// liefert Blocklaenge
   { return blocklength; }
   int get_num_of_blocks()	// liefert Gesamtzahl (ohne Header)
   { return number; }
   int get_iocount()
   {
	   return iocount;
   }
};

//-------------------------------------------------------------------------
class CachedBlockFile : public BlockFile
   // BlockFile mit read-write-Cache dynamischer Groesse
{
   enum uses {free,used,fixed};	// fuer fuf_cont
   int ptr;		        //current position in cache
   int cachesize;		// Dynamische Cachegroesse (>=0)
   int *cache_cont;	    // array of the indices of blocks that are in cache
   uses *fuf_cont; 		//indicator array that shows whether one cache block is free, used or fixed
   int  *LRU_indicator; //indicator that shows how old (unused) is a page in the cache
   char **cache;   		// Cache
   
   // interne Funktionen:

   int next();		// liefert freies Feld, oder, wenn C. voll,
      // die freigemachte ptr-Pos., ptr wird dann hochgesetzt
      // ausgenommen sind fixed Positions
   int in_cache(int index);	// liefert Pos. im Cache oder -1

public:

   int pagefault;

   CachedBlockFile(char* name,int blength, int csize);	// Filename,Blocklaenge
   					// und Cachegroesse
   ~CachedBlockFile();

   int get_pagefault()
   {return pagefault;}
   bool read_block(Block b,int i);
   bool write_block(const Block b,int i);

   bool fix_block(int i);	// Block immer im Cache halten
   bool unfix_block(int i);	// fix_block aufheben
   void unfix_all();			// fix_block insg. aufheben
   void set_cachesize(int s);
   void flush();			// speichert den gesamten Cache
};


#endif // __BLK_FILE

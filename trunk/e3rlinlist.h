#ifndef __E3RLINLIST
#define __E3RLINLIST

#include "stdio.h"
#include "e3rtree.h"

void error(char *t, bool ex);

////////////////////////////////////////////////////////////////////////
// E3RLinList  (E3RSLink)
////////////////////////////////////////////////////////////////////////

class E3REntry;

struct E3RSLink
{
    E3REntry *d;          // Zeiger auf Element-Daten
    E3RSLink *next;          // Zeiger auf naechstes Element
    E3RSLink *prev;          // Zeiger auf vorhergehendes Element

    E3RSLink();
    ~E3RSLink();
};

////////////////////////////////////////////////////////////////////////
// E3RLinList
////////////////////////////////////////////////////////////////////////


class E3RLinList
{
public:
    E3RSLink *first;         // Rootzeiger des Datenbestands
    E3RSLink *last;          // Zeiger auf letztes Element
    int anz;              // Anzahl der belegten Elemente in der Liste
    E3RSLink *akt;           // zeigt auf aktuelles Element
    int akt_index;        // Index des zuletzt mit get geholten Elements


    E3RLinList();
    virtual ~E3RLinList();
    int get_num()               // gibt Anzahl der im Index belegten Elements
        { return anz; }         // zurueck

    void check();               // ueberprueft Konsistenz der Liste
    //void print();

    void insert(E3REntry *f);   // haengt ein Element vorne an die Liste an
    bool erase();               // loescht aktuelles Element aus der Liste

    E3REntry * get(int i);      // liefert i-tes Element
	                            //Note that type cast is necessary to convert to other types of
	                            //entries
    E3REntry * get_first();     // liefert erstes Element im Index
    E3REntry * get_last();      // liefert erstes Element im Index
    E3REntry * get_next();      // liefert naechstes Element im Index
    E3REntry * get_prev();      // liefert vorhergehendes Element im Index
};


////////////////////////////////////////////////////////////////////////
// SortedE3RLinList
////////////////////////////////////////////////////////////////////////


class SortedE3RLinList : public E3RLinList
{
    bool increasing;
public:
    SortedE3RLinList();

    void set_sorting(bool _increasing); // wenn increasing gleich TRUE, wird
                                // aufsteigend einsortiert
                                // DIESE FUNKTION MUSS VOR DEM ERSTEN EINFUEGEN
                                // GERUFEN WERDEN !!!!!!!!!!!
    void insert(E3REntry *f);   // fuegt ein Element durch direktes Einfuegen ein

    void sort(bool _increasing);// sortiert die Liste

    void check();               // ueberprueft die Konsistenz der Liste
};

#endif  // __LINLIST

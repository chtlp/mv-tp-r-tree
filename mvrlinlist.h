#ifndef __MVRLINLIST
#define __MVRLINLIST

#include <stdio.h>
#include "mvrtree.h"

void error(char *t, bool ex);

////////////////////////////////////////////////////////////////////////
// MVRLinList  (SLink)
////////////////////////////////////////////////////////////////////////

class MVREntry;

struct SLink
{
    MVREntry *d;             // Zeiger auf Element-Daten
    SLink *next;          // Zeiger auf naechstes Element
    SLink *prev;          // Zeiger auf vorhergehendes Element

    SLink();
    ~SLink();
};

////////////////////////////////////////////////////////////////////////
// MVRLinList
////////////////////////////////////////////////////////////////////////


class MVRLinList
{
protected:
    SLink *first;         // Rootzeiger des Datenbestands
    SLink *last;          // Zeiger auf letztes Element
    int anz;                    // Anzahl der belegten Elemente in der Liste
    SLink *akt;           // zeigt auf aktuelles Element
    int akt_index;              // Index des zuletzt mit get geholten Elements
public:
    MVRLinList();
    virtual ~MVRLinList();
    int get_num()               // gibt Anzahl der im Index belegten Elements
        { return anz; }         // zurueck

    void check();               // ueberprueft Konsistenz der Liste
    void print();

    void insert(MVREntry *f);       // haengt ein Element vorne an die Liste an
    bool erase();               // loescht aktuelles Element aus der Liste

    MVREntry * get(int i);          // liefert i-tes Element
    MVREntry * get_first();         // liefert erstes Element im Index
    MVREntry * get_last();          // liefert erstes Element im Index
    MVREntry * get_next();          // liefert naechstes Element im Index
    MVREntry * get_prev();          // liefert vorhergehendes Element im Index
};


////////////////////////////////////////////////////////////////////////
// SortedMVRLinList
////////////////////////////////////////////////////////////////////////


class SortedMVRLinList : public MVRLinList
{
    bool increasing;
public:
    SortedMVRLinList();

    void set_sorting(bool _increasing); // wenn increasing gleich TRUE, wird
                                // aufsteigend einsortiert
                                // DIESE FUNKTION MUSS VOR DEM ERSTEN EINFUEGEN
                                // GERUFEN WERDEN !!!!!!!!!!!
    void insert(MVREntry *f);       // fuegt ein Element durch direktes Einfuegen ein

    void sort(bool _increasing);// sortiert die Liste

    void check();               // ueberprueft die Konsistenz der Liste
};

#endif  // __LINLIST

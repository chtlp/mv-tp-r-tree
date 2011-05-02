#include "e3rlinlist.h"

E3RSLink::E3RSLink()
{
    d = NULL;
    next = prev = NULL;
}


E3RSLink::~E3RSLink()
{
    delete d;
}


E3RLinList::E3RLinList()
{
    anz = 0;
    akt_index = -1;
    akt = first = last = NULL;
}


E3RLinList::~E3RLinList()
{
    E3RSLink *hf;

    // Elemente freigeben
    akt = first;
    while (akt != NULL)
    {
	hf = akt->next;
	delete akt;
	akt = hf;
    }
}


void E3RLinList::insert(E3REntry *f)
{
    E3RSLink *sd;

    // neuen Rahmen erzeugen
    sd = new E3RSLink;
    sd->d = f;

    // Zeiger umbiegen
    sd->next = first;
    sd->prev = NULL;
    if (first != NULL)
	first->prev = sd;

    // first, last und anz berichtigen
    anz++;
    first = sd;
    if (last == NULL)
	last = sd;

    // Position ist undefiniert
    akt = NULL;
    akt_index = -1;
}


bool E3RLinList::erase()
{
    E3RSLink *n_akt;

    // Liste leer oder akt nicht definiert?
    if (akt)
    {
	// Element ist erstes Element
	if (akt == first)
	{
	    // Element ist einziges Element
	    if (akt == last)
	    {
		akt_index = -1;
		first = last = NULL;
		n_akt = NULL;
	    }
	    else
	    {
		// Element ist erstes Element, aber nicht leztes
		(akt->next)->prev = NULL;
		first = akt->next;
		n_akt = first;
		akt_index = 0;
	    }
	}
	else
	{
	    // Element ist letztes Element
	    if (akt == last)
	    {
		(akt->prev)->next = NULL;
		last = akt->prev;
		n_akt = NULL;
		akt_index = -1;
	    }
	    else
	    // Element ist mitten in der Liste
	    {
		(akt->next)->prev = akt->prev;
		(akt->prev)->next = akt->next;
		n_akt = akt->next;
		akt_index++;
	    }
	}

	// Speicher freigeben
	delete akt;

	// aktuelles Element setzen
	akt = n_akt;

	// anz berichtigen
	anz--;
	return TRUE;
    }
    return FALSE;
}


E3REntry* E3RLinList::get(int i)
// liefert das i-te Element in der Liste
{
    bool ahead;   // wenn ahead TRUE ist, wird in next-Richtung gesucht
    int j;

    // liegt das i-te Element ueberhaupt in der Liste?
    if (i >= anz)
	return NULL;

    // ist die Liste schon auf das i-te Element positioniert?
    if (i == akt_index)
	return akt->d;

    // hat eine Positionierung der Liste stattgefunden?
    if (akt_index == -1)
    {
	// i liegt naeher an first, als an last
	if (i < (anz / 2))
	{
	    akt = first;
	    akt_index = 0;
	    ahead = TRUE;
	}
	else
	{
	    akt = last;
	    akt_index = anz - 1;
	    ahead = FALSE;
	}
    }
    else
    {
	// die gewuenschte Position liegt vor der aktuellen
	if (i < akt_index)
	{
	    // liegt i naeher an first, als an akt_index?
	    if ((akt_index - i) > i)
	    {
		akt = first;
		akt_index = 0;
		ahead = TRUE;
	    }
	    else
		ahead = FALSE;
	}
	else
	{
	    // liegt i naeher an last, als an akt_index?
	    if ((i - akt_index) > ((anz-1) - i))
	    {
		akt = last;
		akt_index = anz - 1;
		ahead = FALSE;
	    }
	    else
		ahead = TRUE;
	}
    }


    // gesuchter Index liegt in next - Richtung
    if (ahead)
    {
	for (j = akt_index; j < i; j++)
	{
	    if (!akt)
		error("E3RLinList::get: List seems to be inkonsistent", TRUE);
	    akt = akt->next;
	}
    }
    else
    {
	for (j = akt_index; j > i; j--)
	{
	    if (!akt)
		error("E3RLinList::get: List seems to be inkonsistent", TRUE);
	    akt = akt->prev;
	}
    }

    akt_index = i;
    return akt->d;
}


E3REntry* E3RLinList::get_first()
{
    akt = first;

    if (akt != NULL)
    {
	akt_index = 0;
	return akt->d;
    }
    else
	return NULL;
}


E3REntry* E3RLinList::get_last()
{
    akt = last;

    if (akt != NULL)
    {
	akt_index = anz - 1;
	return akt->d;
    }
    else
	return NULL;
}


E3REntry* E3RLinList::get_next()
{
    akt = akt->next;

    if (akt != NULL)
    {
	akt_index++;
	return akt->d;
    }
    else
    {
	akt_index = -1;
	return NULL;
    }
}


E3REntry* E3RLinList::get_prev()
{
    akt = akt->prev;

    if (akt != NULL)
    {
	akt_index--;
	return akt->d;
    }
    else
    {
	akt_index = -1;
	return NULL;
    }
}

/*
void E3RLinList::print()
{
    E3RSLink *cur = first;

    while (cur != NULL)
    {
	printf("%d %f %f %f %f\n", cur->d->son, cur->d->bounces[0], cur->d->bounces[1], cur->d->bounces[2],  cur->d->bounces[3]);
	cur = cur->next;
    }
}*/

void E3RLinList::check()
{
    E3RSLink *f, *old_f;
    int myanz;
    char buffer[255];

    old_f = first;
    // Liste muss ganz leer sein
    if (old_f == NULL)
    {
	if (last != NULL)
	    error("E3RLinList::check: first == NULL, last != NULL", FALSE);
	if (anz != 0)
	    error("E3RLinList::check: first == NULL, anz != 0", FALSE);
	return;
    }

    myanz = 1;
    if (old_f->prev != NULL)
    {
	error("E3RLinList::check: Listenkopf.prev ungleich NULL", FALSE);
	return;
    }

    for (f = old_f->next; f != NULL; f = f->next)
    {
	if (f->prev != old_f)
	{
	    error("E3RLinList::check: Rueckwaertsverkettung fehlerhaft", FALSE);
            return;
        }
	if (old_f->next != f)
	{
	    error("E3RLinList::check: Vorwaertsverkettung fehlerhaft", FALSE);
            return;
        }
	old_f = f;

	myanz ++;
	if (myanz > anz)
	{
	    sprintf(buffer, "E3RLinList::check: anz (%d != %d) passt nicht", myanz, anz);
	    error(buffer, FALSE);
	    return;
	}
    }

    if (old_f->next != NULL)
    {
	error("E3RLinList::check: Listenende.next ungleich NULL", FALSE);
	return;
    }

    if (last != old_f)
    {
	error("E3RLinList::check: last ungleich Listenende", FALSE);
	return;
    }

    if (myanz != anz)
    {
	sprintf(buffer, "E3RLinList::check: anz (%d != %d) passt nicht", myanz, anz);
	error(buffer, FALSE);
    }
}


////////////////////////////////////////////////////////////////////////
// SortedE3RLinList
////////////////////////////////////////////////////////////////////////

SortedE3RLinList::SortedE3RLinList()
    : E3RLinList()
{
    increasing = TRUE;
}



void SortedE3RLinList::set_sorting(bool _increasing)
{
    increasing = _increasing;
}


void SortedE3RLinList::check()
{
    E3RSLink *sd;
    E3REntry *old = new E3REntry();
    bool valid;          // falls TRUE, ist old mit einem Wert besetzt

    // Verkettung konsistent ?
    E3RLinList::check();

    // Sortierung korrekt ?
    valid = FALSE;
    for (sd = first; sd != NULL; sd = sd->next)
    {
	 if (valid && (sd->d->son) < old->son)
         {
	     error("E3RLinList::check: Liste unsortiert", FALSE);
	     return;
         }
	 valid = TRUE;
	 old = sd->d;
    }
}


void SortedE3RLinList::insert(E3REntry *f)
{
    E3RSLink *sd;

    // Rahmen fuer neues Datenelement erzeugen
    sd = new E3RSLink;
    sd->d = f;

    // Liste leer?
    if (first == NULL)
    {
	first = sd;
	last = sd;
	anz = 1;
	sd->next = sd->prev = NULL;

	return;
    }

    // Einfuegestelle bestimmen
    if (increasing)
	for (akt = first; akt != NULL && (akt->d->son) < f->son;
				 akt = akt->next)
	    ;
    else
	for (akt = first; akt != NULL && akt->d->son > f->son; akt = akt->next)
	    ;

    // neues Element muss vor akt eingefuegt werden --> Zeiger umbiegen
    if (akt != NULL)
    {
	if (akt == first)
	    first = sd;
	else
	    (akt->prev)->next = sd;
	sd->next = akt;
	sd->prev = akt->prev;
	akt->prev = sd;
    }
    else
    // neues Element muss als letztes Element eingefuegt werden -->
    // Zeiger umbiegen
    {
	sd->prev = last;
	sd->next = NULL;
	last->next = sd;
	last = sd;
    }

    // Gesamtanzahl erhoehen
    anz ++;
    akt_index = -1;
}



void SortedE3RLinList::sort(bool _increasing)
// Bubblesort der ganzen Liste
{
    E3RSLink *old_akt;
    E3REntry *s;
    bool any, swap;

    set_sorting(_increasing);

    any = TRUE;
    while (any)
    {
	any = FALSE;
	old_akt = NULL;
	for (akt = first; akt != NULL; akt = akt->next)
	{
	    // beim ersten Element gibts nichts zu vergleichen
	    if (old_akt != NULL)
	    {
		swap = FALSE;
		if (_increasing)
		{
		    if ((akt->d->son) > (old_akt->d->son))
			swap = TRUE;
		}
		else
		{
		    if ((akt->d->son) < (old_akt->d->son))
			swap = TRUE;
		}

		if (swap)
		{
		    // es muessen nur die Datenpointer umgehaengt werden !
		    s = akt->d;
		    akt->d = old_akt->d;
		    old_akt->d = s;

		    // in diesem Durchlauf hat sich was getan -->
		    // neuer Durchlauf
		    any = TRUE;
		}
	    }
	    old_akt = akt;
	}
    }

    akt_index = -1;
}


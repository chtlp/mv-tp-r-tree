#include "mvrlinlist.h"

SLink::SLink()
{
    d = NULL;
    next = prev = NULL;
}


SLink::~SLink()
{
    delete d;
}


MVRLinList::MVRLinList()
{
    anz = 0;
    akt_index = -1;
    akt = first = last = NULL;
}


MVRLinList::~MVRLinList()
{
    SLink *hf;

    // Elemente freigeben
    akt = first;
    while (akt != NULL)
    {
	hf = akt->next;
	delete akt;
	akt = hf;
    }
}


void MVRLinList::insert(MVREntry *f)
{
    SLink *sd;

    // neuen Rahmen erzeugen
    sd = new SLink;
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


bool MVRLinList::erase()
{
    SLink *n_akt;

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


MVREntry* MVRLinList::get(int i)
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
		error("MVRLinList::get: List seems to be inkonsistent", TRUE);
	    akt = akt->next;
	}
    }
    else
    {
	for (j = akt_index; j > i; j--)
	{
	    if (!akt)
		error("MVRLinList::get: List seems to be inkonsistent", TRUE);
	    akt = akt->prev;
	}
    }

    akt_index = i;
    return akt->d;
}


MVREntry* MVRLinList::get_first()
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


MVREntry* MVRLinList::get_last()
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


MVREntry* MVRLinList::get_next()
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


MVREntry* MVRLinList::get_prev()
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

void MVRLinList::print()
{
    SLink *cur = first;

    while (cur != NULL)
    {
	printf("%d %f %f %f %f\n", cur->d->son, cur->d->bounces[0], cur->d->bounces[1], cur->d->bounces[2],  cur->d->bounces[3]);
	cur = cur->next;
    }
}

void MVRLinList::check()
{
    SLink *f, *old_f;
    int myanz;
    char buffer[255];

    old_f = first;
    // Liste muss ganz leer sein
    if (old_f == NULL)
    {
	if (last != NULL)
	    error("MVRLinList::check: first == NULL, last != NULL", FALSE);
	if (anz != 0)
	    error("MVRLinList::check: first == NULL, anz != 0", FALSE);
	return;
    }

    myanz = 1;
    if (old_f->prev != NULL)
    {
	error("MVRLinList::check: Listenkopf.prev ungleich NULL", FALSE);
	return;
    }

    for (f = old_f->next; f != NULL; f = f->next)
    {
	if (f->prev != old_f)
	{
	    error("MVRLinList::check: Rueckwaertsverkettung fehlerhaft", FALSE);
            return;
        }
	if (old_f->next != f)
	{
	    error("MVRLinList::check: Vorwaertsverkettung fehlerhaft", FALSE);
            return;
        }
	old_f = f;

	myanz ++;
	if (myanz > anz)
	{
	    sprintf(buffer, "MVRLinList::check: anz (%d != %d) passt nicht", myanz, anz);
	    error(buffer, FALSE);
	    return;
	}
    }

    if (old_f->next != NULL)
    {
	error("MVRLinList::check: Listenende.next ungleich NULL", FALSE);
	return;
    }

    if (last != old_f)
    {
	error("MVRLinList::check: last ungleich Listenende", FALSE);
	return;
    }

    if (myanz != anz)
    {
	sprintf(buffer, "MVRLinList::check: anz (%d != %d) passt nicht", myanz, anz);
	error(buffer, FALSE);
    }
}


////////////////////////////////////////////////////////////////////////
// SortedMVRLinList
////////////////////////////////////////////////////////////////////////

SortedMVRLinList::SortedMVRLinList()
    : MVRLinList()
{
    increasing = TRUE;
}



void SortedMVRLinList::set_sorting(bool _increasing)
{
    increasing = _increasing;
}


void SortedMVRLinList::check()
{
    SLink *sd;
    MVREntry *old = new MVREntry();
    bool valid;          // falls TRUE, ist old mit einem Wert besetzt

    // Verkettung konsistent ?
    MVRLinList::check();

    // Sortierung korrekt ?
    valid = FALSE;
    for (sd = first; sd != NULL; sd = sd->next)
    {
	 if (valid && (sd->d->son) < old->son)
         {
	     error("MVRLinList::check: Liste unsortiert", FALSE);
	     return;
         }
	 valid = TRUE;
	 old = sd->d;
    }
}


void SortedMVRLinList::insert(MVREntry *f)
{
    SLink *sd;

    // Rahmen fuer neues Datenelement erzeugen
    sd = new SLink;
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



void SortedMVRLinList::sort(bool _increasing)
// Bubblesort der ganzen Liste
{
    SLink *old_akt;
    MVREntry *s;
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


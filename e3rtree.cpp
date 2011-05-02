#include "e3rtree.h"
#include "math.h"

//////////////////////////////////////////////////////////////////////////////
// implementation
//////////////////////////////////////////////////////////////////////////////
#define PERCENTFULL 0.40
  //This constant defines how much percent the nodes should be filled
#define LEASTTIMEENTRY 5
  //This specifies the least number of entries when the split is done on time
  //This is effective only when LEASTENTRYON = true
#define LEASTENTRYON false
#define TIMEFAVOR 1.0
#define FORCETIMEROOTSPLIT false


//////////////////////////////////////////////////////////////////////////////
// E3REntry
//////////////////////////////////////////////////////////////////////////////


E3REntry::E3REntry(int _dimension, E3RTree *rt) : AbsEntry()
{
    dimension = _dimension;
    my_tree = rt;
    bounces = new float[2*dimension];
    son_ptr = NULL;
    son = MININT;
	level=-1;
}

E3REntry::~E3REntry()
{
    delete [] bounces;
    if (son_ptr != NULL)
	    delete son_ptr;
}

bool E3REntry::check_valid()
{
	if (son==MININT || level==-1)
		return false;
	for (int i=0;i<(2*dimension);i+=2)
	{
		if (bounces[i]-bounces[i+1]>0)
			return false;
	}
	return true;
}

bool E3REntry::operator == (E3REntry &_d)
{
	if (son!=_d.son) return false;
	if (dimension!=_d.dimension) return false;
	for (int i=0;i<2*dimension;i++)
		if ((bounces[i]-_d.bounces[i])>FLOATZERO) return false;
	return true;
}

E3REntry& E3REntry::operator = (E3REntry &_d)
{
    dimension = _d.dimension;
    son = _d.son;
    son_ptr = _d.son_ptr;
    memcpy(bounces, _d.bounces, sizeof(float) * 2 * dimension);
    my_tree = _d.my_tree;
	level=_d.level;

    return *this;
}

SECTION E3REntry::section(float *mbr)
{
    bool inside;
    bool overlap;
    int i;

    overlap = TRUE;
    inside = TRUE;
    for (i = 0; i < dimension; i++)
    {
	if (mbr[2*i]     > bounces[2*i + 1] ||
	    mbr[2*i + 1] < bounces[2*i])
	    overlap = FALSE;
	if (mbr[2*i]     < bounces[2*i] ||
	    mbr[2*i + 1] > bounces[2*i + 1])
	    inside = FALSE;
    }
    if (inside)
	  return INSIDE;
    else if (overlap)
	  return OVERLAP;
    else
	  return S_NONE;
}

void E3REntry::read_from_buffer(char *buffer)
{
    int i;

    i = 2*dimension*sizeof(float);
    memcpy(bounces, buffer, i);
    memcpy(&son, &buffer[i], sizeof(int));
    i += sizeof(int);
}

void E3REntry::write_to_buffer(char *buffer)
{
    int i;

    i = 2*dimension*sizeof(float);
    memcpy(buffer, bounces, i);
    memcpy(&buffer[i], &son, sizeof(int));
    i += sizeof(int);
}

// 2*dimension floats + son int
int E3REntry::get_size()
{
    return 2*dimension * sizeof(float) + sizeof(int);
}


E3RTNode* E3REntry::get_son()
{
    if (son_ptr == NULL)
	    son_ptr = new E3RTNode(my_tree, son);

    return son_ptr;
}

bool E3REntry::section_circle(float *center, float radius)
{
 float r2;

 r2 = radius * radius;

 if ((r2 - MINDIST(center,bounces, E3RTREEDIMENSION)) < 1.0e-8)
	return TRUE;
 else
	return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
// E3RTNode
//////////////////////////////////////////////////////////////////////////////

int E3RTNode::split(float **mbr, int **distribution)
{
    bool lu;
    int i, j, k, l, s, n, m1, dist, split_axis;
    SortMbr *sml, *smu;
    float minmarg, marg, minover, mindead, dead, over,
	*rxmbr, *rymbr;

    // how much nodes are used?
    n = num_entries;

    // nodes have to be filled at least 40%
    m1 = (int) ((float)n * PERCENTFULL);

    // sort by lower value of their rectangles
    sml = new SortMbr[n];
    smu = new SortMbr[n];
    rxmbr = new float[2*dimension];
    rymbr = new float[2*dimension];

    // choose split axis
    minmarg = MAXREAL;
    for (i = 0; i < dimension; i++)
    // for each axis
    {
        for (j = 0; j < n; j++)
        {
            sml[j].index = smu[j].index = j;
            sml[j].dimension = smu[j].dimension = i;
            sml[j].mbr = smu[j].mbr = mbr[j];
        }

        // Sort by lower and upper value perpendicular axis_i
      	qsort(sml, n, sizeof(SortMbr), sort_lower_mbr);
        qsort(smu, n, sizeof(SortMbr), sort_upper_mbr);

        marg = 0.0;
        // for all possible distributions of sml
        for (k = 0; k < n - 2*m1 + 1; k++)
        {
            // now calculate margin of R1
			// initialize mbr of R1
			for (s = 0; s < 2*dimension; s += 2)
			{
				rxmbr[s] =    MAXREAL;
				rxmbr[s+1] = -MAXREAL;
			}
            for (l = 0; l < m1+k; l++)
            {
                // calculate mbr of R1
				for (s = 0; s < 2*dimension; s += 2)
				{
					rxmbr[s] =   min(rxmbr[s],   sml[l].mbr[s]);
					rxmbr[s+1] = max(rxmbr[s+1], sml[l].mbr[s+1]);
                }
            }
			//marg += margin(dimension, rxmbr);
			//****Coded changed by TAO Yufei*********
			//Here I change the choose_split algorithm to calculate the overall overlap along each
			//dimension in order to make the R*-tree much less sensitive to the scanling value chosen
			//for spatiotemporal data.
			//***************************************

			// now calculate margin of R2
			// initialize mbr of R2
			for (s = 0; s < 2*dimension; s += 2)
			{
				rymbr[s] =    MAXREAL;
				rymbr[s+1] = -MAXREAL;
			}
            for ( ; l < n; l++)
            {
                // calculate mbr of R1
				for (s = 0; s < 2*dimension; s += 2)
				{
					rymbr[s] =   min(rymbr[s],   sml[l].mbr[s]);
					rymbr[s+1] = max(rymbr[s+1], sml[l].mbr[s+1]);
                }
            }
			//marg += margin(dimension, rxmbr);
			marg += overlap(dimension, rxmbr, rymbr);
        }

        // for all possible distributions of smu
       	for (k = 0; k < n - 2*m1 + 1; k++)
        {
            // now calculate margin of R1
			// initialize mbr of R1
			for (s = 0; s < 2*dimension; s += 2)
			{
				rxmbr[s] =    MAXREAL;
				rxmbr[s+1] = -MAXREAL;
			}
            for (l = 0; l < m1+k; l++)
            {
                // calculate mbr of R1
				for (s = 0; s < 2*dimension; s += 2)
				{
					rxmbr[s] =   min(rxmbr[s],   smu[l].mbr[s]);
					rxmbr[s+1] = max(rxmbr[s+1], smu[l].mbr[s+1]);
                }
            }
			//marg += margin(dimension, rxmbr);

            // now calculate margin of R2
			// initialize mbr of R2
			for (s = 0; s < 2*dimension; s += 2)
			{
				rymbr[s] =    MAXREAL;
				rymbr[s+1] = -MAXREAL;
			}
            for ( ; l < n; l++)
            {
                // calculate mbr of R1
				for (s = 0; s < 2*dimension; s += 2)
				{
					rymbr[s] =   min(rymbr[s],   smu[l].mbr[s]);
					rymbr[s+1] = max(rymbr[s+1], smu[l].mbr[s+1]);
                }
            }
			//marg += margin(dimension, rxmbr);
			marg += overlap(dimension, rxmbr, rymbr);
        }

		//These lines favor split on time axis-----------
		if (i==2) 
			marg*=TIMEFAVOR;
		//-----------------------------------------------

        // actual margin better than optimum?
        if (marg < minmarg)
        {
            split_axis = i;
			(my_tree->splitcount[i])++;
            minmarg = marg;
        }

    }
	
    //lines added for testng--------------------------
	if (FORCETIMEROOTSPLIT && level==my_tree->root_ptr->level)
		split_axis=2;  //force to split according to time if it is root
	//------------------------------------------------

	//We try to improve this data structure by the following measurement
	//If the split_axis=2 (i.e. on the time dimension, then we allow the
	//node capacity to be less than 0.4
    if (split_axis==2 && LEASTENTRYON)
	  m1=LEASTTIMEENTRY;

    // Now we are to choose best distribution for split axis
    for (j = 0; j < n; j++)
    {
	sml[j].index = smu[j].index = j;
	sml[j].dimension = smu[j].dimension = split_axis;
	sml[j].mbr = smu[j].mbr = mbr[j];
    }

    // Sort by lower and upper value perpendicular split axis
    qsort(sml, n, sizeof(SortMbr), sort_lower_mbr);
    qsort(smu, n, sizeof(SortMbr), sort_upper_mbr);

    minover = MAXREAL;
    mindead = MAXREAL;

	
    // for all possible distributions of sml and snu
    for (k = 0; k < n - 2*m1 + 1; k++)
    {
		//Now we only allow filtering from high to bottom if split_axis=time
	  //if (split_axis!=2)
	  {
        // lower sort
		// now calculate overlap and deadspace of R1

		// initialize mbr of R1
        dead = 0.0;
  	    for (s = 0; s < 2*dimension; s += 2)
		{
	      rxmbr[s] =    MAXREAL;
	      rxmbr[s+1] = -MAXREAL;
		}
		for (l = 0; l < m1+k; l++)
		{
	      // calculate mbr of R1 (the first node after split)
	      for (s = 0; s < 2*dimension; s += 2)
		  {
		    rxmbr[s] =   min(rxmbr[s],   sml[l].mbr[s]);
			rxmbr[s+1] = max(rxmbr[s+1], sml[l].mbr[s+1]);
		  }
          dead -= area(dimension, sml[l].mbr);
		}
        dead += area(dimension, rxmbr);

		// now calculate margin of R2
		// initialize mbr of R2
		for (s = 0; s < 2*dimension; s += 2)
		{
			rymbr[s] =    MAXREAL;
       		rymbr[s+1] = -MAXREAL;
		}
		for ( ; l < n; l++)
		{
	    // calculate mbr of R1
	      for (s = 0; s < 2*dimension; s += 2)
		  {
		    rymbr[s] =   min(rymbr[s],   sml[l].mbr[s]);
			rymbr[s+1] = max(rymbr[s+1], sml[l].mbr[s+1]);
		  }
		  dead -= area(dimension, sml[l].mbr);
		}
		dead += area(dimension, rymbr);
		over = overlap(dimension, rxmbr, rymbr);

		if ((over < minover) ||
	       (over == minover) && dead < mindead)
        {
            minover = over;
            mindead = dead;
            dist = m1+k;
            lu = TRUE;
        }
	  }
	

		// upper sort
		// now calculate overlap and deadspace of R1
		// initialize mbr of R1
		dead = 0.0;
		for (s = 0; s < 2*dimension; s += 2)
		{
	      rxmbr[s] =    MAXREAL;
	      rxmbr[s+1] = -MAXREAL;
		}
		for (l = 0; l < m1+k; l++)
		{
	    // calculate mbr of R1
	      for (s = 0; s < 2*dimension; s += 2)
		  {
			rxmbr[s] =   min(rxmbr[s],   smu[l].mbr[s]);
			rxmbr[s+1] = max(rxmbr[s+1], smu[l].mbr[s+1]);
		  }
          dead -= area(dimension, smu[l].mbr);
		}
		dead += area(dimension, rxmbr);

		// now calculate margin of R2
		// initialize mbr of R2
		for (s = 0; s < 2*dimension; s += 2)
		{
	      rymbr[s] =    MAXREAL;
	      rymbr[s+1] = -MAXREAL;
		}
		for ( ; l < n; l++)
		{
	    // calculate mbr of R1
	      for (s = 0; s < 2*dimension; s += 2)
		  {
		    rymbr[s] =   min(rymbr[s],   smu[l].mbr[s]);
			rymbr[s+1] = max(rymbr[s+1], smu[l].mbr[s+1]);
		  }
          dead -= area(dimension, smu[l].mbr);
		}
		dead += area(dimension, rxmbr);
		over = overlap(dimension, rxmbr, rymbr);

		if ((over < minover) ||
	       (over == minover) && dead < mindead)
        {
            minover = over;
            mindead = dead;
            dist = m1+k;
            lu = FALSE;
        }
    }

    // calculate best distribution
    *distribution = new int[n];
    for (i = 0; i < n; i++)
    {
        if (lu)
            (*distribution)[i] = sml[i].index;
        else
            (*distribution)[i] = smu[i].index;
    }

    delete sml;
    delete smu;
    delete rxmbr;
    delete rymbr;


    if (split_axis==2)
	  return dist;
	else
	  return dist;
}

E3RTNode::E3RTNode(E3RTree *rt) : AbsRTNode()
// zugehoeriger Plattenblock muss erst erzeugt werden
{
    char *b;
    int header_size;
    E3REntry * d;
    int i;

    my_tree = rt;
    dimension = rt->dimension;
    num_entries = 0;

	if (dimension!=E3RTREEDIMENSION)
		error("Hey--dimension wrong in E3RTNode::E3RTNode!\n",true);

    // mal kurz einen Dateneintrag erzeugen und schauen, wie gross der wird..
    d = new E3REntry();

    // level + num_entries
    header_size = get_headersize();
    capacity = (rt->file->get_blocklength() - header_size) / d->get_size();
    delete d;

    // Eintraege erzeugen, das geht mit einem Trick, da C++ beim
    // Initialisieren von Objektarrays nur Defaultkonstruktoren kapiert.
    // Daher wird ueber globale Variablen die Information uebergeben.
    entries = new E3REntry[capacity];
    for (i=0; i<capacity;i++)
    {
        entries[i].dimension = E3RTREEDIMENSION;
        entries[i].my_tree = my_tree;
    }

    // neuen Plattenblock an File anhaengen
    b = new char[rt->file->get_blocklength()];
    block = rt->file->append_block(b);
    delete [] b;

    // Plattenblock muss auf jeden Fall neu geschrieben werden
    dirty = TRUE;
}

E3RTNode::E3RTNode(E3RTree *rt, int _block) : AbsRTNode ()
// zugehoeriger Plattenblock existiert schon
{
    char *b;
    int header_size;
    E3REntry * d;
    int i;

    my_tree = rt;
    dimension = rt->dimension;
	if (dimension!=E3RTREEDIMENSION)
		error("Hey--dimension wrong in E3RTNode::E3RTNode!\n",true);
    num_entries = 0;

    // mal kurz einen Dateneintrag erzeugen und schauen, wie gross der wird..
    d = new E3REntry();
    // von der Blocklaenge geht die Headergroesse ab
    header_size = get_headersize();
    capacity = (rt->file->get_blocklength() - header_size) / d->get_size();
    delete d;

    // Eintraege erzeugen, das geht mit einem Trick, da C++ beim
    // Initialisieren von Objektarrays nur Defaultkonstruktoren kapiert.
    // Daher wird ueber globale Variablen die Information uebergeben.
    entries = new E3REntry[capacity];
    for (i=0; i<capacity;i++)
    {
        entries[i].dimension = E3RTREEDIMENSION;
        entries[i].my_tree = my_tree;
    }

    // zugehoerigen Plattenblock holen und Daten einlesen
    // dies kann nicht in E3RTNode::E3RTNode(..) passieren, weil
    // beim Aufruf des Basisklassenkonstruktors E3RTNode noch
    // gar nicht konstruiert ist, also auch keine Daten aufnehmen kann
    block = _block;
    b = new char[rt->file->get_blocklength()];
    if (rt->cache == NULL) // no cache
        rt->file->read_block(b, block);
    else
        rt->cache->read_block(b, block, rt);

    read_from_buffer(b);
    delete [] b;

    // Plattenblock muss vorerst nicht geschrieben werden
    dirty = FALSE;
}

E3RTNode::~E3RTNode()
{
    char *b;

    if (dirty)
    {
        // Daten auf zugehoerigen Plattenblock schreiben
        b = new char[my_tree->file->get_blocklength()];
        write_to_buffer(b);
        if (my_tree->cache == NULL) // no cache
            my_tree->file->write_block(b, block);
        else
            my_tree->cache->write_block(b, block, my_tree);

        delete [] b;
    }

    delete [] entries;
}

void E3RTNode::read_from_buffer(char *buffer)
{
    int i, j, s;

    // Level
    memcpy(&level, buffer, sizeof(char));
    j = sizeof(char);

    // num_entries
    memcpy(&num_entries, &(buffer[j]), sizeof(int));
    j += sizeof(int);

    s = entries[0].get_size();
    for (i = 0; i < num_entries; i++)
    {
    	entries[i].read_from_buffer(&buffer[j]);
    	j += s;
    }
}

void E3RTNode::write_to_buffer(char *buffer)
{
    int i, j, s;

    // Level
    memcpy(buffer, &level, sizeof(char));
    j = sizeof(char);

    // num_entries
    memcpy(&buffer[j], &num_entries, sizeof(int));
    j += sizeof(int);

    s = entries[0].get_size();
    for (i = 0; i < num_entries; i++)
    {
    	entries[i].write_to_buffer(&buffer[j]);
       	j += s;
    }
}

void E3RTNode::print()
{
    int i;

    for (i = 0; i < num_entries ; i++)
    {
        printf("(%4.1lf, %4.1lf, %4.1lf, %4.1lf)\n",
	       entries[i].bounces[0],
	       entries[i].bounces[1],
	       entries[i].bounces[2],
	       entries[i].bounces[3]);
    }
    printf("level %d\n", level);
}

int E3RTNode::get_num_of_data()
{
    int i, sum;
    E3RTNode* succ;

    if (level == 0)
        return num_entries;

    sum = 0;
    for (i = 0; i < num_entries ; i++)
    {
        succ = entries[i].get_son();
        sum += succ->get_num_of_data();
    }

    return sum;
}

float* E3RTNode::get_mbr()
{
    int i, j;
    float *mbr;

    mbr = new float[2*dimension];
    for (i = 0; i < 2*dimension; i ++ )
        mbr[i] = entries[0].bounces[i];

    for (j = 1; j < num_entries; j++)
    {
    	for (i = 0; i < 2*dimension; i += 2)
    	{
    	    mbr[i]   = min(mbr[i],   entries[j].bounces[i]);
    	    mbr[i+1] = max(mbr[i+1], entries[j].bounces[i+1]);
        }
    }

    return mbr;
}

void E3RTNode::enter(E3REntry *de)
{
    // ist ein Einfuegen ueberhaupt moeglich?
    if (num_entries > (capacity-1))
        error("E3RTNode::enter: called, but node is full", TRUE);

    // Eintrag an erste freie Stelle kopieren
    entries[num_entries] = *de;

    // jetzt gibts einen mehr
    num_entries++;

    // Eintrag freigeben, aber keine Nachfolgeknoten loeschen !!!!
    de->son_ptr = NULL;
    delete de;
}

void E3RTNode::split(E3RTNode *sn)
// splittet den aktuellen Knoten so auf, dass m mbr's nach sn verschoben
// werden
{
    int i, *distribution, dist, n;
    float **mbr_array;
    E3REntry *new_entries1, *new_entries2;

#ifdef SHOWMBR
	split_000++;
#endif

    // wieviele sind denn nun belegt?
    n = num_entries;

    // mbr_array allokieren und belegen
    mbr_array = new floatptr[n];
    for (i = 0; i < n; i++)
       	mbr_array[i] = entries[i].bounces;

    // Verteilung berechnen
    dist = split(mbr_array, &distribution);

    // neues Datenarray erzeugen
    // --> siehe Konstruktor
    new_entries1 = new E3REntry[capacity];
    new_entries2 = new E3REntry[capacity];

    for (i = 0; i < dist; i++)
    {
       	new_entries1[i] = entries[distribution[i]];
    }

    for (i = dist; i < n; i++)
    {
       	new_entries2[i-dist] = entries[distribution[i]];
    }

    // Datenarrays freigeben
    // da die Nachfolgeknoten aber nicht geloescht werden sollen
    // werden vor dem Aufruf von delete noch alle Pointer auf NULL gesetzt
    for (i = 0; i < n; i++)
    {
       	entries[i].son_ptr = NULL;
       	sn->entries[i].son_ptr = NULL;
    }
    delete [] entries;
    delete [] sn->entries;

    // neue Datenarrays kopieren
    entries = new_entries1;
    sn->entries = new_entries2;

    // Anzahl der Eintraege berichtigen
    num_entries = dist;
    sn->num_entries = n - dist;  // muss wegen Rundung so bleiben !!

    // delete mbr_array
	/*
    for (i = 0; i < n; i++)
    	delete [] mbr_array[i];*/
	//Commented by TAO yufei.
	//Error otherwise.

    delete [] mbr_array;
}

/*
int E3RTNode::choose_subtree(float *mbr)
//Note: This is another version of choosing the subtree, which is based
//on the distance measurement proposed in "Efficient Index Structures for
//Spatio-Temporal Objects".  Be reminded that scales are necessary and
//essential for this method to work efficiently
{
	int follow=-1;
	float mindist=1e9;
	for (int i=0;i<num_entries;i++)
	{
		float *midp1,*midp2;
		midp1=new float[DIMENSION];
		midp2=new float[DIMENSION];
		int j;
		for (j=0;j<DIMENSION;j++)
		{
			midp1[j]=(entries[i].bounces[2*j]+entries[i].bounces[2*j+1])/2;
			midp2[j]=(mbr[2*j]+mbr[2*j+1])/2;
		}
		float dist=0;
		for (j=0;j<DIMENSION;j++)
		{
			float temp=(midp1[j]-midp2[j]);
			dist+=temp*temp;
		}
		dist=sqrt(dist);
		if (dist<mindist)
		{
			mindist=dist; follow=i;
		}
		delete [] midp1;
		midp1=NULL;
		delete [] midp2;
		midp2=NULL;
	}
	return follow;
}
*/

///*
int E3RTNode::choose_subtree(float *mbr)
{
    int i, j, follow, minindex, *inside, inside_count, *over;
    float *bmbr, old_o, o, omin, a, amin, f, fmin;

    // faellt d in einen bestehenden Eintrag ?
    inside_count = 0;
    inside = new int[num_entries];
    over = new int[num_entries];
    for (i = 0; i < num_entries; i++)
    {
    	switch (entries[i].section(mbr))
    	{
        	case INSIDE:
        	    inside[inside_count++] = i;
        	    break;
        }
    }

    if (inside_count == 1)
        // Case 1: There is exactly one dir_mbr that contains mbr
        // Fall 1: Rechteck faellt genau in einen Eintrag
    	follow = inside[0];
    else if (inside_count > 1)
    // Case 2: There are many dir_mbrs that contain mbr
    // choose the one with the minimum area
    // Fall 2: Rechteck faellt in mehrere Eintrag --> nimm den
    // mit der geringsten Flaeche (volumen!!!)
    {
    	fmin = MAXREAL;
    	for (i = 0; i < inside_count; i++)
    	{
    	    f = area(dimension, entries[inside[i]].bounces);
    	    if (f < fmin)
    	    {
    	    	minindex = i;
          		fmin = f;
       	    }
       	}
    	follow = inside[minindex];
    }
    else
    // Case 3: There are no dir_mbrs that contain mbr
    // choose the one for which insertion causes the minimun overlap if son_is_data
    // else choose the one for which insertion causes the minimun area enlargement

    // Fall 3: Rechteck faellt in keinen Eintrag -->
    // fuer Knoten, die auf interne Knoten zeigen:
    // nimm den Eintrag, der am geringsten vergroessert wird;
    // bei gleicher Vergroesserung:
    // nimm den Eintrag, der die geringste Flaeche hat
    //
    // fuer Knoten, die auf Datenknoten zeigen:
    // nimm den, der die geringste Ueberlappung verursacht
    // bei gleicher Ueberlappung:
    // nimm den Eintrag, der am geringsten vergroessert wird;
    // bei gleicher Vergroesserung:
    // nimm den Eintrag, der die geringste Flaeche hat
    {
       	if (level == 1) // son_is_data
    	{
            omin = MAXREAL;
    	    fmin = MAXREAL;
    	    amin = MAXREAL;
    	    for (i = 0; i < num_entries; i++)
    	    {
        		// berechne die mbr, wenn mbr in entries[i] eingefuegt wird
        		enlarge(dimension, &bmbr, mbr, entries[i].bounces);

        		// calculate area and area enlargement
        		a = area(dimension, entries[i].bounces);
        		f = area(dimension, bmbr) - a;

        		// calculate overlap before enlarging entry_i
        		old_o = o = 0.0;
        		for (j = 0; j < num_entries; j++)
        		{
        		    if (j != i)
        		    {
    			        old_o += overlap(dimension,
    					 entries[i].bounces,
    					 entries[j].bounces);
    			        o += overlap(dimension,
    				     bmbr,
    				     entries[j].bounces);
    		        }
    	        }
    	        o -= old_o;

    	        // is this entry better than the former optimum ?
    	        if ((o < omin) ||
    		    (o == omin && f < fmin) ||
    		    (o == omin && f == fmin && a < amin))
    	        {
    	       	    minindex = i;
        		    omin = o;
        		    fmin = f;
        		    amin = a;
        	    }
    	        delete [] bmbr;
    	    }
        }
        else // son is not a data node
        {
    	    fmin = MAXREAL;
    	    amin = MAXREAL;
    	    for (i = 0; i < num_entries; i++)
    	    {
    	        // berechne die mbr, wenn mbr in entries[i] eingefuegt wird
    	        enlarge(dimension, &bmbr, mbr, entries[i].bounces);

    	        // calculate area and area enlargement
    	        a = area(dimension, entries[i].bounces);
    	        f = area(dimension, bmbr) - a;

    	        // is this entry better than the former optimum ?
    	        if ((f < fmin) || (f == fmin && a < amin))
    	        {
    	       	    minindex = i;
    		        fmin = f;
    	            amin = a;
    	        }
	            delete [] bmbr;
	        }
        }

    	// berechne die neue mbr und schreib sie in den Eintrag
    	enlarge(dimension, &bmbr, mbr, entries[minindex].bounces);
    	memcpy(entries[minindex].bounces, bmbr, dimension * 2 * sizeof(float));
    	follow = minindex;
    	delete [] bmbr;

    	// leider muss jetzt der Block auch geschrieben werden
    	dirty = TRUE;
    }

    delete [] inside;
    delete [] over;

    return follow;
}
//*/


R_OVERFLOW E3RTNode::insert(E3REntry *d, E3RTNode **sn)
{
	if ((d->check_valid())==false)
		error("E3RTNode::insert--Something wrong with entry d",true);
    int follow;
    E3RTNode *succ, *new_succ;
    E3RTNode *brother;
    E3REntry *de;
    R_OVERFLOW ret;
    float *mbr,*nmbr;

    int i, last_cand;
    float *center;
    SortMbr *sm;
    E3REntry *nd, *new_entries;

    if (level>0) // direcrtory node
    {
	  if (level>d->level)
	  {
        follow = choose_subtree(d->bounces);

        // Sohn laden
        succ = entries[follow].get_son();

        // insert d into son
        ret = succ->insert(d, &new_succ);
        //if (ret != NONE)
        // if anything happend --> update bounces of entry "follow"
        {
            mbr = succ->get_mbr();
            memcpy(entries[follow].bounces, mbr, sizeof(float) * 2*dimension);
            delete [] mbr;
        }

        if (ret == SPLIT)
        // node has split into itself and *new_succ
        {
            if (num_entries == capacity)
         	    error("E3RTNode::insert: maximum capacity violation", TRUE);

             // create and insert new entry
            de = new E3REntry(dimension, my_tree);
    	    nmbr = new_succ->get_mbr();
            memcpy(de->bounces, nmbr, 2*dimension*sizeof(float));
    	    delete [] nmbr;
            de->son = new_succ->block;
            de->son_ptr = new_succ;
            enter(de);

            if (num_entries == (capacity - 1))
            // maximun no of entries --> Split
            // this happens already if the node is nearly filled
            // for the algorithms are more easy then
            {
        	    brother = new E3RTNode(my_tree);
        	    my_tree->num_of_inodes++;
        	    brother->level = level;
        	    split(brother);
                    *sn = brother;
        	    //printf("ddddddddiiiiiiirr: splitting node %d, creating %d\n", block, (*sn)->block);

                ret = SPLIT;
        	}
            else
          	    ret = NONE;
        }
        // must write page
        dirty = TRUE;

        return ret;
	  }
	  else //level==d->level
	  {
		  entries[num_entries]=*d;
		  num_entries++;
		  if (num_entries == (capacity - 1))
            // maximun no of entries --> Split
            // this happens already if the node is nearly filled
            // for the algorithms are more easy then
		  {
        	brother = new E3RTNode(my_tree);
        	my_tree->num_of_inodes++;
        	brother->level = level;
        	split(brother);
            *sn = brother;
        	//printf("ddddddddiiiiiiirr: splitting node %d, creating %d\n", block, (*sn)->block);

            ret = SPLIT;
		  }
          else
          	ret = NONE;

		  dirty=true;
		  return ret;
	  }	
    }
    else // data (leaf) node
    {
        if (num_entries == capacity)
        	error("RTDataNode::insert: maximum capacity violation", TRUE);

        entries[num_entries] = *d;
        num_entries++;

        // Plattenblock zum Schreiben markieren
        dirty = TRUE;

        if (num_entries == (capacity - 1))
        // maximum # of entries --> Split
        // this happens already if the node is nearly filled
        // for the algorithms are more easy then
        {
			//if (my_tree->re_level[0] == FALSE)
            if (my_tree->re_level[0] == FALSE && my_tree->root_ptr->level>0)
    	    // there was no reinsert on level 0 during this insertion
			//-------------------------------------------------------
			//Here I changed the condition as if it is already root, no need
			//to reinsert.  Split directly in this case
			//-----------By TAO Yufei
            {
                // calculate center of page
                mbr = get_mbr();
                center = new float[dimension];
                for (i = 0; i < dimension; i++)
                     center[i] = (mbr[2*i] + mbr[2*i+1]) / 2.0;

                // neues Datenarray erzeugen
                // --> siehe Konstruktor
                new_entries = new E3REntry[capacity];

        	    // Indexarray aufbauen und initialisieren
        	    sm = new SortMbr[num_entries];
        	    for (i = 0; i < num_entries; i++)
        	    {
            		sm[i].index = i;
            		sm[i].dimension = dimension;
            		sm[i].mbr = entries[i].bounces;
            		sm[i].center = center;
                }

                // sort by distan				ce of each center to the overall center
                qsort(sm, num_entries, sizeof(SortMbr), sort_center_mbr);

                last_cand = (int) ((float)num_entries * 0.30);

                // copy the nearest candidates to new array
                for (i = 0; i < num_entries - last_cand; i++)
    	            new_entries[i] = entries[sm[i].index];

                // insert candidates into reinsertion list
                for ( ; i < num_entries; i++)
                {
                    nd = new E3REntry();
                    *nd = entries[sm[i].index];
					nd->level=0;
                    my_tree->re_data_cands->insert(nd);
                }

                // free and copy data array
                delete [] entries;
        	    entries = new_entries;
				
				/*
               	for (i = 0; i < num_entries; i++)
				{
					delete [] sm[i].mbr;
				}*/
				    //Commented by TAO Yufei-----
					//Because entries have been deleted, so are the bounces
					//entries in them.  Therefore, in the original code, an
					//error will be generated (note that sm[i].mbr points to
					//the bounces entry).

        	    delete sm;
        	    delete [] mbr;
        	    delete [] center;
        	    my_tree->re_level[0] = TRUE;

        	    // correct # of entries
        	    num_entries -= last_cand;

        	    // must write page
        	    dirty = TRUE;

                return REINSERT;
        	}
           	else
           	{
        	    *sn = new E3RTNode(my_tree);
        	    (*sn)->level = level;
        	    my_tree->num_of_dnodes++;
        	    split((E3RTNode *) *sn);
        	    //printf("splitting node %d, creating %d\n", block, (*sn)->block);
    	    }
    	    return SPLIT;
        }
        else
            return NONE;
    }
}

void E3RTNode::rangeQuery(float *mbr, SortedE3RLinList *res, char *rec, int *levelcount)
//This function does the range query (given by mbr) on the current node.
//levelcount will give the nodes visited at each level.  
{
    int i, n;
    SECTION s;
    E3RTNode *succ;
    E3REntry *copy;

	levelcount[level]++;
    n = num_entries;
	int count=0;
    for (i = 0; i < n; i++)
    {
		float newmbr[2*E3RTREEDIMENSION];
		memcpy(newmbr,mbr,2*E3RTREEDIMENSION*sizeof(float));
		s = entries[i].section(newmbr);
        if (s == INSIDE || s == OVERLAP)
        {
			count++;
            if (level == 0)
            {
				if (rec==NULL)
				{
                  copy = new E3REntry();
        		  *copy = entries[i];
				  copy->level=0;
        		  res->insert(copy);
				}
				else
				if (rec[abs(entries[i].son)]==0 || entries[i].son<0)
					//Note that here we add the condition entries[i].son<0
	//Recall that son could be negative to indicate that there is no need to
	//search MVR-tree.  The extra condition along with the following (**) condition
	//helps to avoid the bit be cleared by a negative son.  One might argue that
	//this only guarantees no duplicate nodes visited but entries with the same son
	//could be added many times.  However, this is not a problem since we will have
	//another measurement later to assure this in E3RTimeQuery function in "st.h".
				{
				  copy = new E3REntry();
        		  *copy = entries[i];
				  copy->level=0;
        		  res->insert(copy);
				  if (entries[i].son>=0)  //(**)
					  rec[abs(entries[i].son)]=1;
				}
            }
            else
            {
                succ = entries[i].get_son();
                succ->rangeQuery(mbr,res, rec, levelcount);
            }
        }
    }
}

R_DELETE E3RTNode::delete_entry(E3REntry *e)
{
	if (e->check_valid()==false)
		error("E3RTNode::delete_entry()--Something wrong with entry e\n",true);
	E3RTNode *succ;
	float *tmp;
	if (level>0)
	{
		if (this==my_tree->root_ptr)
			//i.e. this is the root
		{
			for (int i=0;i<num_entries;i++)
			{
				tmp=overlapRect(dimension,entries[i].bounces,e->bounces);
				if (tmp!=NULL)
				{
					delete tmp;
					succ=entries[i].get_son();
					R_DELETE del_ret;
					del_ret=succ->delete_entry(e);
					if (del_ret!=NOTFOUND)
					{
						switch (del_ret)
						{
						case NORMAL:
							float *mbr;
							mbr=succ->get_mbr();
							memcpy(entries[i].bounces,mbr,sizeof(float)*2*dimension);
							dirty=true;
							delete mbr;
							return NORMAL;
							break;
						case UNDERFLOW:
							for (int j=i;j<num_entries-1;j++)
								entries[j]=entries[j+1];
							num_entries--;
							dirty=true;
							return NORMAL;
							break;
						}
					}
				}
			}
			//Not found;
			return NOTFOUND;
		}
		else//is not root and not leaf
		{
			for (int i=0;i<num_entries;i++)
			{
				tmp=overlapRect(dimension,entries[i].bounces,e->bounces);
				if (tmp!=NULL)
				{
					delete tmp;
					succ=entries[i].get_son();
					R_DELETE del_ret;
					del_ret=succ->delete_entry(e);
					if (del_ret!=NOTFOUND)
					{
						switch (del_ret)
						{
						case NORMAL:
							float *mbr;
							mbr=succ->get_mbr();
							memcpy(entries[i].bounces,mbr,sizeof(float)*2*dimension);
							dirty=true;
							delete mbr;
							return NORMAL;
							break;
						case UNDERFLOW:
							for (int j=i;j<num_entries-1;j++)
								entries[j]=entries[j+1];
							num_entries--;
							dirty=true;
							delete succ;
							if (level>1)
								my_tree->num_of_inodes--;
							else
								my_tree->num_of_dnodes--;

							if (num_entries<PERCENTFULL*capacity)
							{
								for (int j=0;j<num_entries;j++)
								{
									E3REntry *e;
									e=new E3REntry();
									(*e)=entries[j];
									e->level=level;
									my_tree->deletelist->insert(e);
								}
								return WUNDERFLOW;
							}
							else
								return NORMAL;
							break;
						}
					}
				}
			}
		}
	}
	else//it's a leaf
	{
		for (int i=0;i<num_entries;i++)
		{
			if (entries[i]==(*e))
			{
				for (int j=i;j<num_entries-1;j++)
				{
					entries[j]=entries[j+1];
				}
				num_entries--;
				dirty=true;
				if (this!=my_tree->root_ptr && num_entries<PERCENTFULL*capacity)
				{
					for (int k=0;k<num_entries;k++)
					{
						E3REntry *en;
					    en=new E3REntry();
						(*en)=entries[k];
						en->level=0;
						my_tree->deletelist->insert(en);
					}
					return WUNDERFLOW;
				}
				else
				{
					return NORMAL;
				}
			}
		}
		return NOTFOUND;
	}
	return NOTFOUND;// by me
}

bool E3RTNode::FindLeaf(E3REntry *e)
{
	E3RTNode *succ;
	if (level>0)
	{
		for (int i=0;i<num_entries;i++)
		{
			float *f;
			f=overlapRect(my_tree->dimension,
				  entries[i].bounces,e->bounces);
			if (f!=NULL)
			{
				delete f;
				succ=entries[i].get_son();
				bool find;
				find=succ->FindLeaf(e);
				return true;
			}
		}
		return false;
	}
	else
	{
		for (int i=0;i<num_entries;i++)
		{
			if (entries[i]==(*e))
				return true;
		}
		return false;
	}
	return false;
}




//////////////////////////////////////////////////////////////////////////////
// E3RTree
//////////////////////////////////////////////////////////////////////////////

E3RTree::E3RTree(char *fname, int _b_length, Cache *c, int _dimension) : AbsRTree()
//This constructor constructs a new E3RTree, but only builds an empty one
//No object is inserted.  To create an E3RTree from an input txt file,
//use another constructor.
{
    file = new BlockFile(fname, _b_length);
    cache = c;

    re_data_cands = new E3RLinList();

    // header allokieren und lesen
    header = new char [file->get_blocklength()];

    //read_header(header);
	   //Commented by TAO Yufei.
	   //I comment this out as header has not been read from disk.  In fact
	   //users call this constructor when they want to create an entirely new
	   //tree.  Therefore, no need to read header, but simply set prop values
	   //directly.

    dimension = _dimension;
    root = 0;
    root_ptr = NULL;
    root_is_data = TRUE;
    num_of_data = num_of_inodes = num_of_dnodes = 0;

    root_ptr = new E3RTNode(this);
    num_of_dnodes++;
    root_ptr->level = 0;

    root = root_ptr->block;
	splitcount=new int[E3RTREEDIMENSION];	
	for (int i=0;i<E3RTREEDIMENSION;i++)
		splitcount[i]=0;
}


E3RTree::E3RTree(char *fname, Cache *c) : AbsRTree()
//This constructor rebuilds the E3RTree from the file
{
    int j;

    // file existiert schon --> Blockgroesse wird im Konstruktor ignoriert
    file = new BlockFile(fname, 0);
    cache =c;

    re_data_cands = new E3RLinList();

    // header allokieren und lesen
    header = new char [file->get_blocklength()];
    file->read_header(header);

    read_header(header);

    root_ptr = NULL;

    //if (num_of_data == 0)
	if (num_of_data == 0 && num_of_dnodes==0)
		//I changed the condition to differ the case where the tree
		//is entirely empty (even without root), and the case where
		//the tree has root but contains no entries (this case no need
		//to create root
		//---------By TAO Yufei
    // Baum war leer -> Datenknoten anlegen und d einfuegen
    {
    	root_ptr = new E3RTNode(this);
    	root = root_ptr->block;
        num_of_dnodes++;
    	root_ptr->level = 0;
    }
    else
	    load_root();

	splitcount=new int[E3RTREEDIMENSION];
	for (int i=0;i<E3RTREEDIMENSION;i++)
		splitcount[i]=0;
}

E3RTree::E3RTree(char *inpname, char *fname, int _b_length, Cache *c, int _dimension) : AbsRTree()
// construct new R-tree from a specified input textfile with rectangles
{
    long id;
    float x0,y0,x1,y1;
    E3REntry *d;
    FILE *fp;
    file = new BlockFile(fname, _b_length);
    cache =c;

    re_data_cands = new E3RLinList();

    // header allokieren und lesen
    header = new char [file->get_blocklength()];

    //read_header(header);

    dimension = _dimension;
    root = 0;
    root_ptr = NULL;
    root_is_data = TRUE;
    num_of_data = num_of_inodes = num_of_dnodes = 0;

    root_ptr = new E3RTNode(this);
    num_of_dnodes++;
    root_ptr->level = 0;
    root = root_ptr->block;

	

    if((fp=fopen(inpname,"r"))==NULL)
    {
      delete this;
      error("Cannot open R-Tree text file", TRUE);
    }
    else
    // unbekanntes File
    {
      while (!feof(fp))
      {
    	fscanf(fp, "%d %f %f %f %f \n", &id, &x0, &y0, &x1, &y1);
    	d = new E3REntry();
    	d->son = id;
		d->level=0;
    	d->bounces[0] = x0;
    	d->bounces[1] = x1;
    	d->bounces[2] = y0;
    	d->bounces[3] = y1;

    	insert(d);
    	delete d;
		
      }
    }

	splitcount=new int[E3RTREEDIMENSION];
	for (int i=0;i<E3RTREEDIMENSION;i++)
		splitcount[i]=0;
}

E3RTree::~E3RTree()
{
    int j;

    write_header(header);

    file->set_header(header);
    delete [] header;
	header=NULL;

    if (root_ptr != NULL)
    {
        delete root_ptr;
        root_ptr = NULL;
    }

	if (cache)
		delete cache;

	int bsize=file->get_blocklength();

    delete file;
	file=NULL;
    delete re_data_cands;
	re_data_cands=NULL;
	
    //printf("This R-Tree contains %d internal, %d data nodes and %d data\n",
	//   num_of_inodes, num_of_dnodes, num_of_data);
	//printf("The size of this R-tree is %f bytes\n",(num_of_inodes+num_of_dnodes)*(float)bsize/1000000.0);
	printf ("The following is the split times along each dimension:\n");
	for (j=0;j<E3RTREEDIMENSION;j++)
		printf("%d\n",splitcount[j]);
	delete [] splitcount;
	splitcount=NULL;
		  
}

void E3RTree::read_header(char *buffer)
{
    int i=0;

    memcpy(&dimension, buffer, sizeof(dimension));
    i = sizeof(dimension);

    memcpy(&num_of_data, &buffer[i], sizeof(num_of_data));
    i += sizeof(num_of_data);

    memcpy(&num_of_dnodes, &buffer[i], sizeof(num_of_dnodes));
    i += sizeof(num_of_dnodes);

    memcpy(&num_of_inodes, &buffer[i], sizeof(num_of_inodes));
    i += sizeof(num_of_inodes);

    memcpy(&root_is_data, &buffer[i], sizeof(root_is_data));
    i += sizeof(root_is_data);

    memcpy(&root, &buffer[i], sizeof(root));
    i += sizeof(root);

	
}



void E3RTree::write_header(char *buffer)
{
    int i=0;

    memcpy(buffer, &dimension, sizeof(dimension));
    i = sizeof(dimension);

    memcpy(&buffer[i], &num_of_data, sizeof(num_of_data));
    i += sizeof(num_of_data);

    memcpy(&buffer[i], &num_of_dnodes, sizeof(num_of_dnodes));
    i += sizeof(num_of_dnodes);

    memcpy(&buffer[i], &num_of_inodes, sizeof(num_of_inodes));
    i += sizeof(num_of_inodes);

    memcpy(&buffer[i], &root_is_data, sizeof(root_is_data));
    i += sizeof(root_is_data);

    memcpy(&buffer[i], &root, sizeof(root));
    i += sizeof(root);

	
}

void E3RTree::load_root()
{
    if (root_ptr == NULL)
    // Baum ist nicht leer und root nicht geladen -> root laden
	    root_ptr = new E3RTNode(this, root);
}

void E3RTree::insert(E3REntry* d)
{
    int i, j;
    E3RTNode *sn;
    E3RTNode *nroot_ptr;
    int nroot;
    E3REntry *de;
    R_OVERFLOW split_root;
    E3REntry *d_cand, *dc;
    float *nmbr;

    // load root into memory
    load_root();

    // no overflow occured until now
    re_level = new bool[root_ptr->level+1];
    for (i = 0; i <= root_ptr->level; i++)
        re_level[i] = FALSE;

    // insert d into re_data_cands as the first entry to insert
    // make a copy of d because it shouldnt be erased later
    dc = new E3REntry();
    *dc = *d;
    re_data_cands->insert(dc);

    j = -1;
    while (re_data_cands->get_num() > 0)
    {
        // first try to insert data, then directory entries
	    d_cand = (E3REntry *)re_data_cands->get_first();
        if (d_cand != NULL)
        {
            // since erase deletes the according data element of the
            // list, we should make a copy of the data before
            // erasing it
            dc = new E3REntry();
            *dc = *d_cand;
            re_data_cands->erase();

            // start rekursive insert with root
			
            split_root = root_ptr->insert(dc, &sn);
    	    delete dc;
        }
        else
	        error("E3RTree::insert: inconsistent list re_data_cands", TRUE);

    	if (split_root == SPLIT)
    	// insert has lead to split --> new root-page having two sons
    	// root and sn
    	{
    	    nroot_ptr = new E3RTNode(this);
    	    nroot_ptr->level = root_ptr->level + 1;
    	    num_of_inodes++;
    	    nroot = nroot_ptr->block;

    	    // mbr der alten root page holen und in neue einfuegen
    	    de = new E3REntry(dimension, this);
    	    nmbr = root_ptr->get_mbr();
    	    memcpy(de->bounces, nmbr, 2*dimension*sizeof(float));
    	    delete [] nmbr;
    	    de->son = root_ptr->block;
    	    de->son_ptr = root_ptr;
    	    nroot_ptr->enter(de);

    	    // mbr der gesplitteten page holen und in neue einfuegen
    	    de = new E3REntry(dimension, this);
    	    nmbr = sn->get_mbr();
    	    memcpy(de->bounces, nmbr, 2*dimension*sizeof(float));
    	    delete [] nmbr;
    	    de->son = sn->block;
    	    de->son_ptr = sn;
    	    nroot_ptr->enter(de);

    	    root = nroot;
            root_ptr = nroot_ptr;

            root_is_data = FALSE;
        }
        j++;
    }

    // jetzt ist der Baum um eines groesser
    num_of_data++;

    // free lists
    delete [] re_level;

    // write nodes
/*
    delete root_ptr;
    root_ptr = NULL;
*/
}

void E3RTree::rangeQuery(float *mbr, SortedE3RLinList *res, char *rec, int reclen, int *levelcount)
{
    load_root();
	for (int i=0;i<reclen;i++)
		rec[i]=0;
	//for (int i=0;i<=root_ptr->level;i++)
	//	levelcount[i]=0;
	//Note that we leave the cleansing work outside this function.  This is because we need
	//to do more than 1 query.
    root_ptr->rangeQuery(mbr,res,rec,levelcount);
	delete root_ptr;
	root_ptr=NULL;
}

bool E3RTree::delete_entry(E3REntry *d)
{
	int olddnum=num_of_data;
	load_root();
	deletelist=new E3RLinList();
	R_DELETE del_ret;
	del_ret=root_ptr->delete_entry(d);
	if (del_ret==NOTFOUND) return false;
	if (del_ret==WUNDERFLOW) 
		error("E3RTree::delete_entry--Something wrong\n",true);
	//Check if we need shorten the tree;
	if (root_ptr->level>0 && root_ptr->num_entries==1)
	{
		root=root_ptr->entries[0].son;
		delete root_ptr;
		root_ptr=NULL;
		load_root();
		num_of_inodes--;
	}
	//Now will reinsert the entries
	E3REntry *e;
	e=(E3REntry *)deletelist->get_first();
	while (e!=NULL)
	{
		insert(e);
		e=(E3REntry *)deletelist->get_next();
	}
	delete deletelist;

	num_of_data=olddnum-1;
	return true;
}

bool E3RTree::FindLeaf(E3REntry *e)
{
	load_root();
	return root_ptr->FindLeaf(e);
}

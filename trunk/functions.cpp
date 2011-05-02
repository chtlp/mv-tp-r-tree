#include <math.h>
#include <stdio.h>
#include "rtreeheader.h"
#include "para.h"

#define CHECK_MBR

//////////////////////////////////////////////////////////////////////////////
// globals
//////////////////////////////////////////////////////////////////////////////

int roundup(float num)
{
	int res=(int)num;
	if ((float)res<num) res++;
	return res;
}

void error(char *t, bool ex)
{
	fprintf(stderr, t);
	if (ex)
		exit(0);
}


//////////////////////////////////////////////////////////////////////////////
// C-functions
//////////////////////////////////////////////////////////////////////////////
float area(int dimension, float *mbr)
	// berechnet Flaeche (Volumen) eines mbr der Dimension dimension
{
	int i;
	float sum;

	sum = 1.0;
		for (i = 0; i < dimension; i++)
			sum *= mbr[2*i+1] - mbr[2*i];


	return sum;
}

float area(int dimension, float *mbr, float *velocity)
{
	int i;
	float sum;

	sum = 1.0;
	for (i = 0; i < dimension; i++) {
		if (PREDICT_TIME < TRIGGER_TIME)
		sum *= mbr[2*i+1] - mbr[2*i] + velocity[4*i] * PREDICT_TIME + velocity[4*i+1] * PREDICT_TIME;
		else {
			float w = (velocity[4*i] + velocity[4*i+1]) * TRIGGER_TIME + (velocity[4*i+2] + velocity[4*i+3]) * (PREDICT_TIME - TRIGGER_TIME);
			sum *= mbr[2*i+1] - mbr[2*i] + w;
		}
	}

	return sum;
}

float margin(int dimension, float *mbr)
	// berechnet Umfang eines mbr der Dimension dimension
{
	float *ml, *mu, *m_last, sum;

	sum = 0.0;
	m_last = mbr + 2*dimension;
	ml = mbr;
	mu = ml + 1;
	while (mu < m_last)
	{
		sum += *mu - *ml;
		ml += 2;
		mu += 2;
	}

	return sum;
}

bool inside(float &p, float &lb, float &ub)
	// ist ein Skalar in einem Intervall ?
{
	return (p >= lb && p <= ub);
}

bool inside(float *v, float *mbr, int dimension)
	// ist ein Vektor in einer Box ?
{
	int i;

	for (i = 0; i < dimension; i++)
		if (!inside(v[i], mbr[2*i], mbr[2*i+1]))
			return FALSE;

	return TRUE;
}

// calcutales the overlapping rectangle between r1 and r2
// if rects do not overlap returns null
float* overlapRect(int dimension, float *r1, float *r2)
{
	float *overlap = new float[2*dimension];
	for (int i=0; i<dimension; i++)
	{
		if ((r1[i*2]>r2[i*2+1]) || (r1[i*2+1]<r2[i*2])) // non overlapping
		{
			delete [] overlap;
			return NULL;
		}
		overlap[2*i] = max(r1[i*2], r2[i*2]);
		overlap[2*i+1] = min(r1[i*2+1], r2[i*2+1]);
	}

	return overlap;
}

float overlap(int dimension, float *r1, float *r2)
	// calcutales the overlapping area of r1 and r2
	// calculate overlap in every dimension and multiplicate the values
{
	float sum;
	float *r1pos, *r2pos, *r1last, r1_lb, r1_ub, r2_lb, r2_ub;

	sum = 1.0;
	r1pos = r1; r2pos = r2;
	r1last = r1 + 2 * dimension;

	while (r1pos < r1last)
	{
		r1_lb = *(r1pos++);
		r1_ub = *(r1pos++);
		r2_lb = *(r2pos++);
		r2_ub = *(r2pos++);

		// calculate overlap in this dimension

		if (inside(r1_ub, r2_lb, r2_ub))
			// upper bound of r1 is inside r2 
		{
			if (inside(r1_lb, r2_lb, r2_ub))
				// and lower bound of r1 is inside
				sum *= (r1_ub - r1_lb);
			else
				sum *= (r1_ub - r2_lb);
		}
		else
		{
			if (inside(r1_lb, r2_lb, r2_ub))
				// and lower bound of r1 is inside
				sum *= (r2_ub - r1_lb);
			else 
			{
				if (inside(r2_lb, r1_lb, r1_ub) &&
					inside(r2_ub, r1_lb, r1_ub))
					// r1 contains r2
					sum *= (r2_ub - r2_lb);
				else
					// r1 and r2 do not overlap
					sum = 0.0;
			}
		}
	}

	return sum;
}

void futureMBR(int dimension, float *r1, float *v1, float *m1) {
	for(int i=0; i<dimension; ++i) {
		if (PREDICT_TIME < TRIGGER_TIME) {
			m1[2*i] = r1[2*i] - v1[4*i] * PREDICT_TIME;
			m1[2*i+1] = r1[2*i+1] + v1[4*i+1] * PREDICT_TIME;
		}
		else {
			m1[2*i] = r1[2*i] - v1[4*i] * TRIGGER_TIME - v1[4*i+2] * (PREDICT_TIME - TRIGGER_TIME);
			m1[2*i+1] = r1[2*i+1] + v1[4*i+1] * TRIGGER_TIME + v1[4*i+3] * (PREDICT_TIME - TRIGGER_TIME);
		}
	}

}
float overlap(int dimension, float *r1, float *v1, float *r2, float *v2)
	// calcutales the overlapping area of r1 and r2
	// calculate overlap in every dimension and multiplicate the values
{
	float *m1 = new float[dimension * 2];
	float *m2 = new float[dimension * 2];
	futureMBR(dimension, r1, v1, m1);
	futureMBR(dimension, r2, v2, m2);
	float sum = overlap(dimension, m1, m2);
	delete[] m1;
	delete[] m2;
	return sum;
}

void enlarge(int dimension, float **mbr, float *r1, float *r2)
	// enlarge r in a way that it contains s
{
	int i;

	*mbr = new float[2*dimension];
	for (i = 0; i < 2*dimension; i += 2)
	{
		(*mbr)[i]   = min(r1[i],   r2[i]);

		(*mbr)[i+1] = max(r1[i+1], r2[i+1]);
	}
}

bool section(int dimension, float *mbr1, float *mbr2)
{
	int i;

	for (i = 0; i < dimension; i++)
	{
		if (mbr1[2*i]     > mbr2[2*i + 1] ||
			mbr1[2*i + 1] < mbr2[2*i]) 
			return FALSE;
	}
	return TRUE;
}

bool section_c(int dimension, float *mbr1, float *center, float radius, int dim)
{
	float r2;

	r2 = radius * radius;

	if ( (r2 - MINDIST(center,mbr1,dim)) < 1.0e-8)
		return TRUE;
	else
		return FALSE;

}

int sort_lower_mbr(const void *d1, const void *d2)
	// Vergleichsfunktion fuer qsort, sortiert nach unterem Wert der mbr bzgl.
	// der angegebenen Dimension
{
	SortMbr *s1, *s2;
	float erg;
	int dimension;

	s1 = (SortMbr *) d1;
	s2 = (SortMbr *) d2;
	dimension = s1->dimension;
	erg = s1->mbr[2*dimension] - s2->mbr[2*dimension];
	if (erg < 0.0)
		return -1;
	else if (erg == 0.0)
		return 0;
	else 
		return 1;
}

int sort_upper_mbr(const void *d1, const void *d2)
	// Vergleichsfunktion fuer qsort, sortiert nach oberem Wert der mbr bzgl.
	// der angegebenen Dimension
{
	SortMbr *s1, *s2;
	float erg;
	int dimension;

	s1 = (SortMbr *) d1;
	s2 = (SortMbr *) d2;
	dimension = s1->dimension;
	erg = s1->mbr[2*dimension+1] - s2->mbr[2*dimension+1];
	if (erg < 0.0)
		return -1;
	else if (erg == 0.0)
		return 0;
	else 
		return 1;
}

int sort_center_mbr(const void *d1, const void *d2)
	// Vergleichsfunktion fuer qsort, sortiert nach center of mbr 
{
	SortMbr *s1, *s2;
	int i, dimension;
	float d, e1, e2;

	s1 = (SortMbr *) d1;
	s2 = (SortMbr *) d2;
	dimension = s1->dimension;

	e1 = e2 = 0.0;
	for (i = 0; i < dimension; i++)
	{
		d = ((s1->mbr[2*i] + s1->mbr[2*i+1]) / 2.0) - s1->center[i];
		e1 += d*d;
		d = ((s2->mbr[2*i] + s2->mbr[2*i+1]) / 2.0) - s2->center[i];
		e2 += d*d;
	}

	if (e1 < e2)
		return -1;
	else if (e1 == e2)
		return 0;
	else 
		return 1;
}

float objectDIST(float *p1, float *p2, int dim)
{

	//
	// Berechnet das Quadrat der euklid'schen Metrik.
	// (Der tatsaechliche Abstand ist uninteressant, weil
	// die anderen Metriken (MINDIST und MINMAXDIST fuer
	// die NearestNarborQuery nur relativ nie absolut 
	// gebraucht werden; nur Aussagen "<" oder ">" sind
	// relevant.
	//

	float summe = 0;
	int i;

	for( i = 0; i < dim; i++)
		summe += pow(p1[i] - p2[i], 2 );

	//return( sqrt(summe) );
	return(summe);
}

float MINDIST(float *p, float *bounces, int dimension)
{
	//
	// Berechne die kuerzeste Entfernung zwischen einem Punkt Point
	// und einem MBR bounces (Lotrecht!)
	//

	float summe = 0.0;
	float r;
	int i;

	for(i = 0; i < dimension; i++)
	{
		if (p[i] < bounces[2*i])
			r = bounces[2*i];
		else
		{
			if (p[i] > bounces[2*i +1])
				r = bounces[2*i+1];
			else 
				r = p[i];
		}    

		summe += pow( p[i] - r , 2);
	}
	return(summe);

}

float MINMAXDIST(float *p, float *bounces, int dimension)
{

	// Berechne den kleinsten maximalen Abstand von einem Punkt Point
	// zu einem MBR bounces.
	// Wird benutzt zur Abschaetzung von Abstaenden bei NearestNarborQuery.
	// Kann als Supremum fuer die aktuell kuerzeste Distanz: 
	// Alle Punkte mit einem Abstand > MINMAXDIST sind keine Kandidaten mehr
	// fuer den NearestNarbor
	// vgl. Literatur: 
	// Nearest Narbor Query v. Roussopoulos, Kelley und Vincent, 
	// University of Maryland

	float summe = 0;
	float minimum = 1.0e20;
	float S = 0;

	float rmk, rMi;
	int k,i;

	for( i = 0; i < dimension; i++) 
	{ 
		rMi = (	p[i] >= (bounces[2*i]+bounces[2*i+1])/2 )
			? bounces[2*i] : bounces[2*i+1];
		S += pow( p[i] - rMi , 2 );
	}

	for( k = 0; k < dimension; k++)
	{  

		rmk = ( p[k] <=  (bounces[2*k]+bounces[2*k+1]) / 2 ) ? 
			bounces[2*k] : bounces[2*k+1];

		summe = pow( p[k] - rmk , 2 );	

		rMi = (	p[k] >= (bounces[2*k]+bounces[2*k+1]) / 2 )
			? bounces[2*k] : bounces[2*k+1];

		summe += S - pow( p[k] - rMi , 2 );

		minimum = min( minimum,summe);
	}

	return(minimum);


}

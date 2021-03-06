/*
 * Copyright (c) 2015 Marzia Rivi
 *
 * This file is part of RadioLensfit.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "read_coordinates.h"

#ifdef __cplusplus
extern "C" {
#endif

// coordinates file and array have the same order
int read_coords_oskar(const char* filename, unsigned long int ncoords, double* x, double* y, double *lenu, double *lenv)
{
    FILE* fp;
    char *token;
    char line[100];
    unsigned long int index, i = 0;
    double maxx = 0.;
    double minx = 0.;
    double maxy = 0.;
    double miny = 0.;
    
    /* Open the file. */
    fp =fopen(filename, "r");
    if (!fp)
    {
        printf("ERROR: Unable to open the file %s\n", filename);
        exit(EXIT_FAILURE);
        return 1;
    }
    
    /* Read uv coordinates. */
    while (!feof(fp) && i < ncoords)
    {
        fgets(line, 100, fp);
        token = strtok(line, " ,\n");
        if (token && (token[0] != '#'))
        {
            if (token) sscanf(token, "%lu", &index);
            token = strtok(NULL, " ,\n");
            if (token)
            {
                sscanf(token, "%lf", &x[i]);
                if(x[i]>maxx) maxx = x[i];
                if(x[i]<minx) minx = x[i];
            }
            token = strtok(NULL, " ,\n");
            if (token)
            {
                sscanf(token, "%lf", &y[i]);
                if(y[i]>maxy) maxy = y[i];
                if(y[i]<miny) miny = y[i];
                
                i++;
            }
        }
    }
    if (maxx > -minx) *lenu = maxx;
    else *lenu = -minx;
    if (maxy > -miny) *lenv = maxy;
    else *lenv = -miny;
    
    fclose(fp);
    
    return i;
}


// for each time read only the baselines above the threshold, update their number,
// compute the maximum baseline (for the angular resolution),
// and compute the maximum u,v coordinate (for the gridding)

// input file is ordered as nbaselines x ntimes
// output array is ordered as ntimes x nbaselines
double read_coord_ska(const char* filename1, const char* filename2, unsigned int ntimes, unsigned int *nbaselines, double* x, double* y, double threshold , double *len)
{
    FILE* fp1;
    FILE* fp2;
    char *token;
    char line[100];
    
    unsigned long int num_coords = ntimes * (*nbaselines);
    double* temp_uu = (double *) malloc(num_coords*sizeof(double));
    double* temp_vv = (double *) malloc (num_coords*sizeof(double));
    unsigned long int* index = (unsigned long int *) malloc(num_coords*sizeof(unsigned long int));
    
#ifdef GRID
    double maxx = 0.;
    double minx = 0.;
    double maxy = 0.;
    double miny = 0.;
#endif
    unsigned int newbaselines, nt;
    unsigned long int i = 0, k=0;
    double xp,yp,modulus;
    
    /* Open the file. */
    fp1 =fopen(filename1, "r");
    if (!fp1)
    {
        printf("ERROR: Unable to open the file %s\n", filename1);
        exit(EXIT_FAILURE);
        return 1;
    }
    
    fp2 =fopen(filename2, "r");
    if (!fp2)
    {
        printf("ERROR: Unable to open the file %s\n", filename2);
        exit(EXIT_FAILURE);
        return 1;
    }
    
    double maxB = 0.;

    while (!feof(fp1) && !feof(fp2))
    {
        fgets(line, 100, fp1);
        sscanf(line, "%lf", &xp);
        
        fgets(line, 100, fp2);
        sscanf(line, "%lf", &yp);
        
        temp_uu[i]=xp;
        temp_vv[i]=yp;
        
        if (i < (*nbaselines))  // time 0
        {
            modulus = sqrt(xp*xp+yp*yp);
            if(modulus > threshold)
            {
                index[k]=i;
                if(modulus > maxB) maxB = modulus;
                x[k*ntimes]=xp; y[k*ntimes]=yp;
                k++;
#ifdef GRID
                if(xp>maxx) maxx = xp;
                if(xp<minx) minx = xp;
                if(yp>maxy) maxy = yp;
                if(yp<miny) miny = yp;
#endif
                
            }
        }
        if (i == (*nbaselines)) newbaselines = k;
        i++;
    }
    
    unsigned long int start = 0;
    for (nt=1; nt<ntimes; nt++)
    {
        start += (*nbaselines);
        for (i=0; i<newbaselines; i++)
        {
            xp = temp_uu[start+index[i]];
            yp = temp_vv[start+index[i]];
 
#ifdef GRID
            if(xp>maxx) maxx = xp;
            if(xp<minx) minx = xp;
            if(yp>maxy) maxy = yp;
            if(yp<miny) miny = yp;
#endif
            k = i*ntimes+nt;
            x[k]=xp; y[k]=yp;
        }
    }
    
    fclose(fp1);
    fclose(fp2);

    free(temp_uu);
    free(temp_vv);
    free(index);

    *nbaselines = newbaselines;
#ifdef GRID
    *len = ceil(fmax(fmax(maxx,-minx),fmax(maxy,-miny)));  // return max coordinate for the uv grid
#endif

    return maxB;
}

#ifdef __cplusplus
}
#endif




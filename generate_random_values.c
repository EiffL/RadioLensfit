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

#ifdef _OPENMP
#include <omp.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <gsl/gsl_rng.h>


#include "random_gaussian.h"
#include "generate_random_values.h"
#include "distributions.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define EPS 1.0e-5
#define JMAX 30
#define N 1000

#ifdef __cplusplus
extern "C" {
#endif
    
    
// Generate random data in the range [min_value,max_value] according to a distribution with a given Cumulative Distribution Function CDFunc(param, x)
void generate_random_data(gsl_rng * gen, int nr, double *data, double min_value, double max_value, double (*CDFunc)(double,double), double param)
{
        
    int i,k;
    double u;
        
    // compute N points of the scalelength cumulative distribution function
    double *F = (double *) malloc((N+1)*sizeof(double));
        
    double h = (max_value-min_value)/N;
    double CFmin = (*CDFunc)(param,min_value);
    double CFrange = (*CDFunc)(param,max_value) - CFmin;
    
    F[0]=0.;
#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
    for (i=1; i <= N; i++) F[i] = (*CDFunc)(param,min_value+i*h) - CFmin;
        
    for (i=0; i < nr; i++)
    {
        /* generate random scalelength according to scale_pdf */
        u = gsl_rng_uniform(gen)*CFrange;
            
        k=0;
        while ((k <= N) && (u > F[k])) k++;
        // u is in the interval (F(k-1),F(k)), find F^{-1}(u) using a linear interpolation
        data[i] = min_value+h*(k-1)+h*(u-F[k-1])/(F[k]-F[k-1]);
    }
    free(F);
        
}

void generate_ellipticity(gsl_rng * gen, int ne, int NP, double *e1, double *e2)
{
    int i,k,ind;
 
    // compute N points of the ellipticity cumulative distribution function
    double *F = (double *) malloc((N+1)*sizeof(double));
    
    double h = 0.804/N;   // e_max = 0.804
    F[0]=0.;
#ifdef _OPENMP
#pragma omp parallel for private(i)
#endif
    for (i=1; i <= N; i++) F[i] = CDF(e_pdf,i*h); 
    
    double phi_0, phi, u, module, inc;
    inc = (double) PI/NP;
    ind = 0;
    
    for (i=0; i < ne; i++)
    {
        /* generate random |e| according to e_pdf */
        u = gsl_rng_uniform(gen);
 
        k=0;
        while (u > F[k]) k++;
        // u is in the interval (F(k-1),F(k)), find F^{-1}(u) using a linear interpolation
        module = h*(k-1)+h*(u-F[k-1])/(F[k]-F[k-1]);
        
        /* Choose a phase shift in a uniform interval (0, 2PI). */
        phi_0 = 2*PI * gsl_rng_uniform_pos(gen);
 
        /* generate a circle of points centered in 0 with radius |e| */
        for (k=0; k < NP; k++)
        {
            phi = phi_0 + k*inc;
            e1[ind] = module*cos(phi);
            e2[ind] = module*sin(phi);
            ind++;
            // phi+PI
            e1[ind] = -e1[ind-1];
            e2[ind] = -e2[ind-1];
            ind++;
        }
    }
    free(F);
 
}
 

// Cumulative Distribution Function: \int_0^b e_pdf(x)
// apply an extended trapezoidal rule (Numerical Recipes p.137)
double CDF(double (*pdf)(double), double b)
{
    double x, tnm, del, sum;
    double s, olds = 0.;
    int it,j,k;
    
    s=0.5*b*(*pdf)(b);
    for (k=2; k<=JMAX; k++)
    {
        for (it=1,j=1;j<k-1;j++) it <<= 1;
        tnm = (double) it;
        del = b/tnm;      //This is the spacing of the points to be added.
        x = 0.5*del;
        for (sum=0.,j=1; j<it; j++,x+=del) sum += (*pdf)(x);
        s = 0.5*(s+del*sum);  //This replaces s by its refined value. NB. pdf(0)=0
    
        if (k > 5)          //Avoid spurious early convergence.
            if (fabs(s-olds) < EPS*fabs(olds) ||
                (s == 0.0 && olds == 0.0)) return s;
        olds=s;
    }
    printf("\n CDF: Too many steps\n");
    return 0.0;
}
    

    
#ifdef __cplusplus
}
#endif

 

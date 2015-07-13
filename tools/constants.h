/**
 * \author Sylvain Marsat, University of Maryland - NASA GSFC
 *
 * \brief C header defining useful physical constants (values taken from LAL).
 * Also defines boolean conventions.
 *
 */

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#define _XOPEN_SOURCE 500

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_bspline.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_min.h>
#include <gsl/gsl_spline.h>

/***************************************************/
/****** Boolean conventions for loading files ******/

#define SUCCESS 0
#define FAILURE 1

/*********************************************************/
/*************** Mathematical constants ******************/

#define PI 3.1415926535897932384626433832795029

/**********************************************************************/
/**************** Physical constants in SI units **********************/

#define C_SI 299792458
#define G_SI 6.67259e-11
#define MSUN_SI 1.98892e30
#define MTSUN_SI 4.9254923218988636432342917247829673e-6
#define PC_SI 3.0856775807e16
#define AU_SI 1.4959787066e11
#define YRSID_SI 31558149.8 /* Sideral year */

#define Omega_SI 1.99098659e-7 /* Orbital pulsation: 2pi/year */
#define f0_SI 3.168753575e-8 /* Orbital fequency: 1/year */
#define L_SI 1.0e9 /* Arm length of the detector: 10^6 km */
#define R_SI 1.4959787066e11 /* Radius of the orbit around the sun: 1AU */

/**********************************************************/
/********** Constants used to relate time scales **********/

#define EPOCH_J2000_0_TAI_UTC 32           /* Leap seconds (TAI-UTC) on the J2000.0 epoch (2000 JAN 1 12h UTC) */
#define EPOCH_J2000_0_GPS 630763213        /* GPS seconds of the J2000.0 epoch (2000 JAN 1 12h UTC) */

/*******************************************/
/**************** NaN **********************/

#ifndef NAN
# define NAN (INFINITY-INFINITY)
#endif 

#if 0
{ /* so that editors will match succeeding brace */
#elif defined(__cplusplus)
}
#endif

#endif /* _CONSTANTS_H */
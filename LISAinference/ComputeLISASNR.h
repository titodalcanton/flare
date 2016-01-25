/**
 * \author Sylvain Marsat, University of Maryland - NASA GSFC
 *
 * \brief C header for functions windowing and computing FFT/IFFT of time/frequency series.
 *
 */

#ifndef __COMPUTELISASNR_H__
#define __COMPUTELISASNR_H__ 1

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

#define _XOPEN_SOURCE 500

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
#include <gsl/gsl_complex.h>

#include "constants.h"
#include "struct.h"
#include "waveform.h"
#include "fft.h"
#include "LISAgeometry.h"
#include "LISAFDresponse.h"
#include "LISAutils.h"

/********************************** Structures ******************************************/

/* Parameters for the generation of a LISA waveform (in the form of a list of modes) */
typedef struct tagComputeLISASNRparams {
  double tRef;               /* reference time (s) - GPS time at the frequency representing coalescence */
  double phiRef;             /* reference phase (rad) - phase at the frequency representing coalescence (or at fRef if specified) */
  double fRef;               /* reference frequency at which phiRef is set (Hz, default 0 which is interpreted as Mf=0.14) */
  double m1;                 /* mass of companion 1 (solar masses, default 2e6) */
  double m2;                 /* mass of companion 2 (solar masses, default 1e6) */
  double distance;           /* distance of source (Mpc, default 1e3) */
  double inclination;        /* inclination of source (rad, default pi/3) */
  double lambda;             /* first angle for the position in the sky (rad, default 0) */
  double beta;               /* second angle for the position in the sky (rad, default 0) */
  double polarization;       /* polarization angle (rad, default 0) */

  int nbmode;                /* number of modes to generate (starting with 22) - defaults to 5 (all modes) */
  double fLow;               /* Minimal frequency (Hz) - when set to 0 (default), use the first frequency covered by the ROM */
  int tagtdi;                /* Tag selecting the desired TDI observables */
  int tagint;                /* Tag choosing the integrator: 0 for Fresnel (default), 1 for linear integration */
  int nbptsoverlap;          /* Number of points to use in loglinear overlaps (default 32768) */
  int fromtditdfile;         /* Option for loading time series for TDI observables and FFTing */
  int nlinesinfile;          /* Number of lines of input file */
  char indir[256];           /* Path for the input directory */
  char infile[256];          /* Path for the input file */
} ComputeLISASNRparams;


#if 0
{ /* so that editors will match succeeding brace */
#elif defined(__cplusplus)
}
#endif

#endif /* _COMPUTELISASNR_H */

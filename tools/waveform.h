/**
 * \author Sylvain Marsat, University of Maryland - NASA GSFC
 *
 * \brief C header for functions manipulating waveforms.
 *
 */

#ifndef _WAVEFORM_H
#define _WAVEFORM_H

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
#include <gsl/gsl_complex.h>

#include "constants.h"
#include "struct.h"
#include "EOBNRv2HMROM.h"


/* NOTE: uses the list of modes of EOBNRv2HMROM (listmode), to be extended when more waveform models are added */

/***************** Function estimating time to coalescence and min/max frequency ****************/

/* Functions reading from a list of modes the minimal and maximal frequencies */
double ListmodesCAmpPhaseFrequencySeries_maxf(ListmodesCAmpPhaseFrequencySeries* listhlm);
double ListmodesCAmpPhaseFrequencySeries_minf(ListmodesCAmpPhaseFrequencySeries* listhlm);

/* Function estimating initial time from Psi22, according to tf_SPA = -1/(2pi)dPsi/df */
double EstimateInitialTime(ListmodesCAmpPhaseFrequencySeries* listhlm, double fLow);

/***************** Functions to manipulate ReImFrequencySeries structure ****************/

void ReImFrequencySeries_AddCAmpPhaseFrequencySeries(
  struct tagReImFrequencySeries* freqseriesReIm,              /* Output Re/Im frequency series */
  struct tagCAmpPhaseFrequencySeries* freqseriesCAmpPhase,    /* Input CAmp/Phase frequency series, to be interpolated and added to the output */
  double fstartobsmode);                                      /* Starting frequency in case of limited duration of observations- assumed to have been scaled with the proper factor m/2 for this mode - set to 0 to ignore */
/* Function evaluating a ReImFrequencySeries by interpolating wach mode of a ListmodesCAmpPhaseFrequencySeries and summing them, given a set of frequencies */
void ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(
  struct tagReImFrequencySeries* freqseriesReIm,                    /* Output Re/Im frequency series - already initialized */
  struct tagListmodesCAmpPhaseFrequencySeries* listmodesCAmpPhase,  /* Input CAmp/Phase frequency series, to be interpolated */
  gsl_vector* freq,                                                 /* Input set of frequencies on which evaluating */
  double fstartobs);                                                /* Starting frequency in case of limited duration of observations - set to 0 to ignore */
/* Helper function to add a mode to hplus, hcross in Fourier domain
 * - copies the function XLALSimAddMode, which was done only for TD structures */
int FDAddMode(
  ReImFrequencySeries* hptilde,       /* Output: frequency series for hplus */
  ReImFrequencySeries* hctilde,       /* Output: frequency series for hcross */
  ReImFrequencySeries* hlmtilde,      /* Input: frequency series for the mode hlm */
  double theta,                       /* First angle for position in the sky of observer */
  double phi,                         /* Second angle for position in the sky of observer  */
  int l,                              /* First mode number l */
  int m,                              /* Second mode number m */
  int sym);                           /* If 1, assume planar symmetry and add also mode l,-m. Do not if set to 0. */
/* Function evaluating the FD frequency series for hplus, hcross from the modes hlm */
int GeneratehphcFDReImFrequencySeries(
  ReImFrequencySeries** hptilde,                 /* Output: frequency series for hplus */
  ReImFrequencySeries** hctilde,                 /* Output: frequency series for hcross */
  ListmodesCAmpPhaseFrequencySeries* listhlm,    /* Input: frequency series for the mode hlm  */
  double fLow,                                   /* Minimal frequency */
  double deltaf,                                 /* Frequency step */
  int nbpt,                                      /* Number of points of output - if 0, determined from deltaF and maximal frequency in input */
  int nbmode,                                    /* Number of modes to add */
  double theta,                                  /* First angle for position in the sky of observer */
  double phi,                                    /* Second angle for position in the sky of observer */
  int sym);                                      /* If 1, assume planar symmetry and add also mode l,-m. Do not if set to 0. */
/* NOTE: nbmode as arg here is a bit deceiptive, only used for max frequency */
int GenerateFDReImFrequencySeries(
  ReImFrequencySeries** freqseries,              /* Output: frequency series */
  ListmodesCAmpPhaseFrequencySeries* listhlm,    /* Input: FD modes hlm in the form AmpReal/AmpIm/Phase  */
  double fLow,                                   /* Minimal frequency */
  double deltaf,                                 /* Frequency step */
  int nbpt,                                      /* Number of points of output - if 0, determined from deltaf and maximal frequency in input */
  int nbmode);                                   /* Number of modes - used only to determine the highest frequency */
/* Function to restrict a frequency series (typically output of a FFT) to a given frequency range */
int RestrictFDReImFrequencySeries(
  ReImFrequencySeries** freqseriesout,           /* Output: truncated frequency series */
  ReImFrequencySeries* freqseriesin,             /* Input: frequency series */
  double fLow,                                   /* Minimal frequency */
  double fHigh);                                 /* Maximal frequency */

/***************** Spin weighted spherical harmonics ****************/

/* Additional function reproducing XLALSpinWeightedSphericalHarmonic */
double complex SpinWeightedSphericalHarmonic(double theta, double phi, int s, int l, int m); /* Currently only supports s=-2, l=2,3,4,5 modes */

#if 0
{ /* so that editors will match succeeding brace */
#elif defined(__cplusplus)
}
#endif

#endif /* _WAVEFORM_H */
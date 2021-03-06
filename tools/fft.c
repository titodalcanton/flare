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

#include <fftw3.h> /* Note: when included AFTER complex.h, fftw_complex type defaults to native double complex */

#include "constants.h"
#include "struct.h"


/* Window functions */
double WindowFunction(double x, double xi, double xf, double deltaxi, double deltaxf)
{
  double di =  deltaxi/(20*log(10));
  double df =  deltaxf/(20*log(10));
  if (x <= xi + di) return 0;
  else if (xi +di < x && x < xi + deltaxi - di) {
    return 1./(1 + exp(deltaxi/(x - xi) + deltaxi/(x - (xi + deltaxi))));
  }
  else if (xi + deltaxi - di <= x && x <= xf - deltaxf + df) return 1.;
  else if (xf - deltaxf + df < x && x < xf - df) {
    return 1./(1 + exp(-(deltaxf/(x - (xf - deltaxf))) - deltaxf/(x - xf)));
  }
  else return 0;
}
double WindowFunctionLeft(double x, double xf, double deltaxf)
{
  double df =  deltaxf/(20*log(10));
  if (x <= xf - deltaxf + df) return 1;
  else if (xf - deltaxf + df < x && x < xf - df) {
    return 1./(1 + exp(-deltaxf/(x - (xf - deltaxf)) - deltaxf/(x - xf)));
  }
  else return 0;
}
double WindowFunctionRight(double x, double xi, double deltaxi)
{
  double di =  deltaxi/(20*log(10));
  if (x <= xi + di) return 0;
  else if (xi + di < x && x < xi + deltaxi - di) {
    return 1./(1 + exp(deltaxi/(x - xi) + deltaxi/(x - (xi + deltaxi))));
  }
  else return 1;
}

/* FFT of real time series */
/* Note: FFT uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
int FFTRealTimeSeries(ReImFrequencySeries** freqseries, RealTimeSeries* timeseries, double twindowbeg, double twindowend, int nzeropad)
{

  /* deltat of time series */
  /* Warning: assumes linear sampling in time */
  double* times = timeseries->times->data;
  double deltat = times[1] - times[0];
  double tshift = times[0]; /* time shift to be re-applied later */

  /* Initialize vector for windowed, 0-padded FFT input */
  int n = (int) timeseries->times->size;
  int nzeros = (int) pow(2, ((int) ceil(log(n)/log(2))) + nzeropad) - n; /* Here defined with ceil, but with floor in IFFT */
  gsl_vector* hvalues = gsl_vector_alloc(n + nzeros);
  gsl_vector_set_zero(hvalues);

  /* Compute input TD values, with windowing */
  int nbptswindowbeg = (int) ceil(twindowbeg/deltat) + 1;
  int nbptswindowend = (int) ceil(twindowend/deltat) + 1;
  double t1windowbeg = times[0];
  double t2windowbeg = times[nbptswindowbeg-1];
  double t1windowend = times[n-nbptswindowend];
  double t2windowend = times[n-1];
  double deltatwindowbeg = t2windowbeg - t1windowbeg;
  double deltatwindowend = t2windowend - t1windowend;
  double* h = timeseries->h->data;
  double* hval = hvalues->data;

  for (int i=0; i<nbptswindowbeg; i++) {
    hval[i] = WindowFunctionRight(times[i], t1windowbeg, deltatwindowbeg) * h[i];
  }
  for (int i=nbptswindowbeg; i<n-nbptswindowend; i++) {
    hval[i] =  h[i];
  }
  for (int i=n-nbptswindowend; i<n; i++) {
    hval[i] = WindowFunctionLeft(times[i], t2windowend, deltatwindowend) * h[i];
  }

  /* FFT - uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
  int N = (int) hvalues->size;
  fftw_plan p;
  double* in = hvalues->data;
  fftw_complex* out;
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2+1)); /* Note: N/2+1 elements */
  p = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
  fftw_execute(p);

  /* Initialize output structure */
  ReImFrequencySeries_Init(freqseries, N/2); /* Note: N/2+1 elements as output of fftw, we drop the last one (at Nyquist frequency) */

  /* Extracting and converting data from FFTW output */
  double deltaf = 1./(N*deltat);
  double* freq = (*freqseries)->freq->data;
  double* hreal = (*freqseries)->h_real->data;
  double* himag = (*freqseries)->h_imag->data;
  double f;
  double complex hcomplex;
  double factorshift = 2*PI*tshift; /* Reminder: Flipped sign convention */
  for(int i=0; i<N/2; i++) {
    f = i*deltaf;
    freq[i] = f;
    hcomplex = deltat * conj(out[i]) * cexp(I*factorshift*f); /* Note that we convert here the FT sign convention */
    hreal[i] = creal(hcomplex);
    himag[i] = cimag(hcomplex);
  }

  /* Clean up */
  gsl_vector_free(hvalues);
  fftw_destroy_plan(p);
  fftw_free(out);

  return SUCCESS;
}

/* FFT of Re/Im time series */
/* Note: FFT uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
/* NOTE: only keeps positive frequencies in the output */
int FFTTimeSeries(ReImFrequencySeries** freqseries, ReImTimeSeries* timeseries, double twindowbeg, double twindowend, int nzeropad)
{
  /* deltat of time series */
  /* Warning: assumes linear sampling in time */
  double* times = timeseries->times->data;
  double deltat = times[1] - times[0];
  double tshift = times[0]; /* time shift to be re-applied later */

  /* Initialize vector for windowed, 0-padded FFT input */
  int n = (int) timeseries->times->size;
  int nzeros = (int) pow(2, ((int) ceil(log(n)/log(2))) + nzeropad) - n; /* Here defined with ceil, but with floor in IFFT */
  int N = n + nzeros;

  /* Compute input TD values, with windowing */
  int nbptswindowbeg = (int) ceil(twindowbeg/deltat) + 1;
  int nbptswindowend = (int) ceil(twindowend/deltat) + 1;
  double t1windowbeg = times[0];
  double t2windowbeg = times[nbptswindowbeg-1];
  double t1windowend = times[n-nbptswindowend];
  double t2windowend = times[n-1];
  double deltatwindowbeg = t2windowbeg - t1windowbeg;
  double deltatwindowend = t2windowend - t1windowend;
  double* htdreal = timeseries->h_real->data;
  double* htdimag = timeseries->h_imag->data;

  fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  for (int i=0; i<nbptswindowbeg; i++) {
    in[i] = WindowFunctionRight(times[i], t1windowbeg, deltatwindowbeg) * (htdreal[i] + I*htdimag[i]);
  }
  for (int i=nbptswindowbeg; i<n-nbptswindowend; i++) {
    in[i] =  (htdreal[i] + I*htdimag[i]);
  }
  for (int i=n-nbptswindowend; i<n; i++) {
    in[i] = WindowFunctionLeft(times[i], t2windowend, deltatwindowend) * (htdreal[i] + I*htdimag[i]);
  }

  /* FFT - uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
  /* Represented here by the use of FFTW_BACKWARD (plus sign in the exp) */
  fftw_plan p;
  fftw_complex* out;
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N); /* Note: N/2+1 elements */
  p = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);
  fftw_execute(p);

  /* Initialize output structure */
  ReImFrequencySeries_Init(freqseries, N/2); /* NOTE: N/2 first elements of output of fftw are positive freqs, we eliminate negative frequency (the second half of the series) */

  /* Extracting and converting data from FFTW output */
  double deltaf = 1./(N*deltat);
  double* freq = (*freqseries)->freq->data;
  double* hreal = (*freqseries)->h_real->data;
  double* himag = (*freqseries)->h_imag->data;
  double f;
  double complex hcomplex;
  double factorshift = 2*PI*tshift; /* Reminder: Flipped sign convention */
  for(int i=0; i<N/2; i++) {
    f = i*deltaf;
    freq[i] = f;
    hcomplex = deltat * out[i] * cexp(I*factorshift*f); /* Note the FT sign convention  */
    hreal[i] = creal(hcomplex);
    himag[i] = cimag(hcomplex);
  }

  /* Clean up */
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);

  return SUCCESS;
}

/* IFFT of frequency series */
/* Note: assumes frequency series is FT of real data - produces real time series */
/* Note: FFT uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
int IFFTFrequencySeriesReal(RealTimeSeries** timeseries, ReImFrequencySeries* freqseries, double f1windowbeg, double f2windowbeg, double f1windowend, double f2windowend, int nzeropad)
{
  /* Checking sanity of windowing frequencies */
  if(!((f1windowbeg<f2windowbeg)&&(f1windowend<f2windowend)&&(f2windowbeg<f1windowend))) {
    printf("Error in IFFTFrequencySeriesReal: inconsistent windowing frequencies.\n");
    printf("(f1windowbeg, f2windowbeg, f1windowend, f2windowend) = (%g, %g, %g, %g)\n", f1windowbeg, f2windowbeg, f1windowend, f2windowend);
  }

  /* deltaf of frequency series */
  /* Warning: assumes linear sampling in frequency */
  double* freq = freqseries->freq->data;
  double deltaf = freq[1] - freq[0];

  /* Initialize vector for windowed, 0-padded FFT input */
  int n = (int) freqseries->freq->size;
  while(freq[n-1] > f2windowend) n--;
  int nzeros = (int) pow(2, ((int) ceil(log(n)/log(2))) + nzeropad) - n; /* Here defined with floor, but with ceil in FFT */
  gsl_vector* hrealvalues = gsl_vector_alloc(n + nzeros);
  gsl_vector* himagvalues = gsl_vector_alloc(n + nzeros);
  gsl_vector_set_zero(hrealvalues);
  gsl_vector_set_zero(himagvalues);

  /* Compute input FD values, with windowing */
  double deltafwindowbeg = f2windowbeg - f1windowbeg;
  double deltafwindowend = f2windowend - f1windowend;
  double* hreal = freqseries->h_real->data;
  double* himag = freqseries->h_imag->data;
  double* hrealval = hrealvalues->data;
  double* himagval = himagvalues->data;
  for (int i=0; i<n; i++) {
    double window = WindowFunction(freq[i], f1windowbeg, f2windowend, deltafwindowbeg, deltafwindowend);
    hrealval[i] = window * hreal[i];
    himagval[i] = window * himag[i];
  }

  /* Input as array of fftw_complex */
  int N = (int) hrealvalues->size;
  fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  /* NOTE: Restoring the standard sign convention for the FT, used by FFTW - change in convention amounts to f->-f, equivalent to a conjugation only for FFT of a real series  */
  for(int i=0; i<n; i++) {
    in[i] = hrealval[i] - I*himagval[i] ;
  }
  for(int i=n; i<N; i++) {
    in[i] = 0;
  }

  /* FFT - uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
  fftw_plan p;
  double* out = fftw_malloc(sizeof(double) * N);
  p = fftw_plan_dft_c2r_1d(N, in, out, FFTW_ESTIMATE);
  fftw_execute(p);

  /* Initialize output structure */
  RealTimeSeries_Init(timeseries, N);

  /* Extracting and converting data from FFTW output - moving negative times to the left */
  double deltat = 1./(N*deltaf);
  double fac = 1./(N*deltat); /* Additional 1/N to put the FFTW convention in agreement with numpy IFFT */
  double* times = (*timeseries)->times->data;
  double* h = (*timeseries)->h->data;
  for(int i=0; i<N/2; i++) {
    times[i] = (i-N/2)*deltat;
    times[N/2+i] = i*deltat;
    h[i] = fac * out[N/2+i];
    h[N/2+i] = fac * out[i];
  }

  /* Clean up */
  gsl_vector_free(hrealvalues);
  gsl_vector_free(himagvalues);
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);

  return SUCCESS;
}


/* IFFT of frequency series */
/* Note: does not assume frequency series is FT of real data - produces complex time series */
/* Note: FFT uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) */
int IFFTFrequencySeries(ReImTimeSeries** timeseries, ReImFrequencySeries* freqseries, double f1windowbeg, double f2windowbeg, double f1windowend, double f2windowend, int nzeropad)
{
  /* Checking sanity of windowing frequencies */
  if(!((f1windowbeg<f2windowbeg)&&(f1windowend<f2windowend)&&(f2windowbeg<f1windowend))) {
    printf("Error in IFFTFrequencySeries: inconsistent windowing frequencies.\n");
    printf("(f1windowbeg, f2windowbeg, f1windowend, f2windowend) = (%g, %g, %g, %g)\n", f1windowbeg, f2windowbeg, f1windowend, f2windowend);
  }

  /* deltaf of frequency series */
  /* Warning: assumes linear sampling in frequency */
  double* freq = freqseries->freq->data;
  double deltaf = freq[1] - freq[0];

  /* Initialize vector for windowed, 0-padded FFT input */
  int n = (int) freqseries->freq->size;
  while(freq[n-1] > f2windowend) n--;
  int nzeros = (int) pow(2, ((int) ceil(log(n)/log(2))) + nzeropad) - n; /* Here defined with floor, but with ceil in FFT */
  gsl_vector* hrealvalues = gsl_vector_alloc(n + nzeros);
  gsl_vector* himagvalues = gsl_vector_alloc(n + nzeros);
  gsl_vector_set_zero(hrealvalues);
  gsl_vector_set_zero(himagvalues);

  /* Compute input FD values, with windowing */
  double deltafwindowbeg = f2windowbeg - f1windowbeg;
  double deltafwindowend = f2windowend - f1windowend;
  double* hreal = freqseries->h_real->data;
  double* himag = freqseries->h_imag->data;
  double* hrealval = hrealvalues->data;
  double* himagval = himagvalues->data;
  for (int i=0; i<n; i++) {
    double window = WindowFunction(freq[i], f1windowbeg, f2windowend, deltafwindowbeg, deltafwindowend);
    hrealval[i] = window * hreal[i];
    himagval[i] = window * himag[i];
  }

  /* Input as array of fftw_complex */
  int N = (int) hrealvalues->size;
  fftw_complex* in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  /* NOTE: the sign convention for the FT used by FFTW is different - change in convention amounts to f->-f, equivalent to a conjugation only for FFT of a real series - for a FFT of a complex series, we do not conjugate and we keep our convention */
  for(int i=0; i<n; i++) {
    in[i] = hrealval[i] + I*himagval[i] ;
  }
  for(int i=n; i<N; i++) {
    in[i] = 0;
  }

  /* FFT - FFTW uses flipped convention (i.e. h(f) = int e^(+2ipift)h(t)) - our convention is h(f) = int e^(-2ipift)h(t) */
  /* NOTE: due to the difference in convention for the FT, we use the FFTW_FORWARD sign (sign - in the exponential) */
  fftw_plan p;
  double complex* out = fftw_malloc(sizeof(fftw_complex) * N);
  p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p);

  /* Initialize output structure */
  ReImTimeSeries_Init(timeseries, N);

  /* Extracting and converting data from FFTW output - moving negative times to the left */
  double deltat = 1./(N*deltaf);
  double fac = 1./(N*deltat); /* Additional 1/N to put the FFTW convention in agreement with numpy IFFT */
  double* times = (*timeseries)->times->data;
  double* htdreal = (*timeseries)->h_real->data;
  double* htdimag = (*timeseries)->h_imag->data;
  for(int i=0; i<N/2; i++) {
    times[i] = (i-N/2)*deltat;
    times[N/2+i] = i*deltat;
    htdreal[i] = fac * creal(out[N/2+i]);
    htdimag[i] = fac * cimag(out[N/2+i]);
    htdreal[N/2+i] = fac * creal(out[i]);
    htdimag[N/2+i] = fac * cimag(out[i]);
  }

  /* Clean up */
  gsl_vector_free(hrealvalues);
  gsl_vector_free(himagvalues);
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);

  return SUCCESS;
}

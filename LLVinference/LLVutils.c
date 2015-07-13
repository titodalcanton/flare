#include "LLVutils.h"

/************ Global Parameters ************/

LLVParams* injectedparams = NULL;
LLVParams* templateparams = NULL;
LLVPrior* priorParams = NULL;

/************ Functions to initalize and clean up structure for the signals ************/

void LLVSignal_Cleanup(LLVSignal* signal) {
  if(signal->LHOSignal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->LHOSignal);
  if(signal->LLOSignal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->LLOSignal);
  if(signal->VIRGOSignal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->VIRGOSignal);
  free(signal);
}

void LLVSignal_Init(LLVSignal** signal) {
  if(!signal) exit(1);
  /* Create storage for structures */
  if(!*signal) *signal = malloc(sizeof(LLVSignal));
  else
  {
    LLVSignal_Cleanup(*signal);
  }
  (*signal)->LHOSignal = NULL;
  (*signal)->LLOSignal = NULL;
  (*signal)->VIRGOSignal = NULL;
}

/************ Functions for LLV parameters, injection, likelihood, prior ************/

/* Parse command line to initialize LLVParams, LLVPrior, and LLVRunParams objects */
void parse_args_LLV(ssize_t argc, char **argv, 
    LLVParams* params, 
    LLVPrior *prior, 
    LLVRunParams *run) 
{
    char help[] = "\
LLVInference by Sylvain Marsat, John Baker, and Philip Graff\n\
Copyright July 2015\n\
\n\
This program performs rapid parameter estimation for LIGO and LISA CBC sources in the no-noise case.\n\
Arguments are as follows:\n\
\n\
--------------------------------------------------\n\
----- Injected Signal Parameters -----------------\n\
--------------------------------------------------\n\
 --tRef                Time at reference frequency (sec, default=0)\n\
 --phiRef              Orbital phase at reference frequency (radians, default=0)\n\
 --m1                  Component mass 1 in Solar masses (larger, default=20)\n\
 --m2                  Component mass 2 in Solar masses (smaller, default=10)\n\
 --distance            Distance to source in Mpc (default=100)\n\
 --ra                  Right ascension of source sky location (radians, defualt=0)\n\
 --dec                 Declination of source sky location (radians, default=0)\n\
 --inclination         Inclination of source orbital plane to observer line of sight\n\
                       (radians, default=PI/3)\n\
 --polarization        Polarization of source (radians, default=0)\n\
 --fRef                Reference frequency (Hz, default=merger[0])\n\
 --nbmode              Number of modes of radiation to use (1-5, default=5)\n\
\n\
--------------------------------------------------\n\
----- Prior Boundary Settings --------------------\n\
--------------------------------------------------\n\
 --deltaT              Half-width of time prior (sec, default=0.2)\n\
 --comp-min            Minimum component mass in Solar masses (default=4)\n\
 --comp-max            Maximum component mass in Solar masses (default=50)\n\
 --mtot-min            Minimum total mass in Solar masses (default=8)\n\
 --mtot-max            Maximum total mass in Solar masses (default=100)\n\
 --q-max               Maximum mass ratio, m1/m2 (default=11.98, minimum is 1)\n\
 --dist-min            Minimum distance to source (Mpc, default=1)\n\
 --dist-max            Maximum distance to source (Mpc, default=10000)\n\
\n\
--------------------------------------------------\n\
----- BAMBI Sampler Settings ---------------------\n\
--------------------------------------------------\n\
 --eff                 Target efficiency of sampling (default=0.1)\n\
 --tol                 Tolerance for evidence calculation convergence (default=0.5)\n\
 --nlive               Number of live points for sampling (default=1000)\n\
 --bambi               Use BAMBI's neural network logL learning (no option, default off)\n\
 --resume              Resume from a previous run (no option, default off)\n\
 --outroot             Root for output files (default='chains/LLVinference_')\n\
 --netfile             Neural network settings file if using --bambi (default='LLVinference.inp')\n\
\n";

    ssize_t i;

    /* set default values for the injection params */
    params->tRef = 0.;
    params->phiRef = 0.;
    params->m1 = 20.;
    params->m2 = 10.;
    params->distance = 100.;
    params->ra = 0.;
    params->dec = 0.;
    params->inclination = PI/3.;
    params->polarization = 0.;
    params->fRef = 0.;
    params->nbmode = 5;

    /* set default values for the prior limits */
    prior->deltaT = 0.1;
    prior->comp_min = 4.0;
    prior->comp_max = 50.0;
    prior->mtot_min = 8.0;
    prior->mtot_max = 100.0;
    prior->qmax = 11.98;
    prior->dist_min = 1.0;
    prior->dist_max = 10000.0;

    /* set default values for the run settings */
    run->eff = 0.1;
    run->tol = 0.5;
    run->nlive = 1000;
    strcpy(run->outroot, "chains/LLVinference_");
    run->bambi = 0;
    run->resume = 0;
    strcpy(run->netfile, "LLVinference.inp");

    /* Consume command line */
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            fprintf(stdout,"%s", help);
            exit(0);
        } else if (strcmp(argv[i], "--tRef") == 0) {
            params->tRef = atof(argv[++i]);
        } else if (strcmp(argv[i], "--phiRef") == 0) {
            params->phiRef = atof(argv[++i]);
        } else if (strcmp(argv[i], "--m1") == 0) {
            params->m1 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--m2") == 0) {
            params->m2 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--distance") == 0) {
            params->distance = atof(argv[++i]);
        } else if (strcmp(argv[i], "--ra") == 0) {
            params->ra = atof(argv[++i]);
        } else if (strcmp(argv[i], "--dec") == 0) {
            params->dec = atof(argv[++i]);
        } else if (strcmp(argv[i], "--inclination") == 0) {
            params->inclination = atof(argv[++i]);
        } else if (strcmp(argv[i], "--polarization") == 0) {
            params->polarization = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fRef") == 0) {
            params->fRef = atof(argv[++i]);
        } else if (strcmp(argv[i], "--nbmode") == 0) {
            params->nbmode = atof(argv[++i]);
        } else if (strcmp(argv[i], "--deltaT") == 0) {
            prior->deltaT = atof(argv[++i]);
        } else if (strcmp(argv[i], "--comp-min") == 0) {
            prior->comp_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--comp-max") == 0) {
            prior->comp_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--mtot-min") == 0) {
            prior->mtot_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--mtot-max") == 0) {
            prior->mtot_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--q-max") == 0) {
            prior->qmax = atof(argv[++i]);
        } else if (strcmp(argv[i], "--dist-min") == 0) {
            prior->dist_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--dist-max") == 0) {
            prior->dist_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--eff") == 0) {
            run->eff = atof(argv[++i]);
        } else if (strcmp(argv[i], "--tol") == 0) {
            run->tol = atof(argv[++i]);
        } else if (strcmp(argv[i], "--nlive") == 0) {
            run->nlive = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--bambi") == 0) {
            run->bambi = 1;
        } else if (strcmp(argv[i], "--resume") == 0) {
            run->resume = 1;
        } else if (strcmp(argv[i], "--outroot") == 0) {
            strcpy(run->outroot, argv[++i]);
        } else if (strcmp(argv[i], "--netfile") == 0) {
            strcpy(run->netfile, argv[++i]);
        } else {
            printf("Error: invalid option: %s\n", argv[i]);
            goto fail;
        }
    }

    return;

    fail:
    exit(1);
}

/* Function generating a LLV signal from LLV parameters */
int LLVGenerateSignal(
  struct tagLLVParams* params,   /* Input: set of LLV parameters of the signal */
  struct tagLLVSignal* signal)   /* Output: structure for the generated signal */
{
  int ret;
  ListmodesCAmpPhaseFrequencySeries* listROM = NULL;
  ListmodesCAmpPhaseFrequencySeries* listLHO = NULL;
  ListmodesCAmpPhaseFrequencySeries* listLLO = NULL;
  ListmodesCAmpPhaseFrequencySeries* listVIRGO = NULL;

  /* Checking that the global injectedparams has been set up */
  if (!injectedparams) {
    printf("Error: when calling LLVGenerateSignal, injectedparams points to NULL.\n");
    exit(1);
  }
  /* Should add more error checking ? */
  /* Generate the waveform with the ROM */
  /* Note: SimEOBNRv2HMROM accepts masses and distances in SI units, whereas LLV params is in solar masses and Mpc */
  ret = SimEOBNRv2HMROM(&listROM, params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, params->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);

  /* If the ROM waveform generation failed (e.g. parameters were out of bounds) return FAILURE */
  if(ret==FAILURE) return FAILURE;

  /* Process the waveform through the LLV response */
  LLVSimFDResponse(&listROM, &listLHO, params->tRef, params->ra, params->dec, params->inclination, params->polarization, LHO);
  LLVSimFDResponse(&listROM, &listLLO, params->tRef, params->ra, params->dec, params->inclination, params->polarization, LLO);
  LLVSimFDResponse(&listROM, &listVIRGO, params->tRef, params->ra, params->dec, params->inclination, params->polarization, VIRGO);

  /* Precompute the inner products (h|h) and (s|s) */
  double LHOhh = FDListmodesOverlap(listLHO, listLHO, NoiseSnLHO, __LLVSimFD_LHONoise_fLow, __LLVSimFD_LHONoise_fHigh);
  double LLOhh = FDListmodesOverlap(listLLO, listLLO, NoiseSnLLO, __LLVSimFD_LLONoise_fLow, __LLVSimFD_LLONoise_fHigh);
  double VIRGOhh = FDListmodesOverlap(listVIRGO, listVIRGO, NoiseSnVIRGO, __LLVSimFD_VIRGONoise_fLow, __LLVSimFD_VIRGONoise_fHigh);

  /* Output and clean up */
  signal->LHOSignal = listLHO;
  signal->LLOSignal = listLLO;
  signal->VIRGOSignal = listVIRGO;
  signal->LHOhh = LHOhh;
  signal->LLOhh = LLOhh;
  signal->VIRGOhh = VIRGOhh;

  ListmodesCAmpPhaseFrequencySeries_Destroy(listROM);
  return SUCCESS;
}

/* Function to check that returned parameter values fit in prior boundaries */
int PriorBoundaryCheck(LLVPrior *prior, double *Cube)
{
	if (Cube[0] < prior->comp_min || Cube[0] > prior->comp_max ||
	 	Cube[1] < prior->comp_min || Cube[1] > prior->comp_max)
	 	return 1;

	if (Cube[0] + Cube[1] < prior->mtot_min || Cube[0] + Cube[1] > prior->mtot_max)
		return 1;

	if (Cube[0] < Cube[1] || Cube[0] / Cube[1] > prior->qmax)
		return 1;

	return 0;
}

/* Utility prior functions to convert from Cube to common distributions */

double CubeToFlatPrior(double r, double x1, double x2)
{
    return x1 + r * ( x2 - x1 );
}

double CubeToLogFlatPrior(double r, double x1, double x2)
{
    double lx1, lx2;
    lx1 = log( x1 );
    lx2 = log( x2 );
    return exp( lx1 + r * ( lx2 - lx1 ) );
}

double CubeToPowerPrior(double p, double r, double x1, double x2)
{
    double pp = p + 1.0;
    return pow(r * pow(x2, pp) + (1.0 - r) * pow(x1, pp), 1.0 / pp);
}

double CubeToGaussianPrior(double r, double mean, double sigma)
{
    return gsl_cdf_gaussian_Pinv(r,sigma) + mean;
}

double CubeToSinPrior(double r, double x1, double x2)
{
    return acos((1.0-r)*cos(x1)+r*cos(x2));
}

double CubeToCosPrior(double r, double x1, double x2)
{
    return asin((1.0-r)*sin(x1)+r*sin(x2));
}
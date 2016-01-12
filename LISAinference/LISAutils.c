#include "LISAutils.h"

/************ Global Parameters ************/

LISAParams* injectedparams = NULL;
LISAGlobalParams* globalparams = NULL;
LISAPrior* priorParams = NULL;

/************ Functions to initalize and clean up structure for the signals ************/

void LISASignalCAmpPhase_Cleanup(LISASignalCAmpPhase* signal) {
  if(signal->TDI1Signal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->TDI1Signal);
  if(signal->TDI2Signal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->TDI2Signal);
  if(signal->TDI3Signal) ListmodesCAmpPhaseFrequencySeries_Destroy(signal->TDI3Signal);
  free(signal);
}

void LISASignalCAmpPhase_Init(LISASignalCAmpPhase** signal) {
  if(!signal) exit(1);
  /* Create storage for structures */
  if(!*signal) *signal = malloc(sizeof(LISASignalCAmpPhase));
  else
  {
    LISASignalCAmpPhase_Cleanup(*signal);
  }
  (*signal)->TDI1Signal = NULL;
  (*signal)->TDI2Signal = NULL;
  (*signal)->TDI3Signal = NULL;
}

void LISAInjectionCAmpPhase_Cleanup(LISAInjectionCAmpPhase* signal) {
  if(signal->TDI1Splines) ListmodesCAmpPhaseSpline_Destroy(signal->TDI1Splines);
  if(signal->TDI2Splines) ListmodesCAmpPhaseSpline_Destroy(signal->TDI2Splines);
  if(signal->TDI3Splines) ListmodesCAmpPhaseSpline_Destroy(signal->TDI3Splines);
  free(signal);
}

void LISAInjectionCAmpPhase_Init(LISAInjectionCAmpPhase** signal) {
  if(!signal) exit(1);
  /* Create storage for structures */
  if(!*signal) *signal = malloc(sizeof(LISAInjectionCAmpPhase));
  else
  {
    LISAInjectionCAmpPhase_Cleanup(*signal);
  }
  (*signal)->TDI1Splines = NULL;
  (*signal)->TDI2Splines = NULL;
  (*signal)->TDI3Splines = NULL;
}

void LISASignalReIm_Cleanup(LISASignalReIm* signal) {
  if(signal->TDI1Signal) ReImFrequencySeries_Cleanup(signal->TDI1Signal);
  if(signal->TDI2Signal) ReImFrequencySeries_Cleanup(signal->TDI2Signal);
  if(signal->TDI3Signal) ReImFrequencySeries_Cleanup(signal->TDI3Signal);
  free(signal);
}

void LISASignalReIm_Init(LISASignalReIm** signal) {
  if(!signal) exit(1);
  /* Create storage for structures */
  if(!*signal) *signal = malloc(sizeof(LISASignalReIm));
  else
  {
    LISASignalReIm_Cleanup(*signal);
  }
  (*signal)->TDI1Signal = NULL;
  (*signal)->TDI2Signal = NULL;
  (*signal)->TDI3Signal = NULL;
}

void LISAInjectionReIm_Cleanup(LISAInjectionReIm* signal) {
  if(signal->TDI1Signal) ReImFrequencySeries_Cleanup(signal->TDI1Signal);
  if(signal->TDI2Signal) ReImFrequencySeries_Cleanup(signal->TDI2Signal);
  if(signal->TDI3Signal) ReImFrequencySeries_Cleanup(signal->TDI3Signal);
  if(signal->freq) gsl_vector_free(signal->freq);
  if(signal->noisevalues1) gsl_vector_free(signal->noisevalues1);
  if(signal->noisevalues2) gsl_vector_free(signal->noisevalues2);
  if(signal->noisevalues3) gsl_vector_free(signal->noisevalues3);
  free(signal);
}

void LISAInjectionReIm_Init(LISAInjectionReIm** signal) {
  if(!signal) exit(1);
  /* Create storage for structures */
  if(!*signal) *signal = malloc(sizeof(LISAInjectionReIm));
  else
  {
    LISAInjectionReIm_Cleanup(*signal);
  }
  (*signal)->TDI1Signal = NULL;
  (*signal)->TDI2Signal = NULL;
  (*signal)->TDI3Signal = NULL;
  (*signal)->freq = NULL;
  (*signal)->noisevalues1 = NULL;
  (*signal)->noisevalues2 = NULL;
  (*signal)->noisevalues3 = NULL;
}


/************ Parsing arguments function ************/

/* Parse command line to initialize LISAParams, LISAPrior, and LISARunParams objects */
void parse_args_LISA(ssize_t argc, char **argv, 
  LISAParams* params,
  LISAGlobalParams* globalparams, 
  LISAPrior* prior, 
  LISARunParams* run) 
{
    char help[] = "\
LISAInference by Sylvain Marsat, John Baker, and Philip Graff\n\
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
 --m1                  Component mass 1 in Solar masses (larger, default=2e6)\n\
 --m2                  Component mass 2 in Solar masses (smaller, default=1e6)\n\
 --distance            Distance to source in Mpc (default=1e9)\n\
 --lambda              First angle for the position in the sky (radians, default=0)\n\
 --beta                Second angle for the position in the sky (radians, default=0)\n\
 --inclination         Inclination of source orbital plane to observer line of sight\n\
                       (radians, default=PI/3)\n\
 --polarization        Polarization of source (radians, default=0)\n\
 --nbmode              Number of modes of radiation to generate (1-5, default=5)\n\
 --snr                 Use a target network SNR for the injection by rescaling distance\n\
\n\
-----------------------------------------------------------------\n\
----- Global Waveform/Inner products Parameters -----------------\n\
-----------------------------------------------------------------\n\
 --fRef                Reference frequency (Hz, default=0, interpreted as Mf=0.14)\n\
 --deltatobs           Observation time (years, default=2)\n\
 --fmin                Minimal frequency (Hz, default=0) - when set to 0, use the first frequency covered by the noise data of the detector\n\
 --nbmodeinj           Number of modes of radiation to use for the injection (1-5, default=5)\n\
 --nbmodetemp          Number of modes of radiation to use for the templates (1-5, default=5)\n\
 --tagint              Tag choosing the integrator: 0 for wip (default), 1 for linear integration\n\
 --tagtdi              Tag choosing the set of TDI variables to use (default TDIAETXYZ)\n\
 --nbptsoverlap        Number of points to use for linear integration (default 32768)\n\
\n\
--------------------------------------------------\n\
----- Prior Boundary Settings --------------------\n\
--------------------------------------------------\n\
 --deltaT              Half-width of time prior (sec, default=1e5)\n\
 --comp-min            Minimum component mass in Solar masses (default=1e4)\n\
 --comp-max            Maximum component mass in Solar masses (default=1e8)\n\
 --mtot-min            Minimum total mass in Solar masses (default=5e4)\n\
 --mtot-max            Maximum total mass in Solar masses (default=1e8)\n\
 --q-max               Maximum mass ratio, m1/m2 (default=11.98, minimum is 1)\n\
 --dist-min            Minimum distance to source (Mpc, default=100)\n\
 --dist-max            Maximum distance to source (Mpc, default=40*1e3)\n\
 --rescale-distprior        In case a target SNR is given with --snr, rescale dist-min and dist-max accordingly\n\
Parameters lambda, beta, phase, pol, inc can also ge given min and max values (for testing)\n\
Syntax: --PARAM-min\n\
\n\
--------------------------------------------------\n\
----- Fix Parameters In Sampling -----------------\n\
--------------------------------------------------\n\
 --pin-PARAM           Pin indicated parameter to injected value\n\
 --fix-PARAM           Fix indicated parameter to specified value\n\
 Available parameter names are:\n\
   m1          Mass 1 (MSol)\n\
   m2          Mass 2 (MSol)\n\
   dist        Distance (luminosity, Mpc)\n\
   time        Reference time (GPS sec)\n\
   phase       Reference orbital phase (rad)\n\
   lambda      First angle for the position in the sky (rad)\n\
   beta        Second angle for the position in the sky (rad)\n\
   inc         Inclination of orbital plane to observer (rad)\n\
   pol         Polarization (rad)\n\
 Note: --pin-PARAM overrides --fix-PARAM\n\
\n\
--------------------------------------------------\n\
----- BAMBI Sampler Settings ---------------------\n\
--------------------------------------------------\n\
 --eff                 Target efficiency of sampling (default=0.1)\n\
 --tol                 Tolerance for evidence calculation convergence (default=0.5)\n\
 --nlive               Number of live points for sampling (default=1000)\n\
 --bambi               Use BAMBI's neural network logL learning (no option, default off)\n\
 --resume              Resume from a previous run (no option, default off)\n\
 --outroot             Root for output files (default='chains/LISAinference_')\n\
 --netfile             Neural network settings file if using --bambi (default='LISAinference.inp')\n\
 --mmodal              Use multimodal decomposition (no option, default off)\n\
 --maxcls              Max number of modes in multimodal decomposition (default 1)\n\
 --nclspar             Number of parameters to use for multimodal decomposition - in the order of the cube (default 1)\n\
 --ztol                In multimodal decomposition, modes with lnZ lower than ztol are ignored (default -1e90)\n\
 --seed                Seed the inference by setting one of the live points to the injection (no option, default off)\n\
\n";

    ssize_t i;

    /* set default values for the injection params */
    params->tRef = 0.;
    params->phiRef = 0.;
    params->m1 = 2*1e6;
    params->m2 = 1*1e6;
    params->distance = 40*1e3;
    params->lambda = 0.;
    params->beta = 0.;
    params->inclination = PI/3.;
    params->polarization = 0.;
    params->nbmode = 5;

    /* set default values for the global params */
    globalparams->fRef = 0.;
    globalparams->deltatobs = 2.;
    globalparams->fmin = 0.;
    globalparams->nbmodeinj = 5;
    globalparams->nbmodetemp = 5;
    globalparams->tagint = 0;
    globalparams->tagtdi = TDIAETXYZ;
    globalparams->nbptsoverlap = 32768;

    /* set default values for the prior limits */
    prior->deltaT = 3600.;
    prior->comp_min = 1e4;
    prior->comp_max = 1e8;
    prior->mtot_min = 5e4;
    prior->mtot_max = 1e8;
    prior->qmax = 11.98;
    prior->dist_min = 1e3;
    prior->dist_max = 400*1e3;
    prior->lambda_min = 0.;
    prior->lambda_max = 2*PI;
    prior->beta_min = -PI/2.;
    prior->beta_max = PI/2.;
    prior->phase_min = 0.;
    prior->phase_max = 2*PI;
    prior->pol_min = 0.;
    prior->pol_max = 2*PI;
    prior->inc_min = 0.;
    prior->inc_max = PI;
    prior->fix_m1 = NAN;
    prior->fix_m2 = NAN;
    prior->fix_dist = NAN;
    prior->fix_time = NAN;
    prior->fix_phase = NAN;
    prior->fix_pol = NAN;
    prior->fix_lambda = NAN;
    prior->fix_beta = NAN;
    prior->fix_inc = NAN;
    prior->pin_m1 = 0;
    prior->pin_m2 = 0;
    prior->pin_dist = 0;
    prior->pin_time = 0;
    prior->pin_phase = 0;
    prior->pin_pol = 0;
    prior->pin_lambda = 0;
    prior->pin_beta = 0;
    prior->pin_inc = 0;
    prior->snr_target = NAN;
    prior->rescale_distprior = 0;
    prior->flat_distprior = 0;

    /* set default values for the run settings */
    run->eff = 0.1;
    run->tol = 0.5;
    run->nlive = 1000;
    strcpy(run->outroot, "chains/LISAinference_");
    run->bambi = 0;
    run->resume = 0;
    strcpy(run->netfile, "LISAinference.inp");
    run->mmodal = 0;
    run->maxcls = 1;
    run->nclspar = 1;
    run->ztol = -1e90;
    run->seed = 0;

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
        } else if (strcmp(argv[i], "--lambda") == 0) {
            params->lambda = atof(argv[++i]);
        } else if (strcmp(argv[i], "--beta") == 0) {
            params->beta = atof(argv[++i]);
        } else if (strcmp(argv[i], "--inclination") == 0) {
            params->inclination = atof(argv[++i]);
        } else if (strcmp(argv[i], "--polarization") == 0) {
            params->polarization = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fRef") == 0) {
            globalparams->fRef = atof(argv[++i]);
        } else if (strcmp(argv[i], "--deltatobs") == 0) {
            globalparams->deltatobs = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fmin") == 0) {
            globalparams->fmin = atof(argv[++i]);
        } else if (strcmp(argv[i], "--nbmodeinj") == 0) {
            globalparams->nbmodeinj = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--nbmodetemp") == 0) {
            globalparams->nbmodetemp = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tagint") == 0) {
            globalparams->tagint = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tagtdi") == 0) {
            globalparams->tagtdi = ParseTDItag(argv[++i]);
        } else if (strcmp(argv[i], "--nbptsoverlap") == 0) {
            globalparams->nbptsoverlap = atoi(argv[++i]);
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
        } else if (strcmp(argv[i], "--lambda-min") == 0) {
            prior->lambda_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--lambda-max") == 0) {
            prior->lambda_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--beta-min") == 0) {
            prior->beta_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--beta-max") == 0) {
            prior->beta_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--phase-min") == 0) {
            prior->phase_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--phase-max") == 0) {
            prior->phase_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--pol-min") == 0) {
            prior->pol_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--pol-max") == 0) {
            prior->pol_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--inc-min") == 0) {
            prior->inc_min = atof(argv[++i]);
        } else if (strcmp(argv[i], "--inc-max") == 0) {
            prior->inc_max = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-m1") == 0) {
            prior->fix_m1 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-m2") == 0) {
            prior->fix_m2 = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-dist") == 0) {
            prior->fix_dist = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-lambda") == 0) {
            prior->fix_lambda = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-beta") == 0) {
            prior->fix_beta = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-time") == 0) {
            prior->fix_time = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-phase") == 0) {
            prior->fix_phase = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-inc") == 0) {
            prior->fix_inc = atof(argv[++i]);
        } else if (strcmp(argv[i], "--fix-pol") == 0) {
            prior->fix_pol = atof(argv[++i]);
        } else if (strcmp(argv[i], "--pin-m1") == 0) {
            prior->pin_m1 = 1;
        } else if (strcmp(argv[i], "--pin-m2") == 0) {
            prior->pin_m2 = 1;
        } else if (strcmp(argv[i], "--pin-dist") == 0) {
            prior->pin_dist = 1;
        } else if (strcmp(argv[i], "--pin-lambda") == 0) {
            prior->pin_lambda = 1;
        } else if (strcmp(argv[i], "--pin-beta") == 0) {
            prior->pin_beta = 1;
        } else if (strcmp(argv[i], "--pin-time") == 0) {
            prior->pin_time = 1;
        } else if (strcmp(argv[i], "--pin-phase") == 0) {
            prior->pin_phase = 1;
        } else if (strcmp(argv[i], "--pin-inc") == 0) {
            prior->pin_inc = 1;
        } else if (strcmp(argv[i], "--pin-pol") == 0) {
            prior->pin_pol = 1;
        } else if (strcmp(argv[i], "--snr") == 0) {
            prior->snr_target = atof(argv[++i]);
	} else if (strcmp(argv[i], "--rescale-distprior") == 0) {
            prior->rescale_distprior = 1;
	} else if (strcmp(argv[i], "--flat-distprior") == 0) {
            prior->flat_distprior = 1;
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
        } else if (strcmp(argv[i], "--mmodal") == 0) {
            run->mmodal = 1;
        } else if (strcmp(argv[i], "--maxcls") == 0) {
            run->maxcls = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--nclspar") == 0) {
            run->nclspar = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ztol") == 0) {
            run->ztol = atof(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0) {
            run->seed = 1;
        } else {
            printf("Error: invalid option: %s\n", argv[i]);
            goto fail;
        }
    }

    return;

    fail:
    exit(1);
}

/* Function printing all parameters of the run to an output file for future reference */
int print_parameters_to_file_LISA(
                     LISAParams* params,
		     LISAGlobalParams* globalparams,
		     LISAPrior* prior,
		     LISARunParams* run)
{
  /* Output file */
  char *path=malloc(strlen(run->outroot)+64);
  sprintf(path,"%sparams.txt", run->outroot);
  FILE *f = fopen(path, "w");

  /* Print injection parameters (before possible rescaling of the distances to match the target snr) */
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "Injection parameters:\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "m1:           %.16e\n", params->tRef);
  fprintf(f, "m2:           %.16e\n", params->phiRef);
  fprintf(f, "tRef:         %.16e\n", params->tRef);
  fprintf(f, "phiRef:       %.16e\n", params->phiRef);
  fprintf(f, "distance:     %.16e\n", params->distance);
  fprintf(f, "lambda:       %.16e\n", params->lambda);
  fprintf(f, "beta:         %.16e\n", params->beta);
  fprintf(f, "inclination:  %.16e\n", params->inclination);
  fprintf(f, "polarization: %.16e\n", params->polarization);
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "\n");

  /* Print global parameters */
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "Global parameters:\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "fRef:         %.16e\n", globalparams->fRef);
  fprintf(f, "deltatobs:    %.16e\n", globalparams->deltatobs);
  fprintf(f, "fmin:         %.16e\n", globalparams->fmin);
  fprintf(f, "nbmodeinj:    %d\n", globalparams->nbmodeinj);
  fprintf(f, "nbmodetemp:   %d\n", globalparams->nbmodetemp);
  fprintf(f, "tagint:       %d\n", globalparams->tagint);
  fprintf(f, "tagtdi:       %d\n", globalparams->tagtdi); //Translation back from enum to string not implemented yet
  fprintf(f, "nbptsoverlap: %d\n", globalparams->nbptsoverlap);
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "\n");

  /* Print prior parameters */
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "Prior parameters:\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "deltaT:            %.16e\n", prior->deltaT);
  fprintf(f, "comp_min:          %.16e\n", prior->comp_min);
  fprintf(f, "comp_max:          %.16e\n", prior->comp_max);
  fprintf(f, "mtot_min:          %.16e\n", prior->mtot_min);
  fprintf(f, "mtot_max:          %.16e\n", prior->mtot_max);
  fprintf(f, "qmax:              %.16e\n", prior->qmax);
  fprintf(f, "dist_min:          %.16e\n", prior->dist_min);
  fprintf(f, "dist_max:          %.16e\n", prior->dist_max);
  fprintf(f, "lambda_min:        %.16e\n", prior->lambda_min);
  fprintf(f, "lambda_max:        %.16e\n", prior->lambda_max);
  fprintf(f, "beta_min:          %.16e\n", prior->beta_min);
  fprintf(f, "beta_max:          %.16e\n", prior->beta_max);
  fprintf(f, "phase_min:         %.16e\n", prior->phase_min);
  fprintf(f, "phase_max:         %.16e\n", prior->phase_max);
  fprintf(f, "pol_min:           %.16e\n", prior->pol_min);
  fprintf(f, "pol_max:           %.16e\n", prior->pol_max);
  fprintf(f, "inc_min:           %.16e\n", prior->inc_min);
  fprintf(f, "inc_max:           %.16e\n", prior->inc_max);
  fprintf(f, "fix_m1:            %.16e\n", prior->fix_m1);
  fprintf(f, "fix_m2:            %.16e\n", prior->fix_m2);
  fprintf(f, "fix_dist:          %.16e\n", prior->fix_dist);
  fprintf(f, "fix_time:          %.16e\n", prior->fix_time);
  fprintf(f, "fix_phase:         %.16e\n", prior->fix_phase);
  fprintf(f, "fix_pol:           %.16e\n", prior->fix_pol);
  fprintf(f, "fix_lambda:        %.16e\n", prior->fix_lambda);
  fprintf(f, "fix_beta:          %.16e\n", prior->fix_beta);
  fprintf(f, "fix_inc:           %.16e\n", prior->fix_inc);
  fprintf(f, "pin_m1:            %d\n", prior->pin_m1);
  fprintf(f, "pin_m2:            %d\n", prior->pin_m2);
  fprintf(f, "pin_dist:          %d\n", prior->pin_dist);
  fprintf(f, "pin_time:          %d\n", prior->pin_time);
  fprintf(f, "pin_phase:         %d\n", prior->pin_phase);
  fprintf(f, "pin_pol:           %d\n", prior->pin_pol);
  fprintf(f, "pin_lambda:        %d\n", prior->pin_lambda);
  fprintf(f, "pin_beta:          %d\n", prior->pin_beta);
  fprintf(f, "pin_inc:           %d\n", prior->pin_inc);
  fprintf(f, "snr_target:        %.16e\n", prior->snr_target);
  fprintf(f, "rescale_distprior: %d\n", prior->rescale_distprior);
  fprintf(f, "flat_distprior:    %d\n", prior->flat_distprior);
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "\n");

  /* Print run parameters */
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "Run parameters:\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "eff:     %g\n", run->eff);
  fprintf(f, "tol:     %g\n", run->tol);
  fprintf(f, "nlive:   %d\n", run->nlive);
  fprintf(f, "bambi:   %d\n", run->bambi);
  fprintf(f, "resume:  %d\n", run->resume);
  fprintf(f, "mmodal:  %d\n", run->mmodal);
  fprintf(f, "maxcls:  %d\n", run->maxcls);
  fprintf(f, "nclspar: %d\n", run->nclspar);
  fprintf(f, "ztol:    %g\n", run->ztol);
  fprintf(f, "seed:    %d\n", run->seed);
  fprintf(f, "-----------------------------------------------\n");

  /* Close output file */
  fclose(f);
  return SUCCESS;
}

/* Function printing distance parameters (used if they have been rescaled to a target snr) */
int print_rescaleddist_to_file_LISA(
                     LISAParams* params,
		     LISAGlobalParams* globalparams,
		     LISAPrior* prior,
		     LISARunParams* run)
{
  /* Output file */
  char *path=malloc(strlen(run->outroot)+64);
  sprintf(path,"%sparams.txt", run->outroot);
  FILE *f = fopen(path, "a");

  /* Print rescaled distance and dist prior */
  fprintf(f, "\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "Rescaled dist parameters:\n");
  fprintf(f, "-----------------------------------------------\n");
  fprintf(f, "distance: %.16e\n", params->distance);
  fprintf(f, "dist_min: %.16e\n", prior->dist_min);
  fprintf(f, "dist_max: %.16e\n", prior->dist_max);
  fprintf(f, "-----------------------------------------------\n");

  /* Close output file */
  fclose(f);
  return SUCCESS;
}

/***************************** Functions to generate signals and compute likelihoods ******************************/

/* Function generating a LISA signal as a list of modes in CAmp/Phase form, from LISA parameters */
int LISAGenerateSignalCAmpPhase(
  struct tagLISAParams* params,            /* Input: set of LISA parameters of the signal */
  struct tagLISASignalCAmpPhase* signal)   /* Output: structure for the generated signal */
{
  int ret;
  ListmodesCAmpPhaseFrequencySeries* listROM = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI1 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI2 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI3 = NULL;

  /* Checking that the global injectedparams has been set up */
  if (!injectedparams) {
    printf("Error: when calling LISAGenerateSignal, injectedparams points to NULL.\n");
    exit(1);
  }
  /* Should add more error checking ? */
  /* Generate the waveform with the ROM */
  /* Note: SimEOBNRv2HMROM accepts masses and distances in SI units, whereas LISA params is in solar masses and Mpc */
  ret = SimEOBNRv2HMROM(&listROM, params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, globalparams->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);

  //
  //printf("%d|%g|%g|%g|%g|%g|%g\n", params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, globalparams->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);

  /* If the ROM waveform generation failed (e.g. parameters were out of bounds) return FAILURE */
  if(ret==FAILURE) return FAILURE;

  /* Process the waveform through the LISA response */
  //WARNING: tRef is ignored for now, i.e. set to 0
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  LISASimFDResponseTDI3Chan(&listROM, &listTDI1, &listTDI2, &listTDI3, params->tRef, params->lambda, params->beta, params->inclination, params->polarization, globalparams->tagtdi);
  //tend = clock();
  //printf("time LISASimFDResponse: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  //
  //params->distance = 58667.9;
  //params->tRef = 0;
  //params->phiRef = 0;
  //params->m1 = 1.5e6;
  //params->m2 = 0.5e6;
  //params->lambda = 1.7;
  //params->beta = PI/3;
  //params->inclination = PI/3;
  //params->polarization = 1.2;
  //ret = SimEOBNRv2HMROM(&listROM, params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, globalparams->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);
  //LISASimFDResponseTDIAET(&listROM, &listTDIA, &listTDIE, &listTDIT, params->tRef, params->lambda, params->beta, params->inclination, params->polarization);
  /* printf("Params in LISAGenerateSignalCAmpPhase:\n"); */
  /* printf("params->nbmode: %d\n", params->nbmode); */
  /* printf("params->deltatRef: %g\n", params->tRef - injectedparams->tRef); */
  /* printf("params->phiRef: %g\n", params->phiRef); */
  /* printf("globalparams->fRef: %g\n", globalparams->fRef); */
  /* printf("params->m1: %g\n", params->m1); */
  /* printf("params->m2: %g\n", params->m2); */
  /* printf("params->distance: %g\n", params->distance); */
  /* printf("params->tRef: %g\n", params->tRef); */
  /* printf("params->lambda: %g\n", params->lambda); */
  /* printf("params->beta: %g\n", params->beta); */
  /* printf("params->inclination: %g\n", params->inclination); */
  /* printf("params->polarization: %g\n", params->polarization); */

  /* Pre-interpolate the injection, building the spline matrices */
  ListmodesCAmpPhaseSpline* listsplinesgen1 = NULL;
  ListmodesCAmpPhaseSpline* listsplinesgen2 = NULL;
  ListmodesCAmpPhaseSpline* listsplinesgen3 = NULL;
  BuildListmodesCAmpPhaseSpline(&listsplinesgen1, listTDI1);
  BuildListmodesCAmpPhaseSpline(&listsplinesgen2, listTDI2);
  BuildListmodesCAmpPhaseSpline(&listsplinesgen3, listTDI3);

  /* Precompute the inner product (h|h) - takes into account the length of the observation with deltatobs */
  double Mfstartobs = NewtonianfoftGeom(params->m1 / params->m2, (globalparams->deltatobs * YRSID_SI) / ((params->m1 + params->m2) * MTSUN_SI));
  double fstartobs = Mfstartobs / ((params->m1 + params->m2) * MTSUN_SI);
  double fLow = fmax(__LISASimFD_Noise_fLow, globalparams->fmin);
  double fHigh = __LISASimFD_Noise_fHigh;
  RealFunctionPtr NoiseSn1 = NoiseFunction(globalparams->tagtdi, 1);
  RealFunctionPtr NoiseSn2 = NoiseFunction(globalparams->tagtdi, 2);
  RealFunctionPtr NoiseSn3 = NoiseFunction(globalparams->tagtdi, 3);
  //TESTING
  //tbeg = clock();
  double TDI123hh = FDListmodesFresnelOverlap3Chan(listTDI1, listTDI2, listTDI3, listsplinesgen1, listsplinesgen2, listsplinesgen3, NoiseSn1, NoiseSn2, NoiseSn3, fLow, fHigh, fstartobs, fstartobs);
  //tend = clock();
  //printf("time SNRs: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);

  //
  //printf("fstartobs inside LISAGenerateSignalCAmpPhase: %g\n", fstartobs);
/*   Write_Text_Vector("/Users/marsat/src/flare/test/testfresnel/temp4", "signalAfreq.txt", ListmodesCAmpPhaseFrequencySeries_GetMode(listTDIA, 2, 2)->freqseries->freq); */
/* Write_Text_Vector("/Users/marsat/src/flare/test/testlisaoverlap/temp4", "signalAampreal.txt", ListmodesCAmpPhaseFrequencySeries_GetMode(listTDIA, 2, 2)->freqseries->amp_real); */
/* Write_Text_Vector("/Users/marsat/src/flare/test/testlisaoverlap/temp4", "signalAampimag.txt", ListmodesCAmpPhaseFrequencySeries_GetMode(listTDIA, 2, 2)->freqseries->amp_imag); */
/* Write_Text_Vector("/Users/marsat/src/flare/test/testlisaoverlap/temp4", "signalAphase.txt", ListmodesCAmpPhaseFrequencySeries_GetMode(listTDIA, 2, 2)->freqseries->phase); */
/*   printf("TDIAEThh inside LISAGenerateSignalCAmpPhase: %g\n", TDIAEThh); */

  /* Output and clean up */
  signal->TDI1Signal = listTDI1;
  signal->TDI2Signal = listTDI2;
  signal->TDI3Signal = listTDI3;
  signal->TDI123hh = TDI123hh;

  ListmodesCAmpPhaseFrequencySeries_Destroy(listROM);
  ListmodesCAmpPhaseSpline_Destroy(listsplinesgen1);
  ListmodesCAmpPhaseSpline_Destroy(listsplinesgen2);
  ListmodesCAmpPhaseSpline_Destroy(listsplinesgen3);
  return SUCCESS;
}

/* Function generating a LISA signal as a list of modes in CAmp/Phase form, from LISA parameters */
int LISAGenerateInjectionCAmpPhase(
  struct tagLISAParams* params,       /* Input: set of LISA parameters of the signal */
  struct tagLISAInjectionCAmpPhase* signal)   /* Output: structure for the injected signal */
{
  int ret;
  ListmodesCAmpPhaseFrequencySeries* listROM = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI1 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI2 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI3 = NULL;

  /* Should add more error checking ? */
  /* Generate the waveform with the ROM */
  /* Note: SimEOBNRv2HMROM accepts masses and distances in SI units, whereas LISA params is in solar masses and Mpc */
  ret = SimEOBNRv2HMROM(&listROM, params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, globalparams->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);

  /* If the ROM waveform generation failed (e.g. parameters were out of bounds) return FAILURE */
  if(ret==FAILURE) return FAILURE;

  /* Process the waveform through the LISA response */
  //WARNING: tRef is ignored for now, i.e. set to 0
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  LISASimFDResponseTDI3Chan(&listROM, &listTDI1, &listTDI2, &listTDI3, params->tRef, params->lambda, params->beta, params->inclination, params->polarization, globalparams->tagtdi);
  //tend = clock();
  //printf("time LISASimFDResponse: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  //
  /* printf("Params in LISAGenerateInjectionCAmpPhase:\n"); */
  /* printf("params->nbmode: %d\n", params->nbmode); */
  /* printf("params->deltatRef: %g\n", params->tRef - injectedparams->tRef); */
  /* printf("params->phiRef: %g\n", params->phiRef); */
  /* printf("globalparams->fRef: %g\n", globalparams->fRef); */
  /* printf("params->m1: %g\n", params->m1); */
  /* printf("params->m2: %g\n", params->m2); */
  /* printf("params->distance: %g\n", params->distance); */
  /* printf("params->tRef: %g\n", params->tRef); */
  /* printf("params->lambda: %g\n", params->lambda); */
  /* printf("params->beta: %g\n", params->beta); */
  /* printf("params->inclination: %g\n", params->inclination); */
  /* printf("params->polarization: %g\n", params->polarization); */

  /* Pre-interpolate the injection, building the spline matrices */
  ListmodesCAmpPhaseSpline* listsplinesinj1 = NULL;
  ListmodesCAmpPhaseSpline* listsplinesinj2 = NULL;
  ListmodesCAmpPhaseSpline* listsplinesinj3 = NULL;
  BuildListmodesCAmpPhaseSpline(&listsplinesinj1, listTDI1);
  BuildListmodesCAmpPhaseSpline(&listsplinesinj2, listTDI2);
  BuildListmodesCAmpPhaseSpline(&listsplinesinj3, listTDI3);

  /* Precompute the inner product (h|h) - takes into account the length of the observation with deltatobs */
  double Mfstartobs = NewtonianfoftGeom(injectedparams->m1 / injectedparams->m2, (globalparams->deltatobs * YRSID_SI) / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI));
  double fstartobs = Mfstartobs / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI);
  double fLow = fmax(__LISASimFD_Noise_fLow, globalparams->fmin);
  double fHigh = __LISASimFD_Noise_fHigh;
  RealFunctionPtr NoiseSn1 = NoiseFunction(globalparams->tagtdi, 1);
  RealFunctionPtr NoiseSn2 = NoiseFunction(globalparams->tagtdi, 2);
  RealFunctionPtr NoiseSn3 = NoiseFunction(globalparams->tagtdi, 3);
  //TESTING
  //tbeg = clock();
  double TDI123ss = FDListmodesFresnelOverlap3Chan(listTDI1, listTDI2, listTDI3, listsplinesinj1, listsplinesinj2, listsplinesinj3, NoiseSn1, NoiseSn2, NoiseSn3, fLow, fHigh, fstartobs, fstartobs);
  //tend = clock();
  //printf("time SNRs: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);

  //
  //printf("fstartobs inside LISAGenerateInjectionCAmpPhase: %g\n", fstartobs);
  //printf("TDIAETss inside LISAGenerateInjectionCAmpPhase: %g\n", TDIAETss);

  /* Output and clean up */
  signal->TDI1Splines = listsplinesinj1;
  signal->TDI2Splines = listsplinesinj2;
  signal->TDI3Splines = listsplinesinj3;
  signal->TDI123ss = TDI123ss;

  ListmodesCAmpPhaseFrequencySeries_Destroy(listROM);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI1);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI2);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI3);

  return SUCCESS;
}

/* Function generating a LISA signal as a frequency series in Re/Im form where the modes have been summed, from LISA parameters - takes as argument the frequencies on which to evaluate */
int LISAGenerateSignalReIm(
  struct tagLISAParams* params,       /* Input: set of LISA parameters of the template */
  gsl_vector* freq,                   /* Input: frequencies on which evaluating the waveform (from the injection) */
  struct tagLISASignalReIm* signal)   /* Output: structure for the generated signal */
{
  int ret;
  ListmodesCAmpPhaseFrequencySeries* listROM = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI1 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI2 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI3 = NULL;

  /* Checking that the global injectedparams has been set up */
  if (!injectedparams) {
    printf("Error: when calling LISAGenerateSignalReIm, injectedparams points to NULL.\n");
    exit(1);
  }
  /* Should add more error checking ? */
  /* Generate the waveform with the ROM */
  /* Note: SimEOBNRv2HMROM accepts masses and distances in SI units, whereas LISA params is in solar masses and Mpc */
  ret = SimEOBNRv2HMROM(&listROM, params->nbmode, params->tRef - injectedparams->tRef, params->phiRef, globalparams->fRef, (params->m1)*MSUN_SI, (params->m2)*MSUN_SI, (params->distance)*1e6*PC_SI);

  /* If the ROM waveform generation failed (e.g. parameters were out of bounds) return FAILURE */
  if(ret==FAILURE) return FAILURE;

  /* Process the waveform through the LISA response */
  //WARNING: tRef is ignored for now, i.e. set to 0
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  LISASimFDResponseTDI3Chan(&listROM, &listTDI1, &listTDI2, &listTDI3, params->tRef, params->lambda, params->beta, params->inclination, params->polarization, globalparams->tagtdi);
  //tend = clock();
  //printf("time LISASimFDResponse: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* Initialize structures for the ReIm frequency series */
  int nbpts = (int) freq->size;
  ReImFrequencySeries* TDI1 = NULL;
  ReImFrequencySeries* TDI2 = NULL;
  ReImFrequencySeries* TDI3 = NULL;
  ReImFrequencySeries_Init(&TDI1, nbpts);
  ReImFrequencySeries_Init(&TDI2, nbpts);
  ReImFrequencySeries_Init(&TDI3, nbpts);

  /* Compute the Re/Im frequency series - takes into account the length of the observation with deltatobs */
  double Mfstartobs = NewtonianfoftGeom(params->m1 / params->m2, (globalparams->deltatobs * YRSID_SI) / ((params->m1 + params->m2) * MTSUN_SI));
  double fstartobs = Mfstartobs / ((params->m1 + params->m2) * MTSUN_SI);
  //TESTING
  //tbeg = clock();
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI1, listTDI1, freq, fstartobs);
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI2, listTDI2, freq, fstartobs);
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI3, listTDI3, freq, fstartobs);
  //tend = clock();
  //printf("time ReIm: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* Output and clean up */
  signal->TDI1Signal = TDI1;
  signal->TDI2Signal = TDI2;
  signal->TDI3Signal = TDI3;

  ListmodesCAmpPhaseFrequencySeries_Destroy(listROM);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI1);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI2);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI3);
  return SUCCESS;
}

/* Function generating a LISA injection signal as a frequency series in Re/Im form where the modes have been summed, from LISA parameters - determines the frequencies */
int LISAGenerateInjectionReIm(
  struct tagLISAParams* injectedparams,      /* Input: set of LISA parameters of the template */
  double fLow,                               /* Input: additional lower frequency limit (argument fmin) */
  int nbpts,                                 /* Input: number of frequency samples */
  int tagsampling,                           /* Input: tag for using linear (0) or logarithmic (1) sampling */
  struct tagLISAInjectionReIm* injection)    /* Output: structure for the generated signal */
{
  int ret;
  ListmodesCAmpPhaseFrequencySeries* listROM = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI1 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI2 = NULL;
  ListmodesCAmpPhaseFrequencySeries* listTDI3 = NULL;

  /* Should add more error checking ? */
  /* Generate the waveform with the ROM */
  /* Note: SimEOBNRv2HMROM accepts masses and distances in SI units, whereas LISA params is in solar masses and Mpc */
  ret = SimEOBNRv2HMROM(&listROM, injectedparams->nbmode, 0., injectedparams->phiRef, globalparams->fRef, (injectedparams->m1)*MSUN_SI, (injectedparams->m2)*MSUN_SI, (injectedparams->distance)*1e6*PC_SI);

  /* If the ROM waveform generation failed (e.g. parameters were out of bounds) return FAILURE */
  if(ret==FAILURE) return FAILURE;

  /* Process the waveform through the LISA response */
  //WARNING: tRef is ignored for now, i.e. set to 0
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  LISASimFDResponseTDI3Chan(&listROM, &listTDI1, &listTDI2, &listTDI3, injectedparams->tRef, injectedparams->lambda, injectedparams->beta, injectedparams->inclination, injectedparams->polarization, globalparams->tagtdi);
  //tend = clock();
  //printf("time LISASimFDResponse: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* Determine the frequency vector - uses the fact that the detector limiting frequencies are the same in all channels - takes into account the length of the observation with deltatobs */
  gsl_vector* freq = gsl_vector_alloc(nbpts);
  double Mfstartobs = NewtonianfoftGeom(injectedparams->m1 / injectedparams->m2, (globalparams->deltatobs * YRSID_SI) / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI));
  double fstartobs = Mfstartobs / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI);
  double fLowCut = fmax(fmax(__LISASimFD_Noise_fLow, fLow), fstartobs);
  double fHigh = __LISASimFD_Noise_fHigh;
  ListmodesSetFrequencies(listROM, fLowCut, fHigh, nbpts, tagsampling, freq);

  /* Initialize structures for the ReIm frequency series */
  ReImFrequencySeries* TDI1 = NULL;
  ReImFrequencySeries* TDI2 = NULL;
  ReImFrequencySeries* TDI3 = NULL;
  ReImFrequencySeries_Init(&TDI1, nbpts);
  ReImFrequencySeries_Init(&TDI2, nbpts);
  ReImFrequencySeries_Init(&TDI3, nbpts);

  /* Compute the Re/Im frequency series */
  //TESTING
  //tbeg = clock();
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI1, listTDI1, freq, fstartobs);
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI2, listTDI2, freq, fstartobs);
  ReImFrequencySeries_SumListmodesCAmpPhaseFrequencySeries(TDI3, listTDI3, freq, fstartobs);
  //tend = clock();
  //printf("time ReIm: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* Compute the noise values */
  RealFunctionPtr NoiseSn1 = NoiseFunction(globalparams->tagtdi, 1);
  RealFunctionPtr NoiseSn2 = NoiseFunction(globalparams->tagtdi, 2);
  RealFunctionPtr NoiseSn3 = NoiseFunction(globalparams->tagtdi, 3);
  gsl_vector* noisevalues1 = gsl_vector_alloc(nbpts);
  gsl_vector* noisevalues2 = gsl_vector_alloc(nbpts);
  gsl_vector* noisevalues3 = gsl_vector_alloc(nbpts);
  EvaluateNoise(noisevalues1, freq, NoiseSn1, __LISASimFD_Noise_fLow, __LISASimFD_Noise_fHigh);
  EvaluateNoise(noisevalues2, freq, NoiseSn2, __LISASimFD_Noise_fLow, __LISASimFD_Noise_fHigh);
  EvaluateNoise(noisevalues3, freq, NoiseSn3, __LISASimFD_Noise_fLow, __LISASimFD_Noise_fHigh);

  /* Output and clean up */
  injection->TDI1Signal = TDI1;
  injection->TDI2Signal = TDI2;
  injection->TDI3Signal = TDI3;
  injection->freq = freq;
  injection->noisevalues1 = noisevalues1;
  injection->noisevalues2 = noisevalues2;
  injection->noisevalues3 = noisevalues3;

  ListmodesCAmpPhaseFrequencySeries_Destroy(listROM);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI1);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI2);
  ListmodesCAmpPhaseFrequencySeries_Destroy(listTDI3);
  return SUCCESS;
}

/* Log-Likelihood function */

double CalculateLogLCAmpPhase(LISAParams *params, LISAInjectionCAmpPhase* injection)
{
  double logL = -DBL_MAX;
  int ret;

  /* Generating the signal in the three detectors for the input parameters */
  LISASignalCAmpPhase* generatedsignal = NULL;
  LISASignalCAmpPhase_Init(&generatedsignal);
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  ret = LISAGenerateSignalCAmpPhase(params, generatedsignal);
  //tend = clock();
  //printf("time GenerateSignal: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* If LISAGenerateSignal failed (e.g. parameters out of bound), silently return -Infinity logL */
  if(ret==FAILURE) {
    logL = -DBL_MAX;
  }
  else if(ret==SUCCESS) {
    /* Computing the likelihood for each TDI channel - fstartobs is the max between the fstartobs of the injected and generated signals */
    double Mfstartobsinjected = NewtonianfoftGeom(injectedparams->m1 / injectedparams->m2, (globalparams->deltatobs * YRSID_SI) / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI));
    double Mfstartobsgenerated = NewtonianfoftGeom(params->m1 / params->m2, (globalparams->deltatobs * YRSID_SI) / ((params->m1 + params->m2) * MTSUN_SI));
    double fstartobsinjected = Mfstartobsinjected / ((injectedparams->m1 + injectedparams->m2) * MTSUN_SI);
    double fstartobsgenerated = Mfstartobsgenerated / ((params->m1 + params->m2) * MTSUN_SI);
    double fLow = fmax(__LISASimFD_Noise_fLow, globalparams->fmin);
    double fHigh = __LISASimFD_Noise_fHigh;
    RealFunctionPtr NoiseSn1 = NoiseFunction(globalparams->tagtdi, 1);
    RealFunctionPtr NoiseSn2 = NoiseFunction(globalparams->tagtdi, 2);
    RealFunctionPtr NoiseSn3 = NoiseFunction(globalparams->tagtdi, 3);
    //TESTING
    //tbeg = clock();
    double overlapTDI123 = FDListmodesFresnelOverlap3Chan(generatedsignal->TDI1Signal, generatedsignal->TDI2Signal, generatedsignal->TDI3Signal, injection->TDI1Splines, injection->TDI2Splines, injection->TDI3Splines, NoiseSn1, NoiseSn2, NoiseSn3, fLow, fHigh, fstartobsinjected, fstartobsgenerated);
    /* double loglikelihoodTDIA = FDLogLikelihood(injection->TDIASignal, generatedsignal->TDIASignal, NoiseSnA, fLow, fHigh, injection->TDIAhh, generatedsignal->TDIAhh, fstartobsinjected, fstartobsgenerated, globalparams->tagint); */
    /* double loglikelihoodTDIE = FDLogLikelihood(injection->TDIESignal, generatedsignal->TDIESignal, NoiseSnE, fLow, fHigh, injection->TDIEhh, generatedsignal->TDIEhh, fstartobsinjected, fstartobsgenerated, globalparams->tagint); */
    /* double loglikelihoodTDIT = FDLogLikelihood(injection->TDITSignal, generatedsignal->TDITSignal, NoiseSnT, fLow, fHigh, injection->TDIThh, generatedsignal->TDIThh, fstartobsinjected, fstartobsgenerated, globalparams->tagint); */
    //tend = clock();
    //printf("time Overlaps: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
    //

    //
    //printf("-----------------\n");
    //printf("(s|h) overlap TDIAET: %g\n", overlapTDIAET);
    //printf("-----------------\n");

    /* Output: value of the loglikelihood for the combined signals, assuming noise independence */

    //
  /* ListmodesCAmpPhaseSpline* listsplinesgenA = NULL; */
  /* ListmodesCAmpPhaseSpline* listsplinesgenE = NULL; */
  /* ListmodesCAmpPhaseSpline* listsplinesgenT = NULL; */
  /* BuildListmodesCAmpPhaseSpline(&listsplinesgenA, generatedsignal->TDIASignal); */
  /* BuildListmodesCAmpPhaseSpline(&listsplinesgenE, generatedsignal->TDIESignal); */
  /* BuildListmodesCAmpPhaseSpline(&listsplinesgenT, generatedsignal->TDITSignal); */
  /* double testhhoverlapTDIAET = FDListmodesFresnelOverlapAET(generatedsignal->TDIASignal, generatedsignal->TDIESignal, generatedsignal->TDITSignal, listsplinesgenA, listsplinesgenE, listsplinesgenT, NoiseSnA, NoiseSnE, NoiseSnT, fLow, fHigh, fstartobsgenerated, fstartobsinjected); */
  /* printf("hh overlap TDIAET: %g\n", testhhoverlapTDIAET); */
  /*   printf("(overlapTDIAET,1./2*(injection->TDIAETss), 1./2*(generatedsignal->TDIAEThh): (%g, %g, %g)\n", overlapTDIAET, 1./2*(injection->TDIAETss), 1./2*(generatedsignal->TDIAEThh)); */

    logL = overlapTDI123 - 1./2*(injection->TDI123ss) - 1./2*(generatedsignal->TDI123hh);
  }

  /* Clean up */
  LISASignalCAmpPhase_Cleanup(generatedsignal);

  return logL;
}

double CalculateLogLReIm(LISAParams *params, LISAInjectionReIm* injection)
{
  double logL = -DBL_MAX;
  int ret;

  /* Frequency vector - assumes common to A,E,T, i.e. identical fLow, fHigh in all channels */
  gsl_vector* freq = injection->freq;

  /* Generating the signal in the three detectors for the input parameters */
  LISASignalReIm* generatedsignal = NULL;
  LISASignalReIm_Init(&generatedsignal);
  //TESTING
  //clock_t tbeg, tend;
  //tbeg = clock();
  ret = LISAGenerateSignalReIm(params, freq, generatedsignal);
  //tend = clock();
  //printf("time GenerateSignal: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
  //

  /* If LISAGenerateSignal failed (e.g. parameters out of bound), silently return -Infinity logL */
  if(ret==FAILURE) {
    logL = -DBL_MAX;
  }
  else if(ret==SUCCESS) {
    /* Computing the likelihood for each TDI channel - fstartobs has already been taken into account */
    //TESTING
    //tbeg = clock();
    double loglikelihoodTDI1 = FDLogLikelihoodReIm(injection->TDI1Signal, generatedsignal->TDI1Signal, injection->noisevalues1);
    double loglikelihoodTDI2 = FDLogLikelihoodReIm(injection->TDI2Signal, generatedsignal->TDI2Signal, injection->noisevalues2);
    double loglikelihoodTDI3 = FDLogLikelihoodReIm(injection->TDI3Signal, generatedsignal->TDI3Signal, injection->noisevalues3);
    //tend = clock();
    //printf("time Overlaps: %g\n", (double) (tend-tbeg)/CLOCKS_PER_SEC);
    //

    /* Output: value of the loglikelihood for the combined signals, assuming noise independence */
    logL = loglikelihoodTDI1 + loglikelihoodTDI2 + loglikelihoodTDI3;
  }

  /* Clean up */
  LISASignalReIm_Cleanup(generatedsignal);

  return logL;
}

/***************************** Functions handling the prior ******************************/

/* Function to check that returned parameter values fit in prior boundaries */
int PriorBoundaryCheck(LISAPrior *prior, double *Cube)
{
	if (Cube[0] < Cube[1])
	 	return 1;

	if (Cube[0] < prior->comp_min || Cube[0] > prior->comp_max ||
	 	Cube[1] < prior->comp_min || Cube[1] > prior->comp_max)
	 	return 1;

	if (Cube[0] + Cube[1] < prior->mtot_min || Cube[0] + Cube[1] > prior->mtot_max)
		return 1;

	if (Cube[0] < Cube[1] || Cube[0] / Cube[1] > prior->qmax)
		return 1;

	return 0;
}

/* Utility prior functions to convert from Cube to common distributions, and back */

double CubeToFlatPrior(double r, double x1, double x2)
{
  return x1 + r * (x2 - x1);
}
double FlatPriorToCube(double y, double x1, double x2)
{
  return (y - x1) / (x2 - x1);
}

double CubeToLogFlatPrior(double r, double x1, double x2)
{
  return exp(log(x1) + r * (log(x2) - log(x1)));
}
double LogFlatPriorToCube(double y, double x1, double x2)
{
  return (log(y) - log(x1)) / (log(x2) - log(x1));
}

double CubeToPowerPrior(double p, double r, double x1, double x2)
{
  double pp = p + 1.0;
  return pow(r * pow(x2, pp) + (1.0 - r) * pow(x1, pp), 1.0 / pp);
}
double PowerPriorToCube(double p, double y, double x1, double x2)
{
  double pp = p + 1.0;
  return (pow(y, pp) - pow(x1, pp)) / (pow(x2, pp) - pow(x1, pp));
}

double CubeToSinPrior(double r, double x1, double x2)
{
  return acos((1.0-r)*cos(x1)+r*cos(x2));
}
double SinPriorToCube(double y, double x1, double x2) /* Note: on [0,pi] cos(x1)>cos(y)>cos(x2), not important */
{
  return (cos(x1) - cos(y))/(cos(x1) - cos(x2));
}

double CubeToCosPrior(double r, double x1, double x2)
{
  return asin((1.0-r)*sin(x1)+r*sin(x2));
}
double CosPriorToCube(double y, double x1, double x2) /* Note: on [-pi/2,pi/2] normally sin(x1)<sin(y)<sin(x2), not important */
{
  return (sin(y) - sin(x1))/(sin(x2) - sin(x1));
}

double CubeToGaussianPrior(double r, double mean, double sigma)
{
  return gsl_cdf_gaussian_Pinv(r,sigma) + mean;
}

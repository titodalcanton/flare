#include "LLVinference.h"

/******************************************** getphysparams routine ****************************************************/

void getphysparams(double *Cube, int *ndim)
{
	int i = 0;

  // component masses
  double m1 = CubeToFlatPrior(Cube[i++], priorParams->comp_min, priorParams->comp_max);
  double m2 = CubeToFlatPrior(Cube[i++], priorParams->comp_min, priorParams->comp_max);
  if (m2 > m1) {
    double tmp = m1;
    m1 = m2;
    m2 = tmp;
  }
  templateparams->m1 = m1;
  templateparams->m2 = m2;

  // time
  templateparams->tRef = CubeToFlatPrior(Cube[i++], injectedparams->tRef - priorParams->deltaT, injectedparams->tRef + priorParams->deltaT);

  // distance
  templateparams->distance = CubeToPowerPrior(2.0, Cube[i++], priorParams->dist_min, priorParams->dist_max);

  // orbital phase
  templateparams->phiRef = CubeToFlatPrior(Cube[i++], 0.0, 2.0 * M_PI);

  // inclination
  templateparams->inclination = CubeToSinPrior(Cube[i++], 0.0, M_PI);

  // sky location
  templateparams->ra = CubeToFlatPrior(Cube[i++], 0.0, 2.0 * M_PI);
  templateparams->dec = CubeToCosPrior(Cube[i++], -M_PI / 2.0, M_PI / 2.0);

  // polarization
  templateparams->polarization = CubeToFlatPrior(Cube[i++], 0.0, M_PI);

  // put all values back in Cube as well
  Cube[0] = m1;
  Cube[1] = m2;
  Cube[2] = templateparams->tRef;
  Cube[3] = templateparams->distance;
  Cube[4] = templateparams->phiRef;
  Cube[5] = templateparams->inclination;
  Cube[6] = templateparams->ra;
  Cube[7] = templateparams->dec;
  Cube[8] = templateparams->polarization;
}

/******************************************** getallparams routine ****************************************************/

void getallparams(double *Cube, int *ndim)
{
	getphysparams(Cube,ndim);
}

/******************************************** loglikelihood routine ****************************************************/

// Now an example, sample an egg box likelihood

// Input arguments
// ndim 						= dimensionality (total number of free parameters) of the problem
// npars 						= total number of free plus derived parameters
//
// Input/Output arguments
// Cube[npars] 						= on entry has the ndim parameters in unit-hypercube
//	 						on exit, the physical parameters plus copy any derived parameters you want to store with the free parameters
//
// Output arguments
// lnew 						= loglikelihood

/* Note: context must point to the LLVSignal structure representing the injected signals */
//void getLogLike(double *Cube, int *ndim, int *npars, double *lnew, void *context)
void getLogLike(LLVParams *params, double *lnew, void *context)
{
  int ret;
  /*  */
  /*getallparams(Cube,ndim);
  if (PriorBoundaryCheck(priorParams, Cube)) {
    *lnew = -DBL_MAX;
    return;
  } */

  /* Note: context points to a LLVContext structure containing a LLVSignal* */
  LLVSignal* injection = ((LLVSignal*) context);

  /* Generating the signal in the three detectors for the input parameters */
  LLVSignal* generatedsignal = NULL;
  LLVSignal_Init(&generatedsignal);
  ret = LLVGenerateSignal(params, generatedsignal);

  /* If LLVGenerateSignal failed (e.g. parameters out of bound), silently return -Infinity logL */
  if(ret==FAILURE) {
    *lnew = -DBL_MAX;
  }
  else if(ret==SUCCESS) {
    /* Computing the likelihood for each detector */
    double loglikelihoodLHO = FDLogLikelihood(injection->LHOSignal, generatedsignal->LHOSignal, NoiseSnLHO, __LLVSimFD_LHONoise_fLow, __LLVSimFD_LHONoise_fHigh, injection->LHOhh, generatedsignal->LHOhh);
    double loglikelihoodLLO = FDLogLikelihood(injection->LLOSignal, generatedsignal->LLOSignal, NoiseSnLLO, __LLVSimFD_LLONoise_fLow, __LLVSimFD_LLONoise_fHigh, injection->LLOhh, generatedsignal->LLOhh);
    double loglikelihoodVIRGO = FDLogLikelihood(injection->VIRGOSignal, generatedsignal->VIRGOSignal, NoiseSnVIRGO, __LLVSimFD_VIRGONoise_fLow, __LLVSimFD_VIRGONoise_fHigh, injection->VIRGOhh, generatedsignal->VIRGOhh);

    /* Output: value of the loglikelihood for the combined signals, assuming noise independence */
    *lnew = loglikelihoodLHO + loglikelihoodLLO + loglikelihoodVIRGO;
  }

  /* Clean up */
  LLVSignal_Cleanup(generatedsignal);
}


/************************************************* dumper routine ******************************************************/

// The dumper routine will be called every updInt*10 iterations
// MultiNest doesn not need to the user to do anything. User can use the arguments in whichever way he/she wants
//
//
// Arguments:
//
// nSamples 						= total number of samples in posterior distribution
// nlive 						= total number of live points
// nPar 						= total number of parameters (free + derived)
// physLive[1][nlive * (nPar + 1)] 			= 2D array containing the last set of live points (physical parameters plus derived parameters) along with their loglikelihood values
// posterior[1][nSamples * (nPar + 2)] 			= posterior distribution containing nSamples points. Each sample has nPar parameters (physical + derived) along with the their loglike value & posterior probability
// paramConstr[1][4*nPar]:
// paramConstr[0][0] to paramConstr[0][nPar - 1] 	= mean values of the parameters
// paramConstr[0][nPar] to paramConstr[0][2*nPar - 1] 	= standard deviation of the parameters
// paramConstr[0][nPar*2] to paramConstr[0][3*nPar - 1] = best-fit (maxlike) parameters
// paramConstr[0][nPar*4] to paramConstr[0][4*nPar - 1] = MAP (maximum-a-posteriori) parameters
// maxLogLike						= maximum loglikelihood value
// logZ							= log evidence value
// logZerr						= error on log evidence value
// context						void pointer, any additional information

void dumper(int *nSamples, int *nlive, int *nPar, double **physLive, double **posterior, double **paramConstr, double *maxLogLike, double *logZ, double *logZerr, void *context)
{
	// convert the 2D Fortran arrays to C arrays


	// the posterior distribution
	// postdist will have nPar parameters in the first nPar columns & loglike value & the posterior probability in the last two columns

	int i, j;

	double postdist[*nSamples][*nPar + 2];
	for( i = 0; i < *nPar + 2; i++ )
		for( j = 0; j < *nSamples; j++ )
			postdist[j][i] = posterior[0][i * (*nSamples) + j];



	// last set of live points
	// pLivePts will have nPar parameters in the first nPar columns & loglike value in the last column

	double pLivePts[*nlive][*nPar + 1];
	for( i = 0; i < *nPar + 1; i++ )
		for( j = 0; j < *nlive; j++ )
			pLivePts[j][i] = physLive[0][i * (*nlive) + j];
}

/***********************************************************************************************************************/




/************************************************** Main program *******************************************************/



int main(int argc, char *argv[])
{
#ifdef PARALLEL
 	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
#endif

	/*********** Addendum *************/

	/* Initialize structs for holding various options */
  LLVRunParams runParams;
  injectedparams = (LLVParams*) malloc(sizeof(LLVParams));
  memset(injectedparams, 0, sizeof(LLVParams));
  templateparams = (LLVParams*) malloc(sizeof(LLVParams));
  memset(templateparams, 0, sizeof(LLVParams));
  priorParams = (LLVPrior*) malloc(sizeof(LLVPrior));
  memset(priorParams, 0, sizeof(LLVPrior));
  
  /* Parse commandline to read parameters of injection */
  parse_args_LLV(argc, argv, injectedparams, priorParams, &runParams);

  /* Use the same fRef and nbmode for recovery templates as injected signal */
  templateparams->fRef = injectedparams->fRef;
  templateparams->nbmode = injectedparams->nbmode;

	/* Load and initialize the detector noise */
	LLVSimFD_Noise_Init_ParsePath();

	/* Initialize the data structure for the injection */
	LLVSignal* injectedsignal = NULL;
	LLVSignal_Init(&injectedsignal);

	/* Generate the injection */
	LLVGenerateSignal(injectedparams, injectedsignal);

	/* Set the context pointer */
	void *context = injectedsignal;

	/********** End of addendum ****************/

	/********** Test ****************/
	/*double l;
	  getLogLike(injectedparams, &l, context);
	  printf("LogLikelihood: %g\n", l);
	  free(injectedparams);
	  LLVSignal_Cleanup(injectedsignal);*/
	/********** End of test ****************/

	int i;

	// set the MultiNest sampling parameters

	int mmodal = 0;					// do mode separation?

	int ceff = 0;					// run in constant efficiency mode?

	int nlive = runParams.nlive;				// number of live points

	double efr = runParams.eff;				// set the required efficiency

	double tol = runParams.tol;				// tol, defines the stopping criteria

	int ndims = 9;					// dimensionality (no. of free parameters)

	int nPar = 9;					// total no. of parameters including free & derived parameters

	int nClsPar = 2;				// no. of parameters to do mode separation on

	int updInt = 50;				// after how many iterations feedback is required & the output files should be updated
							// note: posterior files are updated & dumper routine is called after every updInt*10 iterations

	double Ztol = -1E90;				// all the modes with logZ < Ztol are ignored

	int maxModes = 1;				// expected max no. of modes (used only for memory allocation)

	int pWrap[ndims];				// which parameters to have periodic boundary conditions?
	for(i = 0; i < ndims; i++) pWrap[i] = 0;
  pWrap[4] = pWrap[6] = pWrap[8] = 1;

	strcpy(root, runParams.outroot);			// root for output files
	strcpy(networkinputs, runParams.netfile);			// file with input parameters for network training

	int seed = -1;					// random no. generator seed, if < 0 then take the seed from system clock

	int fb = 1;					// need feedback on standard output?

	resume = runParams.resume;					// resume from a previous job?

	int outfile = 1;				// write output files?

	int initMPI = 0;				// initialize MPI routines?, relevant only if compiling with MPI
							// set it to F if you want your main program to handle MPI initialization

	logZero = -1E90;				// points with loglike < logZero will be ignored by MultiNest

	int maxiter = 0;				// max no. of iterations, a non-positive value means infinity. MultiNest will terminate if either it
							// has done max no. of iterations or convergence criterion (defined through tol) has been satisfied

	// void *context = 0;				// not required by MultiNest, any additional information user wants to pass

	doBAMBI = runParams.bambi;					// BAMBI?

	useNN = 0;

	// calling MultiNest

	BAMBIrun(mmodal, ceff, nlive, tol, efr, ndims, nPar, nClsPar, maxModes, updInt, Ztol, root, seed, pWrap, fb, resume, outfile, initMPI,
	logZero, maxiter, LogLikeFctn, dumper, BAMBIfctn, context);

  free(injectedparams);
  free(templateparams);
  free(priorParams);

#ifdef PARALLEL
 	MPI_Finalize();
#endif
}

/***********************************************************************************************************************/
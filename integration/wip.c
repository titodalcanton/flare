//Written by John G Baker at gsfc.nasa.gov
//Weighted Inner-Product
#include "spline.h"
#include "Faddeeva.h"
#include <complex.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

//# This function realizes integration of S-weighted inner product
//# of the difference between two complex functions represented by
//# slowly evolving phase and amplitude.
//# 
//# We write the integral as:
//#
//# I = \int{df A(f)exp(i\psi(f))} = \int{df |s_1(f)* s_2(f)|^2/S(f)}
//#
//# For comparison we have a slow classic approach applying standard quadrature
//# encoded in the function wip_sum.  We also try a novel approach encoded
//# in wip_phase.



//# Our novel approach involves locally approximating 
//# A(f) and \psi(f) as quadratic polynomials
//# We can then evaluate the integral explicitly over short local intervals

int wip_relative=1;
double wip_mindf=1e-8;
double wip_errtol=1.0e-8;
int wip_uselogf=0;
int wip_count;

// To avoid loss of precision at small z, cexpm1i(zi) implements cos(zi)-1 + i*sin(zi)
// The real analogue of this function is part of the standard library, and the C99 standard
// reserves the name, but it is not generally implemented
// inline double complex cexpm1i(double zi){
double complex cexpm1i(double zi){
  double rr;//rr=cos(x)-1=2*sin(x/2)**2
  double ri=sin(zi);
  double sinhalf=sin(zi/2.0);
  rr=-2.0*sinhalf*sinhalf;
  return rr+I*ri;
};

///Interpolation of integrand on a new f-grid
///If min_f and max_f are nonpositive then the integrand is defined over the intersection of the f1 and f2 domains.
///If either of these is greater than zero, then that value is used for the repsective bound.
void interpolate_ip_integrand(double *f1, double *s1Ar, double *s1Ai, double  *s1p, int n1, double *f2, double *s2Ar, double*s2Ai, double *s2p, int n2,  double (*Snoise)(double), double scalefactor, double *fnew, double *Ars,  double *Ais, double *dphis, int *nnew, double min_f, double max_f){
  //#returns fnew, integrand
  //#f1,f2 should be arrays of ordered positive values
  //#s1,s2,Sn should be a function

  //Construct splines from the input data:
  double s1Arz[n1],s1Aiz[n1],s1pz[n1];
  double s2Arz[n2],s2Aiz[n2],s2pz[n2];
  spline_construct(f1,s1Ar,s1Arz,n1);
  spline_construct(f1,s1Ai,s1Aiz,n1);
  spline_construct(f1,s1p, s1pz, n1);
  spline_construct(f2,s2Ar,s2Arz,n2);
  spline_construct(f2,s2Ai,s2Aiz,n2);
  spline_construct(f2,s2p, s2pz, n2);

  //#Construct new grid based on spacing of the old grid spacings with continuity in df
  //#(could make this smoother by going higher order, but why).
  //#We define the new scaling to have appropriate limits, when either grids spacing is much smaller,
  //#or when they are equal.
  //#With scale=1, defining df12=df1+df2, mu=df1*df2/df12, want:
  //#  dfnew ~ min(df1,fd2) mu        when mu is small
  //#  dfnew ~ df12/2                 when mu is maximal, mu ~ df12/4
  //#  so
  //#  dfnew = mu*(1+4*mu/df12)

  double f_max=fmin(f1[n1-1],f2[n2-1]);
  if(max_f>0){
    if(f_max>=max_f)f_max=max_f;
    else{
      printf("wip:interpolate_ip_integrand: Specified upper range bound max_f is beyond data range.\n");
    }
  }      
  double f_min=fmax(f1[0],f2[0]);
  if(min_f>0){
    if(f_min<=min_f)f_min=min_f;
    else{
      printf("wip:interpolate_ip_integrand: Specified lower range bound min_f is beyond data range.\n");
    }
  }      
  double f=f_min;
  int NnewMax=*nnew;
  int i1=0,i2=0,inew=0;
  while(1){//skip the first increment
    double df=0;
    if(inew>0){
      while(f1[i1]<f && i1<n1-2) i1+=1;//#must keep i<nf-1
      while(f2[i2]<f && i2<n2-2) i2+=1;
      double df1= f1[i1+1]-f1[i1];  
      double df2= f2[i2+1]-f2[i2];
      double df12=df1+df2;
      double mu=df1*df2/df12;
      //printf(" %i: df1=%7.4f ->df2=%7.4f\n",inew,df1,df2);
      df=mu*(1+4.0*mu/df12)/scalefactor;  
    }
    f+=df;
    if(f_max-f<wip_mindf)f=f_max;
    if(inew>=NnewMax){
      //printf(" inew==%i>=%i\n",inew,NnewMax);
      printf("wip:Error, out of space in array.\n");
      exit(1);
    }
    fnew[inew]=f;
    //printf(" %i: df=%7.4f ->f=%7.4f\n",inew,df,f);
    //#now we compute the integrand on the new grid
    double A1r=spline_int(f,f1,s1Ar,s1Arz,n1);
    double A1i=spline_int(f,f1,s1Ai,s1Aiz,n1);
    double p1=spline_int(f,f1,s1p,s1pz,n1);
    double A2r=spline_int(f,f2,s2Ar,s2Arz,n2);
    double A2i=spline_int(f,f2,s2Ai,s2Aiz,n2);
    double p2=spline_int(f,f2,s2p,s2pz,n2);
    double Sn=Snoise(f);
    //Ars[inew]=(A1r*A2r-A1i*A2i)/Sn;
    //Ais[inew]=(A1r*A2i+A1i*A2r)/Sn;
    Ars[inew]=(A1r*A2r+A1i*A2i)/Sn;//previously hadn't taken CC of S2
    Ais[inew]=(-A1r*A2i+A1i*A2r)/Sn;
    if(wip_uselogf){
      double ef=exp(f);
      Ars[inew]*=ef;
      Ais[inew]*=ef;
    }
    dphis[inew]=-p2+p1;
    if(f>=f_max)break;
    inew++;
  }
  fnew[inew]=f_max;
  *nnew=inew+1;
};
        
    
//#Now perform the integration...
double complex wip_phase (double *f1, int n1, double *f2, int n2, double *s1Ar, double *s1Ai, double  *s1p, double *s2Ar, double*s2Ai, double *s2p, double (*Snoise)(double), double scalefactor, double min_f, double max_f){
  int i;
  double f1x[n1],f2x[n2];
  if(wip_uselogf){
    for(i=0; i<n1;i++){
      double f=f1[i];
      if(f<=0)printf("wip_phase:Trouble using log grid: f1=%g\n",f);
      f1x[i]=log(f);
    }
    for(i=0; i<n2;i++){
      double f=f2[i];
      if(f<=0)printf("wip_phase:Trouble using log grid: f2=%g\n",f);
      f2x[i]=log(f);
    }
  } else {
    for(i=0; i<n1;i++)f1x[i]=f1[i];
    for(i=0; i<n2;i++)f2x[i]=f2[i];
  }
  
  //#integrate to new grid
  int NfMax=(n1+n2)*scalefactor;//should be adequately large. 
  double fs[NfMax],Ars[NfMax],Ais[NfMax],Arz[NfMax],Aiz[NfMax],dphis[NfMax],dphiz[NfMax];//(does this work in C?)
  int nf=NfMax;
  interpolate_ip_integrand(f1x, s1Ar, s1Ai, s1p, n1, f2x, s2Ar, s2Ai, s2p, n2, Snoise, scalefactor, fs, Ars, Ais, dphis, &nf, min_f, max_f);
  wip_count=nf;//just for testing/reference
  spline_construct(fs,Ars,Arz,nf);
  spline_construct(fs,Ais,Aiz,nf);
  spline_construct(fs,dphis,dphiz,nf);

  //printf( "fs: [");for(i=0;i<nf-1;i++)printf(" %g,",fs[i]);printf(" %g ]\n",fs[nf-1]);
  //printf( "Ars: [");for(i=0;i<nf-1;i++)printf(" %g,",Ars[i]);printf(" %g ]\n",Ars[nf-1]);
  //printf( "Ais: [");for(i=0;i<nf-1;i++)printf(" %g,",Ais[i]);printf(" %g ]\n",Ais[nf-1]);
  //printf( "dphis: [");for(i=0;i<nf-1;i++)printf(" %g,",dphis[i]);printf(" %g ]\n",dphis[nf-1]);


  double complex intsum=0;
  //intd3vec(f,fs,Ars,Azs,nf)
  const double complex sqrti = cexp(0.25*I*M_PI);
  const double sqrtpi = sqrt(M_PI);
  
  for(i=0;i<nf-1;i++){//loop over freq intervals
    //#quadratic phase integration over the current interval (fs[0],fs[2]):
    //#might also try a linear-phase version for comparison...
    double f=fs[i];
    double eps=fs[i+1]-f;
    double eps2=eps*eps;

    //#amp coeffs
    double a0r,a0i,a1r,a1i,a2r,a2i,a3r,a3i;
    spline_intd3(f,fs,Ars,Arz,nf,&a0r,&a1r,&a2r,&a3r);
    spline_intd3(f,fs,Ais,Aiz,nf,&a0i,&a1i,&a2i,&a3i);
    double complex a0=a0r+I*a0i;
    double complex a1=a1r+I*a1i;
    double complex a2=a2r+I*a2i;
    double complex a3=a3r+I*a3i;
      
    a2=a2/2.0;
    a3=a3/6.0;
    //printf("i=%i: As = %g+%gi ,  %g+%gi ,  %g+%gi , %g+%gi \n",i,a0r,a0i,a1r,a1i,a2r/2,a2i/2,a3r/6,a3i/6);
    double complex ampscale=1;
    if(wip_relative){
      ampscale=a0;
      a0=1;
      a1=a1/ampscale;
      a2=a2/ampscale;
      a3=a3/ampscale;
    }
    //#phase coefficients
    double p0,p1,p2,p3;
    spline_intd3(f,fs,dphis,dphiz,nf,&p0,&p1,&p2,&p3);
    p2=p2/2.0;
    p3=p3/6.0;

    //#Inm   = Integrate(x^n Em(x), {x,0,eps}) / E0
    //#Em(x) = Exp(p1*x+...pm*x^m)
    double p1e=p1*eps;
    double p2e2=p2*eps2;
    double p3e3=p3*eps*eps2;
    double complex E0=cexp(I*p0);
    double complex E2m1=cexpm1i(p1e+p2e2);
    double complex E2=E2m1+1.0;
    //#const phase terms
    double I00, I10, I20, I30;
    I00=eps;
    I10=eps2/2.0;
    I20=eps2*eps/3.0;
    I30=eps2*eps2/4.0;
    //#linear phase terms (if needed), and quadratic phase terms
    double complex I01, I11, I21, I31;
    double complex I02, I12, I22, I32;
    int useapproxquad=p2e2*p2e2*p2e2*p2<wip_errtol;
    if(useapproxquad){
      double p1e2=p1e*p1e;
      if(p1e2*p1e2<wip_errtol){// #small p1e approx with errs order p1e^4
	double complex ip1=I*p1;
	I01=I00+ip1*(I10+ip1/2.0*(I20+ip1/3.0*I30));
	I11=I10+ip1*(I20+ip1/2.0*I30);
	I21=I20+ip1*I30;
	I31=I30;
      } else {
	double complex iop1=I/p1;
	//A space for the cexpm1 function is reserved in the C99 standard, though it need not be implemented.
	//If it exists it can avoid precision issues with small values of the argument.
	double complex E1m1=cexpm1i(p1e);
	double complex E1=1.0+E1m1;
	I01=-iop1*E1m1;
	I11=iop1*(I01-I00*E1);
	I21=2.0*iop1*(I11-I10*E1);
	I31=3.0*iop1*(I21-I20*E1);
      }
      //#small p2e approx with errs order p2e^2
      double complex ip2=I*p2;
      I02=I01+ip2*I21;
      I12=I11+ip2*I31;
      I22=I21;
      I32=I31;
    } else {
      double complex io2p2=0.5*I/p2;
      double complex s=csqrt(p2);
      double complex z0=p1/s/2.0;
      double complex w0=Faddeeva_w(sqrti*z0,wip_errtol);
      double complex wp=Faddeeva_w(sqrti*( eps*s+z0),wip_errtol);
      double complex ip1=I*p1;
      I02=-0.5/s*sqrti*sqrtpi*( E2*wp - w0 );
      I12= -io2p2*(E2m1-ip1*I02);
      I22=-io2p2*(I00*E2-I02-ip1*I12);
      I32=-io2p2*(2.0*(I10*E2-I12)-ip1*I22);
    }
    //printf("i=%i: Ix2s = %g+%gi ,  %g+%gi ,  %g+%gi , %g+%gi \n",i,creal(I02),cimag(I02),creal(I12),cimag(I12),creal(I22),cimag(I22),creal(I32),cimag(I32));
    //#cubic phase terms
    //#small p3e3 approx is our only option (we can keep more terms... above)
    double complex I03, I13, I23, I33;
    double complex ip3=I*p3;
    I03=I02+ip3*I32;
    I13=I12;
    I23=I22;
    I33=I32;
    //printf("i=%i: In3s = %g+%gi ,  %g+%gi ,  %g+%gi , %g+%gi \n",i,creal(I03),cimag(I03),creal(I13),cimag(I13),creal(I23),cimag(I23),creal(I33),cimag(I33));
    //#finish
    //printf("i=%i cofac=%g+%gi\n",i,creal(ampscale*E0),cimag(ampscale*E0));
    //printf("i=%i: ampscale=%g+%gi, E0=%g+%gi\n",i,creal(ampscale),cimag(ampscale),creal(E0),cimag(E0));
    double complex dint=ampscale*E0*(a0*I03 + a1*I13 + a2*I23 + a3*I33);
    // #step
    intsum += dint;
    //printf("i=%i: intsum,dint= %g+%gi, %g+%gi\n",i,creal(intsum),cimag(intsum),creal(dint),cimag(dint)); 
  }//#end of loop over freq intervals
  
  return intsum;
};

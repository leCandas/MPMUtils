/// \file UnpolarizedBeta.cc
#include "UnpolarizedBeta.hh"
#include <vector>
#include <map>
#include <cassert>
#include <gsl/gsl_sf_dilog.h>

#ifdef USE_ROOT_MATH
#include <TMath.h>
#define Gamma(x) TMath::Gamma(x)
#else
#define Gamma(x) tgamma(x)
#endif

using std::map;
using std::vector;

double plainPhaseSpaceCDF(double W, double W0) {
    if(W <= 1) return 0;
    
    double x = sqrt(W*W-1);
    double W2 = W*W;
    double W3 = W*W2;
    double W4 = W2*W2;
    return W0*log(x+W)/4 + x/60 * (12*W4 - 30*W3*W0 + 4*W2*(5*W0*W0-1) + 15*W*W0 -20*W0*W0 - 8);
}

/// struct for 1-index coefficients
struct coeff1 {
    coeff1(int ii, double cc): i(ii), c(cc) {}
    int i;
    double c;
};

/// sum coeff3 power series
double sumCoeffs(const vector<coeff1>& coeffs, double x=1.0) {
    double s = 0;
    for(vector<coeff1>::const_iterator it = coeffs.begin(); it != coeffs.end(); it++)
        s += it->c*pow(x,it->i);
    return s;
}

/// struct for 3-index coefficients
struct coeff2 {
    coeff2(int ii, int jj, double cc): i(ii), j(jj), c(cc) {}
    int i;
    int j;
    double c; 
};

/// sum coeff3 power series
double sumCoeffs(const vector<coeff2>& coeffs, double x=1.0, double y=1.0) {
    double s = 0;
    for(vector<coeff2>::const_iterator it = coeffs.begin(); it != coeffs.end(); it++)
        s += it->c*pow(x,it->i)*pow(y,it->j);
    return s;
}

/// struct for 3-index coefficients
struct coeff3 {
    coeff3(int ii, int jj, int kk, double cc): i(ii), j(jj), k(kk), c(cc) {}
    int i;
    int j;
    int k;
    double c; 
};

/// sum coeff3 power series
double sumCoeffs(const vector<coeff3>& coeffs, double x=1.0, double y=1.0, double z=1.0) {
    double s = 0;
    for(vector<coeff3>::const_iterator it = coeffs.begin(); it != coeffs.end(); it++)
        s += it->c*pow(x,it->i)*pow(y,it->j)*pow(z,it->k);
    return s;
}

/// definition of \gamma in [1]
double WilkinsonGamma(double Z = 1.) { return sqrt(1-(alpha*Z)*(alpha*Z)); }

double WilkinsonF_PowerSeries(double Z, double W, double R) {
    // set up coeffs
    static vector<coeff3> coeffs;
    if(!coeffs.size()) {
        coeffs.push_back(coeff3(0,0,0,1.));
        
        coeffs.push_back(coeff3(1,1,0,M_PI));
        
        coeffs.push_back(coeff3(2,0,0,0.577216));
        coeffs.push_back(coeff3(2,0,1,-1.));
        coeffs.push_back(coeff3(2,2,0,3.289868));
        
        coeffs.push_back(coeff3(3,1,0,1.813376));
        coeffs.push_back(coeff3(3,1,1,-M_PI));
    }
    
    double p = sqrt(W*W-1);
    double gm = WilkinsonGamma(Z);
    double z = Gamma(2.*gm+1.);
    return 2.*(gm+1.)/(z*z)*pow(2*R,2*(gm-1))*sumCoeffs(coeffs,alpha*Z,W/p,log(p));
}

/// approximation to |Gamma(gm+iaZW/p)|^2, as per [3] eq. 1
double WilkinsonGammaMagSquaredApprox(double Z, double W, unsigned int N) {
    double s = 0;
    double gm = WilkinsonGamma(Z);
    double y = alpha*Z*W/sqrt(W*W-1);
    double Ngm = N+gm;
    double a = (N+1.)/Ngm;
    double y1 = a*y;
    for(unsigned int n=0; n<N; n++)
        s += log((n*n+y1*y1)/((n+gm)*(n+gm)+y*y));
    return exp(s + log(M_PI*(N*N+y1*y1)/(y1*sinh(M_PI*y1)))
    +(1.-gm)*(2.-log(Ngm*Ngm+y*y) + 2.*y/Ngm*atan(y/Ngm)
    +1./(Ngm*Ngm+y*y)/(6.*a))
    -(2.*N+1.)*log(a));
}

double F_approx(double W, double Z, double R, bool fullterms) {
    if(W<=1) return 1;
    double F = 1;
    double p = sqrt(W*W-1.);
    double azn = alpha*Z;
    double x = 11./4.-gamma_euler-log(2*p*R);
    F += azn*(M_PI*W/p);
    if(!fullterms) return F;
    azn *= alpha*Z;
    F += azn*(x+M_PI*M_PI*W*W/(3.*p*p));
    azn *= alpha*Z;
    F += azn*(M_PI*W/p*x);
    return F<20?F:20;
}

// F0 = 2/(1+WilkinsonGamma) * F
double WilkinsonF0(double Z, double W, double R, unsigned int N) {
    if(W<=1) return 0;
    double gm = WilkinsonGamma(Z);
    double GMi = 1./Gamma(2*gm+1);
    double p = sqrt(W*W-1.);
    double F0 = 4.*pow(2.*p*R,2.*gm-2.)*GMi*GMi*exp(M_PI*Z*alpha*W/p)*WilkinsonGammaMagSquaredApprox(Z,W,N);
    return F0<1e3?F0:0;
}

double WilkinsonRV(double W, double W0, double M) {
    return (1.+W0*W0/(2.*M*M)-11./(6.*M*M)
    +W0/(3*M*M)/W
    +(2./M-4.*W0/(3.*M*M))*W
    +16./(3.*M*M)*W*W);
}

double WilkinsonRA(double W, double W0, double M) {
    return (1.+2.*W0/(3.*M)-W0*W0/(6.*M*M)-77./(18.*M*M)
    +(-2./(3.*M)+7*W0/(9.*M*M))/W
    +(10./(3.*M)-28.*W0/(9.*M*M))*W
    +88./(9.*M*M)*W*W);
}

double CombinedR(double W, double M2_F, double M2_GT, double W0, double M) {
    return (M2_F*WilkinsonRV(W,W0,M)+lambda*lambda*M2_GT*WilkinsonRA(W,W0,M))/(M2_F+lambda*lambda*M2_GT);
}

double Bilenkii59_RWM(double W) {
    const double mu = 2.792847356-(-1.91304273);
    return (-2.*lambda*(lambda+mu)*beta_W0
    +2.*(5.*lambda*lambda+2.*lambda*mu+1.)*W
    -2.*lambda*(mu+lambda)/W)/(1.+3*lambda*lambda)/proton_M0;
}

double WilkinsonL0(double Z, double W, double R) {
    // set up coeffs
    static vector<coeff1> ai[6];
    static vector<coeff1> aminus1;
    static map<double,vector<coeff1> > aiZ;
    static map<double,double> aminus1Z;
    
    if(!ai[0].size()) {
        aminus1.push_back(coeff1(1,0.115));
        aminus1.push_back(coeff1(2,-1.8123));
        aminus1.push_back(coeff1(3,8.2498));
        aminus1.push_back(coeff1(4,-11.223));
        aminus1.push_back(coeff1(5,-14.854));
        aminus1.push_back(coeff1(6,32.086));
        
        ai[0].push_back(coeff1(1,-0.00062));
        ai[1].push_back(coeff1(1,0.02482));
        ai[2].push_back(coeff1(1,-0.14038));
        ai[3].push_back(coeff1(1,0.008152));
        ai[4].push_back(coeff1(1,1.2145));
        ai[5].push_back(coeff1(1,-1.5632));
        
        ai[0].push_back(coeff1(2,0.007165));
        ai[1].push_back(coeff1(2,-0.5975));
        ai[2].push_back(coeff1(2,3.64953));
        ai[3].push_back(coeff1(2,-1.15664));
        ai[4].push_back(coeff1(2,-23.9931));
        ai[5].push_back(coeff1(2,33.4192));
        
        ai[0].push_back(coeff1(3,0.01841));
        ai[1].push_back(coeff1(3,4.84199));
        ai[2].push_back(coeff1(3,-38.8143));
        ai[3].push_back(coeff1(3,49.9663));
        ai[4].push_back(coeff1(3,149.9718));
        ai[5].push_back(coeff1(3,-255.1333));
        
        ai[0].push_back(coeff1(4,-0.53736));
        ai[1].push_back(coeff1(4,-15.3374));
        ai[2].push_back(coeff1(4,172.1368));
        ai[3].push_back(coeff1(4,-273.711));
        ai[4].push_back(coeff1(4,-471.2985));
        ai[5].push_back(coeff1(4,938.5297));
        
        ai[0].push_back(coeff1(5,1.2691));
        ai[1].push_back(coeff1(5,23.9774));
        ai[2].push_back(coeff1(5,-346.708));
        ai[3].push_back(coeff1(5,657.6292));
        ai[4].push_back(coeff1(5,662.1909));
        ai[5].push_back(coeff1(5,-1641.2845));
        
        ai[0].push_back(coeff1(6,-1.5467));
        ai[1].push_back(coeff1(6,-12.6534));
        ai[2].push_back(coeff1(6,288.7873));
        ai[3].push_back(coeff1(6,-603.7033));
        ai[4].push_back(coeff1(6,-305.6804));
        ai[5].push_back(coeff1(6,1095.358));
    }
    
    if(!aiZ.count(Z)) {
        vector<coeff1> aiZi;
        for(unsigned int i=0; i<6; i++)
            aiZi.push_back(coeff1(i, sumCoeffs(ai[i], alpha*Z)));
        aiZ.emplace(Z, aiZi);
        aminus1Z.emplace(Z, sumCoeffs(aminus1,alpha*Z));
    }
    
    if(W<=1)
        return 0;
    
    double gm = WilkinsonGamma(Z);
    double L0 = (1.+13.*(alpha*Z)*(alpha*Z)/60.-W*R*alpha*Z*(41.-26.*gm)/(15.*(2.*gm-1.))
    -alpha*Z*R*gm*(17.-2.*gm)/(30.*W*(2.*gm-1.))
    +aminus1Z[Z]*R/W+sumCoeffs(aiZ[Z],W*R)
    +0.41*(R-0.0164)*pow(alpha*Z,4.5) );
    
    return L0==L0?L0*2./(1.+gm):0;
}

double WilkinsonVC(double Z, double W, double W0, double R) {
    double gm = WilkinsonGamma(Z);
    return (1.-233.*alpha*Z*alpha*Z/630.-W0*R*W0*R/5.-6*W0*R*alpha*Z/35.
    +(-13.*R*alpha*Z/35.+4.*W0*R*R/15.)*W
    +(2.*gm*W0*R*R/15.+gm*R*alpha*Z/70.)/W
    -4*R*R/15.*W*W);
}

double WilkinsonAC(double Z, double W, double W0, double R) {
    return (1.-233.*alpha*Z*alpha*Z/630.-W0*R*W0*R/5.+2*W0*R*alpha*Z/35.
    +(-21.*R*alpha*Z/35.+4.*W0*R*R/9.)*W
    -4.*R*R/9.*W*W);
}

double CombinedC(double Z, double W, double M2_F, double M2_GT, double W0, double R) {
    return (M2_F*WilkinsonVC(Z,W,W0,R)+lambda*lambda*M2_GT*WilkinsonAC(Z,W,W0,R))/(M2_F+lambda*lambda*M2_GT);
}

double WilkinsonQ(double, double W, double W0, double M) {
    double B = (1.-lambda)/(1.+3.*lambda*lambda);
    return 1.-M_PI*alpha/(M*sqrt(W*W-1))*(1+B*(W0-W)/(3.*W));
}

double SpenceL(double x) {
    return gsl_sf_dilog(x);
    
    /* sum approximation
    assert(-1.<x && x<=1.);
    double s = 0;
    double xk = x;
    for(unsigned int k=1; k<=N; k++) {
        s += xk/(k*k);
        xk *= x;
    }
    return -s;
    */
}

//-----------------------------------------------------//

BetaSpectrumGenerator::BetaSpectrumGenerator(double a, double z, double ep): A(a), Z(z), EP(ep),
W0((EP+m_e)/m_e), R(pow(A,1./3.)*neutron_R0), M0(fabs(Z)*proton_M0+(A-fabs(Z))*neutron_M0),
forbidden(0), M2_F(0), M2_GT(1) { }

double BetaSpectrumGenerator::spectrumCorrectionFactor(double W) const {
    double c = WilkinsonF0(Z,W,R);              // Fermi function Coulomb
    c *= WilkinsonL0(Z,W,R);                    // Nonzero charge radius effect on Coulomb correction
    c *= CombinedC(Z,W,M2_F,M2_GT,W0,R);        // electron/nucleon nonzero size wavefunction convolution
    c *= WilkinsonQ(Z,W,W0,M0);                 // Recoil effect on Coulomb correction (tiny tiny!)
    c *= (1.+Wilkinson_g_a2pi(W,W0,M0));        // outer radiative corrections
    if(A==1 && Z==1) {
        c *= (1.+Bilenkii59_RWM(W));            // recoil + WM for free neutron
    } else {
        c *= CombinedR(W,M2_F,M2_GT,W0,M0);     // recoil effect on phase space
    }
    // first forbidden Axial-Vector decays
    if(forbidden==1 && M2_GT>0 && M2_F==0)
        c *= Davidson_C1T(W, W0, Z, R);
    // Cs137 second-forbidden decay
    if(forbidden==2 && A==137)
        c *= Behrens_Cs137_C(W, W0);
    
    return c;
}

double BetaSpectrumGenerator::decayProb(double KE) const {
    double W = (KE+m_e)/m_e;
    return plainPhaseSpace(W,W0)*spectrumCorrectionFactor(W);
}


/// \file NuclEvtGen.cc
#include "NuclEvtGen.hh"
#include "SMExcept.hh"
#include "StringManip.hh"
#include "PathUtils.hh"
#include <math.h>
#include <cfloat>
#include <stdlib.h>
#include <algorithm>
#include <TRandom.h>

unsigned int PSelector::select(double* x) const {
    static double rnd_tmp;
    if(!x) { x=&rnd_tmp; rnd_tmp=gRandom->Uniform(0,cumprob.back()); }
    else { smassert(0. <= *x && *x <= 1.); (*x) *= cumprob.back(); }
    vector<double>::const_iterator itsel = std::upper_bound(cumprob.begin(),cumprob.end(),*x);
    unsigned int selected = (unsigned int)(itsel-cumprob.begin()-1);
    smassert(selected<cumprob.size()-1);
    (*x) = ((*x) - cumprob[selected])/(cumprob[selected+1]-cumprob[selected]);
    return selected;
}

void PSelector::scale(double s) {
    for(auto& p: cumprob) p *= s;
}

double PSelector::getProb(unsigned int n) const {
    smassert(n<cumprob.size()-1);
    return (cumprob[n+1]-cumprob[n])/cumprob.back();
}

//-----------------------------------------

string particleName(DecayType t) {
    if(t==D_GAMMA) return "gamma";
    if(t==D_ELECTRON) return "e-";
    if(t==D_POSITRON) return "e+";
    if(t==D_NEUTRINO) return "neutrino";
    return "UNKNOWN";
}

DecayType particleType(const std::string& s) {
    if(s=="gamma") return D_GAMMA;
    if(s=="e-") return D_ELECTRON;
    if(s=="e+") return D_POSITRON;
    if(s=="neutrino") return D_NEUTRINO;
    return D_NONEVENT;
}

void randomDirection(double& x, double& y, double& z, double* rnd) {
    double phi = 2.0*M_PI*(rnd?rnd[1]:gRandom->Uniform(0,1));
    double costheta = 2.0*(rnd?rnd[0]:gRandom->Uniform(0,1))-1.0;
    double sintheta = sqrt(1.0-costheta*costheta);
    x = cos(phi)*sintheta;
    y = sin(phi)*sintheta;
    z = costheta;
}

//-----------------------------------------

NucLevel::NucLevel(const Stringmap& m): fluxIn(0), fluxOut(0) {
    name = m.getDefault("nm","0.0.0");
    vector<string> v = split(name,".");
    smassert(v.size()==3);
    A = atoi(v[0].c_str());
    Z = atoi(v[1].c_str());
    n = atoi(v[2].c_str());
    E = m.getDefault("E",0);
    hl = m.getDefault("hl",0);
    if(hl < 0) hl = DBL_MAX;
    jpi = m.getDefault("jpi","");
}

void NucLevel::display(bool) const {
    printf("[%i] A=%i Z=%i jpi=%s\t E = %.2f keV\t HL = %.3g s\t Flux in = %.3g, out = %.3g\n",
           n,A,Z,jpi.c_str(),E,hl,fluxIn,fluxOut);
}

//-----------------------------------------

DecayAtom::DecayAtom(BindingEnergyTable const* B): BET(B), Iauger(0), Ikxr(0), ICEK(0), IMissing(0), pAuger(0) {
    if(BET->getZ()>2)
        Eauger = BET->getSubshellBinding(0,0) - BET->getSubshellBinding(1,0) - BET->getSubshellBinding(1,1);
    else
        Eauger = 0;
}

void DecayAtom::load(const Stringmap& m) {
    for(std::multimap< std::string, string >::const_iterator it = m.dat.begin(); it != m.dat.end(); it++) {
        smassert(it->first.size());
        if(it->first[0]=='a') Iauger += atof(it->second.c_str())/100.0;
        else if(it->first[0]=='k') Ikxr += atof(it->second.c_str())/100.0;
    }
    Iauger = m.getDefault("Iauger",0)/100.0;
    
    pAuger = Iauger/(Iauger+Ikxr);
    IMissing = Iauger+Ikxr-ICEK;
    if(!Iauger) IMissing = pAuger = 0;
}

void DecayAtom::genAuger(vector<NucDecayEvent>& v) {
    if(gRandom->Uniform(0,1) > pAuger) return;
    NucDecayEvent evt;
    evt.d = D_ELECTRON;
    evt.E = Eauger;
    evt.randp();
    v.push_back(evt);
}

void DecayAtom::display(bool) const {
    printf("%s %i: pAuger = %.3f, Eauger = %.2f, initCapt = %.3f\n",
           BET->getName().c_str(),BET->getZ(),pAuger,Eauger,IMissing);
}

//-----------------------------------------

void TransitionBase::display(bool) const {
    printf("[%i]->[%i] %.3g (%i DF)\n",from.n,to.n,Itotal,getNDF());
}

//-----------------------------------------

ConversionGamma::ConversionGamma(NucLevel& f, NucLevel& t, const Stringmap& m): TransitionBase(f,t) {
    Egamma = from.E - to.E;
    Igamma = m.getDefault("Igamma",0.0)/100.0;
    // load conversion electron and subshell probabilities
    unsigned int nshells = BindingEnergyTable::shellnames.size();
    for(unsigned int i=0; i<nshells; i++) {
        vector<string> v = split(m.getDefault("CE_"+c_to_str(BindingEnergyTable::shellnames[i]),""),"@");
        if(!v.size()) break;
        float_err shprob(v[0]);
        shells.addProb(shprob.x);
        shellUncert.push_back(shprob.err*Igamma);
        vector<double> ss;
        if(v.size()==1) ss.push_back(1.);
        else ss = sToDoubles(v[1],":");
        subshells.push_back(PSelector());
        for(unsigned int s=0; s<ss.size(); s++)
            subshells.back().addProb(ss[s]);
    }
    // assign remaining probability for gamma
    shells.addProb(1.);
    shells.scale(Igamma);
    Itotal = shells.getCumProb();
}

void ConversionGamma::run(vector<NucDecayEvent>& v, double* rnd) {
    shell = (int)shells.select(rnd);
    if(shell < (int)subshells.size())
        subshell = (int)subshells[shell].select(rnd);
    else
        shell = subshell = -1;
    NucDecayEvent evt;
    evt.E = Egamma;
    if(shell<0) {
        evt.d = D_GAMMA;
    } else {
        evt.d = D_ELECTRON;
        evt.E -= toAtom->BET->getSubshellBinding(shell,subshell);
    }
    evt.randp(rnd);
    v.push_back(evt);
}

void ConversionGamma::display(bool verbose) const {
    double ceff = 100.*getConversionEffic();
    printf("Gamma %.1f (%.3g%%)",Egamma,(100.-ceff)*Itotal);
    if(subshells.size()) {
        float_err eavg = averageE();
        printf(", CE %.2f~%.2f (%.3g%%)",eavg.x,eavg.err,ceff*Itotal);
    }
    printf("\t");
    TransitionBase::display(verbose);
    if(verbose) {
        for(unsigned int n=0; n<subshells.size(); n++) {
            printf("\t[%c] %.2fkeV\t%.3g%%\t%.3g%%\t",
                   BindingEnergyTable::shellnames[n],shellAverageE(n),
                   100.*shells.getProb(n),100.0*shells.getProb(n)*Itotal);
            if(subshells[n].getN()>1) {
                for(unsigned int i=0; i<subshells[n].getN(); i++) {
                    if(i) printf(":");
                    printf("%.3g",subshells[n].getProb(i));
                }
            }
            printf("\n");
        }
    }
}

double ConversionGamma::getConversionEffic() const {
    double ce = 0;
    for(unsigned int n=0; n<subshells.size(); n++)
        ce += getPVacant(n);
    return ce;
}

double ConversionGamma::shellAverageE(unsigned int n) const {
    smassert(n<subshells.size());
    double e = 0;
    double w = 0;
    for(unsigned int i=0; i<subshells[n].getN(); i++) {
        double p = subshells[n].getProb(i);
        w += p;
        e += (Egamma-toAtom->BET->getSubshellBinding(n,i))*p;
    }
    return e/w;
}

float_err ConversionGamma::averageE() const {
    double e = 0;
    double w = 0;
    for(unsigned int n=0; n<subshells.size(); n++) {
        double p = shells.getProb(n);
        e += shellAverageE(n)*p;
        w += p;
    }
    e /= w;
    double serr = 0;
    for(unsigned int n=0; n<subshells.size(); n++) {
        double u = (shellAverageE(n)-e)*shellUncert[n];
        serr += u*u;
    }
    return float_err(e,sqrt(serr)/w);
}

void ConversionGamma::scale(double s) {
    TransitionBase::scale(s);
    Igamma /= s;
    shells.scale(s);
}

//-----------------------------------------

BetaDecayTrans::BetaDecayTrans(NucLevel& f, NucLevel& t, bool pstrn, unsigned int forbidden):
TransitionBase(f,t), positron(pstrn), BSG(to.A,to.Z*(positron?-1:1),from.E-to.E),
betaTF1((f.name+"-"+t.name+"_Beta").c_str(),this,&BetaDecayTrans::evalBeta,0,1,0) {
    BSG.forbidden = forbidden;
    betaTF1.SetNpx(1000);
    betaTF1.SetRange(0,from.E-to.E);
    if(from.jpi==to.jpi) { BSG.M2_F = 1; BSG.M2_GT = 0; }
    else { BSG.M2_GT = 1; BSG.M2_F = 0; } // TODO not strictly true; need more general mechanism to fix
    betaQuantiles = new TF1_Quantiles(betaTF1);
}

BetaDecayTrans::~BetaDecayTrans() {
    delete betaQuantiles;
}

void BetaDecayTrans::run(vector<NucDecayEvent>& v, double* rnd) {
    NucDecayEvent evt;
    evt.d = positron?D_POSITRON:D_ELECTRON;
    evt.randp(rnd);
    if(rnd) evt.E = betaQuantiles->eval(rnd[2]);
    else evt.E = betaTF1.GetRandom();
    v.push_back(evt);
}

double BetaDecayTrans::evalBeta(double* x, double*) { return BSG.decayProb(x[0]); }

//-----------------------------------------

void ECapture::run(vector<NucDecayEvent>&, double*) {
    isKCapt = gRandom->Uniform(0,1) < toAtom->IMissing;
}

//-----------------------------------------

bool sortLevels(const NucLevel& a, const NucLevel& b) { return (a.E < b.E); }

NucDecaySystem::NucDecaySystem(const SMFile& Q, const BindingEnergyLibrary& B, double t): BEL(B) {
    
    fancyname = Q.getDefault("fileinfo","fancyname","");
    
    // load levels data
    vector<Stringmap> levs = Q.retrieve("level");
    for(auto& l: levs) {
        levels.push_back(NucLevel(l));
        transIn.push_back(vector<TransitionBase*>());
        transOut.push_back(vector<TransitionBase*>());
        levelDecays.push_back(PSelector());
    }
    std::sort(levels.begin(),levels.end(),sortLevels);
    int nlev = 0;
    for(auto& l: levels) {
        smassert(levelIndex.find(l.name) == levelIndex.end());
        l.n = nlev++;
        levelIndex.emplace(l.name, l.n);
    }
    
    // set up internal conversions
    vector<Stringmap> gammatrans = Q.retrieve("gamma");
    for(auto& g: gammatrans) {
        addTransition(new ConversionGamma(levels[levIndex(g.getDefault("from",""))],levels[levIndex(g.getDefault("to",""))],g));
    }
    if(Q.getDefault("norm","gamma","") == "groundstate") {
        double gsflux = 0;
        for(auto& l: levels)
            if(!l.fluxOut)
                gsflux += l.fluxIn;
            for(auto& t: transitions)
                t->scale(1./gsflux);
            for(auto& l: levels)
                l.scale(1./gsflux);
    }
    
    // set up Augers
    for(auto& t: transitions)
        t->toAtom->ICEK += t->getPVacant(0)*t->Itotal;
    vector<Stringmap> augers = Q.retrieve("AugerK");
    for(auto& a: augers) {
        int Z = a.getDefault("Z",0);
        if(!Z) {
            SMExcept e("BadAugerZ");
            e.insert("Z",Z);
            throw(e);
        }
        getAtom(Z)->load(a);
    }
    
    // set up beta decays
    vector<Stringmap> betatrans = Q.retrieve("beta");
    for(auto& bt: betatrans) {
        BetaDecayTrans* BD = new BetaDecayTrans(levels[levIndex(bt.getDefault("from",""))],
                                                levels[levIndex(bt.getDefault("to",""))],
                                                bt.getDefault("positron",0),
                                                bt.getDefault("forbidden",0));
        BD->Itotal = bt.getDefault("I",0)/100.0;
        if(bt.count("M2_F") || bt.count("M2_GT")) {
            BD->BSG.M2_F = bt.getDefault("M2_F",0);
            BD->BSG.M2_GT = bt.getDefault("M2_GT",0);
        }
        addTransition(BD);
    }
    
    // set up electron captures
    vector<Stringmap> ecapts = Q.retrieve("ecapt");
    for(auto& ec: ecapts) {
        NucLevel& Lorig = levels[levIndex(ec.getDefault("from",""))];
        string to = ec.getDefault("to","AUTO");
        if(to == "AUTO") {
            for(auto& Ldest: levels) {
                if(Ldest.A == Lorig.A && Ldest.Z+1 == Lorig.Z && Ldest.E < Lorig.E) {
                    double missingFlux = Ldest.fluxOut - Ldest.fluxIn;
                    if(missingFlux <= 0) continue;
                    ECapture* EC = new ECapture(Lorig,Ldest);
                    EC->Itotal = missingFlux;
                    addTransition(EC);
                }
            }
        } else {
            NucLevel& Ldest = levels[levIndex(to)];
            smassert(Ldest.A == Lorig.A && Ldest.Z+1 == Lorig.Z && Ldest.E < Lorig.E);
            ECapture* EC = new ECapture(Lorig,Ldest);
            EC->Itotal = ec.getDefault("I",0.);
            addTransition(EC);
        }
    }
    
    setCutoff(t);
}

NucDecaySystem::~NucDecaySystem() {
    for(auto& t: transitions) delete t;
    for(auto& kv: atoms) delete kv.second;
}

DecayAtom* NucDecaySystem::getAtom(unsigned int Z) {
    auto it = atoms.find(Z);
    if(it != atoms.end()) return it->second;
    DecayAtom* A = new DecayAtom(BEL.getBindingTable(Z));
    atoms.emplace(Z,A);
    return A;
}

void NucDecaySystem::addTransition(TransitionBase* T) {
    T->toAtom = getAtom(T->to.Z);
    transIn[T->to.n].push_back(T);
    transOut[T->from.n].push_back(T);
    levelDecays[T->from.n].addProb(T->Itotal);
    T->from.fluxOut += T->Itotal;
    T->to.fluxIn += T->Itotal;
    transitions.push_back(T);
}

void NucDecaySystem::setCutoff(double t) {
    tcut = t;
    lStart = PSelector();
    for(unsigned int n=0; n<levels.size(); n++) {
        levelDecays[n] = PSelector();
        for(unsigned int t = 0; t < transOut[n].size(); t++)
            levelDecays[n].addProb(transOut[n][t]->Itotal);
        
        double pStart = (n+1==levels.size());
        if(!pStart && levels[n].hl > tcut && transOut[n].size())
            for(unsigned int t = 0; t < transIn[n].size(); t++)
                pStart += transIn[n][t]->Itotal;
            lStart.addProb(pStart);
    }
}

void NucDecaySystem::display(bool verbose) const {
    printf("---- Nuclear Level System ----\n");
    printf("---- %i DF\n",getNDF());
    displayLevels(verbose);
    displayAtoms(verbose);
    displayTransitions(verbose);
    printf("------------------------------\n");
}

void NucDecaySystem::displayLevels(bool verbose) const {
    printf("---- Energy Levels ----\n");
    for(vector<NucLevel>::const_iterator it = levels.begin(); it != levels.end(); it++) {
        printf("[%i DF] ",getNDF(it->n));
        it->display(verbose);
    }
}

void NucDecaySystem::displayTransitions(bool verbose) const {
    printf("---- Transitions ----\n");
    for(unsigned int i = 0; i<transitions.size(); i++) {
        printf("(%i) ",i);
        transitions[i]->display(verbose);
    }
}

void NucDecaySystem::displayAtoms(bool verbose) const {
    printf("---- Atoms ----\n");
    for(map<unsigned int, DecayAtom*>::const_iterator it = atoms.begin(); it != atoms.end(); it++)
        it->second->display(verbose);
}


unsigned int NucDecaySystem::levIndex(const std::string& s) const {
    map<std::string,unsigned int>::const_iterator n = levelIndex.find(s);
    if(n==levelIndex.end()) {
        SMExcept e("UnknownLevel");
        e.insert("name",s);
        throw(e);
    }
    return n->second;
}

void NucDecaySystem::genDecayChain(vector<NucDecayEvent>& v, double* rnd, unsigned int n) {
    bool init = n>=levels.size();
    if(init)
        n = lStart.select(rnd);
    if(!levels[n].fluxOut || (!init && levels[n].hl > tcut)) return;
    
    TransitionBase* T = transOut[n][levelDecays[n].select(rnd)];
    T->run(v, rnd);
    if(rnd) rnd += T->getNDF(); // remove random numbers "consumed" by continuous processes
    unsigned int nAugerK = T->nVacant(0);
    while(nAugerK--)
        getAtom(T->to.Z)->genAuger(v);
    
    genDecayChain(v, rnd, T->to.n);
}

unsigned int NucDecaySystem::getNDF(unsigned int n) const {
    unsigned int ndf = 0;
    if(n>=levels.size()) {
        // maximum DF over all starting levels
        for(unsigned int i=0; i<levels.size(); i++) {
            if(!lStart.getProb(i)) continue;
            unsigned int lndf = getNDF(i);
            ndf = lndf>ndf?lndf:ndf;
        }
    } else {
        // maximum DF over all transitions from this level
        for(vector<TransitionBase*>::const_iterator it = transOut[n].begin(); it != transOut[n].end(); it++) {
            unsigned int lndf = (*it)->getNDF()+getNDF((*it)->to.n);
            ndf = lndf>ndf?lndf:ndf;
        }
    }
    return ndf;
}

void NucDecaySystem::scale(double s) {
    lStart.scale(s);
    for(unsigned int i = 0; i<transitions.size(); i++)
        transitions[i]->scale(s);
    for(unsigned int i = 0; i<levels.size(); i++) {
        levels[i].scale(s);
        levelDecays[i].scale(s);
    }
}

//-----------------------------------------

NucDecayLibrary::NucDecayLibrary(const std::string& datp, double t):
datpath(datp), tcut(t), BEL(SMFile(datpath+"/ElectronBindingEnergy.txt")) { }

NucDecayLibrary::~NucDecayLibrary() {
    for(auto& kv: NDs) delete kv.second;
}

NucDecaySystem& NucDecayLibrary::getGenerator(const std::string& nm) {
    auto it = NDs.find(nm);
    if(it != NDs.end()) return *(it->second);
    string fname = datpath+"/"+nm+".txt"; 
    if(!fileExists(fname)) {
        SMExcept e("MissingDecayData");
        e.insert("fname",fname);
        throw(e);
    }
    return *(NDs.emplace(nm, new NucDecaySystem(SMFile(fname),BEL,tcut)).first->second);
}

bool NucDecayLibrary::hasGenerator(const std::string& nm) {
    if(cantdothis.count(nm)) return false;
    try {
        getGenerator(nm);
        return true;
    } catch(...) {
        cantdothis.insert(nm);
    }
    return false;
}

//-----------------------------------------


GammaForest::GammaForest(const std::string& fname, double E2keV) {
    if(!fileExists(fname)) {
        SMExcept e("fileUnreadable");
        e.insert("filename",fname);
        throw(e);
    }
    std::ifstream fin(fname.c_str());
    string s;
    while (fin.good()) {
        std::getline(fin,s);
        s = strip(s);
        if(!s.size() || s[0]=='#')
            continue;
        vector<double> v = sToDoubles(s," ,\t");
        if(v.size() != 2) continue;
        gammaE.push_back(v[0]*E2keV);
        gammaProb.addProb(v[1]);
    }
    fin.close();
    printf("Located %i gammas with total cross section %g\n",(int)gammaE.size(),gammaProb.getCumProb());
}

void GammaForest::genDecays(vector<NucDecayEvent>& v, double n) {
    while(n>=1. || gRandom->Uniform(0,1)<n) {
        NucDecayEvent evt;
        evt.d = D_GAMMA;
        evt.t = 0;
        evt.E = gammaE[gammaProb.select()];
        v.push_back(evt);
        --n;
    }
}


//-----------------------------------------

void square2circle(double& x, double& y, double r) {
    double th = 2*M_PI*x;
    r *= sqrt(y);
    x = r*cos(th);
    y = r*sin(th);
}

void CubePosGen::genPos(double* v, double* rnd) const {
    for(AxisDirection d = X_DIRECTION; d <= Z_DIRECTION; ++d)
        v[d] = rnd?rnd[d]:gRandom->Uniform(0,1);
}

void CylPosGen::genPos(double* v, double* rnd) const {
    for(AxisDirection d = X_DIRECTION; d <= Z_DIRECTION; ++d)
        v[d] = rnd?rnd[d]:gRandom->Uniform(0,1);
    square2circle(v[X_DIRECTION],v[Y_DIRECTION],r);
    v[Z_DIRECTION] = (v[Z_DIRECTION]-0.5)*dz;
}

//------------------------------------------------------------------------------


void EventTreeScanner::setReadpoints(TTree* T) {
    SetBranchAddress(T,"num",&evt.eid);
    SetBranchAddress(T,"PID",&evt.d);
    SetBranchAddress(T,"KE",&evt.E);
    SetBranchAddress(T,"vertex",evt.x);
    SetBranchAddress(T,"direction",evt.p);
    SetBranchAddress(T,"time",&evt.t);
    SetBranchAddress(T,"weight",&evt.w);
}

int EventTreeScanner::addFile(const std::string& filename) {
    int nf = TChainScanner::addFile(filename);
    startScan();
    nextPoint();
    prevN = evt.eid;
    firstpass = true;
    return nf;
}

unsigned int EventTreeScanner::loadEvt(vector<NucDecayEvent>& v) {
    unsigned int nevts = 0;
    do {
        v.push_back(evt);
        ++nevts;
        nextPoint();
    } while(prevN==evt.eid);
    firstpass &= evt.eid>=prevN;
    prevN=evt.eid;
    return nevts;
}

//------------------------------------------------------------------------------


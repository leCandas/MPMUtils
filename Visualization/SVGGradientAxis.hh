/// \file SVGGradientAxis.hh Color gradient z-axis for SVG plots
// This file was produced under the employ of the United States Government,
// and is consequently in the PUBLIC DOMAIN, free from all provisions of
// US Copyright Law (per USC Title 17, Section 105).
//
// -- Michael P. Mendenhall, 2015

#ifndef SVGGRADIENTAXIS_HH
#define SVGGRADIENTAXIS_HH

#include "SVGBuilder.hh"
#include <set>
using std::set;

/// D-dimensional plane equation
template<size_t D, typename val_tp>
class PlaneEquation {
public:
    /// Constructor
    PlaneEquation() { }
    /// Evaluate at point
    val_tp operator()(const val_tp* x) const {
        val_tp s = 0;
        for(size_t i=0; i<D; i++) s += P[i+1]*(x[i]-x0[i]);
        return s;
    }

    val_tp x0[D];       ///< relative centerpoint
    val_tp P[D+1];      ///< coefficients, y = P[0] + P[i+1]*x[i]
};

/// Color axis
class SVGGradientAxis {
public:
    /// Constructor
    SVGGradientAxis();
    /// normalize to axis internal coordinates
    double axisUnits(double x) const;
    /// derivative of axis transformation
    double dAxisUnits(double x) const;
    /// finalize range; set up text
    void finalize();
    /// Determine gradient mapping given face plane equation
    string gradient_remap(const PlaneEquation<2,float>& P) const;
    /// add an axis label tick
    void addtick(double z, const string& lbl = "auto", int lvl = 0);

    struct tick {
        double z;
        int level;
        string label;
        bool operator<(const tick& rhs) const { return z < rhs.z; }
    };

    bool logscale = false;                                      ///< log scale setting
    BBox<1,double> range = BBox<1,double>::nullBox();           ///< axis range
    set<tick> axticks;                                          ///< axis tick locations
    shared_ptr<SVG::group> axisGroup = make_shared<SVG::group>();       ///< group containing axis information

    color::Gradient G;                                          ///< gradient color definition
    shared_ptr<XMLBuilder> Gaxis = make_shared<XMLBuilder>("linearGradient");       ///< axis plot SVG
    shared_ptr<SVG::lingradient> base_gradient;                 ///< gradient in SVG form
};

#endif
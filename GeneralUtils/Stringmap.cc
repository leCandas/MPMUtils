/// \file Stringmap.cc
/* 
 * Stringmap.cc, part of the MPMUtils package.
 * Copyright (c) 2014 Michael P. Mendenhall
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Stringmap.hh"

#include <iostream>
#include <sstream>
#include <fstream>
#include <utility>
#include "StringManip.hh"
#include "PathUtils.hh"
#include "SMExcept.hh"

Stringmap::Stringmap(const string& str) {
    auto pairs = split(str,"\t");
    for(auto const& s: pairs) {
        auto keyval = split(s,"=");
        if(keyval.size() != 2)
            continue;
        dat.emplace(strip(keyval[0]),strip(keyval[1]));
    }
}

void Stringmap::insert(const string& s, const string& v) {
    dat.emplace(s,v);
}

void Stringmap::insert(const string& s, double d) {
    insert(s,to_str(d));
}

void Stringmap::erase(const string& s) { dat.erase(s); }

vector<string> Stringmap::retrieve(const string& s) const {
    vector<string> v;
    for(multimap<string,string>::const_iterator it = dat.lower_bound(s); it != dat.upper_bound(s); it++)
        v.push_back(it->second);
    return v;
}

string Stringmap::getDefault(const string& s, const string& d) const {
    multimap<string,string>::const_iterator it = dat.find(s);
    if(it == dat.end())
        return d;
    return it->second;
}

string Stringmap::toString() const {
    string s;
    for(auto const& kv: dat)
        s += "\t" + kv.first + " = " + kv.second;
    return s;
}

void Stringmap::display(string linepfx) const {
    for(auto const& kv: dat)
        std::cout << linepfx << kv.first << ": " << kv.second << "\n";
}


double Stringmap::getDefault(const string& k, double d) const {
    string s = getDefault(k,"");
    if(!s.size())
        return d;
    std::stringstream ss(s);
    ss >> d;
    return d;
}

vector<double> Stringmap::retrieveDouble(const string& k) const {
    vector<double> v;
    double d;
    for(auto const& ss: retrieve(k)) {
        std::stringstream s(ss);
        s >> d;
        v.push_back(d);
    }
    return v;
}

void Stringmap::mergeInto(Stringmap& S) const {
    for(auto const& kv: dat) S.insert(kv.first,kv.second);
}

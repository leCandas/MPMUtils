/// \file XMLBuilder.hh Simple XML output class

// This file was produced under the employ of the United States Government,
// and is consequently in the PUBLIC DOMAIN, free from all provisions of
// US Copyright Law (per USC Title 17, Section 105).
// 
// -- Michael P. Mendenhall, 2015

#ifndef XMLBUILDER_HH
#define XMLBUILDER_HH

#include "RefCounter.hh"
#include "StringManip.hh"

#include <map>
using std::map;
#include <vector>
using std::vector;
#include <cassert>
#include <iostream>
using std::ostream;

/// Reference-counted XML tag
class XMLBuilder: public RefCounter {
public:
    /// Constructor
    XMLBuilder(const string& nm = ""): name(nm) { }
    /// Destructor
    virtual ~XMLBuilder() { for(auto c: children) c->release(); }
    
    /// Add child node
    virtual void addChild(XMLBuilder* C) { assert(C); C->retain(); children.push_back(C); }
    /// Add a tag attribute
    virtual void addAttr(const string& nm, const string& val) { attrs[nm] = val; }
    /// Add numerical attribute
    virtual void addAttr(const string& nm, double val) { addAttr(nm, to_str(val)); }
    
    /// Write output
    virtual void write(ostream& o, unsigned int ndeep = 0);
    
    string name;                        ///< tag head
    bool oneline = false;               ///< whether to force single-line output
    map<string,string> attrs;           ///< tag attributes
    static string indent;               ///< indentation string
    
protected:
    
    vector<XMLBuilder*> children;       ///< child nodes
    
    /// subclass me! setup before write
    virtual void prepare() { }
    /// generate closing tag
    void closeTag(ostream& o, bool abbrev = false);
};

/// "verbatim contents" XML-includable text
class XMLText: public XMLBuilder {
public:
    /// Constructor
    XMLText(const string& c): contents(c) { }
    /// write output
    virtual void write(ostream& o, unsigned int ndeep = 0) { while(ndeep--) o << indent; o << contents; }
    string contents;    ///< text to include between tags
};

/// Base class for objects that can provide XML output ``on demand''
class XMLProvider {
public:
    /// Constructor
    XMLProvider(const string& name): tagname(name) { }
    /// Destructor
    virtual ~XMLProvider() { }
    /// build XML output
    XMLBuilder* makeXML();
    /// Add child node
    virtual void addChild(XMLProvider* C) { assert(C); children.push_back(C); }
    /// Add a tag attribute
    virtual void addAttr(const string& nm, const string& val) { xattrs[nm] = val; }
    /// Add numerical attribute
    virtual void addAttr(const string& nm, double val) { addAttr(nm, to_str(val)); }
    
    string tagname;                     ///< this item's tag name

protected:
    /// add class-specific XML data; subclass me!
    virtual void _makeXML(XMLBuilder&) { }
    
    map<string,string> xattrs;          ///< tag attributes
    vector<XMLProvider*> children;      ///< child providers
};

#endif

/// \file ControlMenu.hh interactive text-based menu interface
/*
 * ControlMenu.hh, part of the MPMUtils package.
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

#ifndef CONTROLMENU_HH
#define CONTROLMENU_HH 1

#include "StringManip.hh"
#include <cassert>
#include <deque>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <map>

using std::deque;
using std::stack;
using std::map;

/// base class for interacting with a deque of strings
class StreamInteractor {
public:
    /// constructor
    StreamInteractor(): mydeque(nullptr), mystack(nullptr)  {}
    /// destructor
    virtual ~StreamInteractor() {}
    /// do something! (put something useful here in subclasses)
    virtual void doIt() {}

    /// check that there are at least the specified number of items on the stack
    bool menutils_CheckStackSize(unsigned int n);

    /// pop string off front of deque
    string popStringD() { assert(mydeque); string s = mydeque->front(); mydeque->pop_front(); return s; }
    /// pop int off front of deque
    int popIntD() { return atoi(popStringD().c_str()); }
    /// pop float off front of deque
    float popFloatD() { return atof(popStringD().c_str()); }

    /// pop string off stack
    string popString() { assert(mystack); string s = mystack->top(); mystack->pop(); return s; }
    /// pop int off stack
    int popInt() { return atoi(popString().c_str()); }
    /// pop float off stack
    float popFloat() { return atof(popString().c_str()); }

    deque<string>* mydeque;     ///< command arguments deque
    stack<string>* mystack;     ///< working space stack
};

/// stream interactor with a name for screen display
class NamedInteractor: public StreamInteractor {
public:
    /// constructor
    explicit NamedInteractor(string nm): name(nm) {}
    /// get my name/description
    virtual string getDescription() { return name; }

    string name; ///< name for this interactor
};


/// stream interactor with named/numbered arguments that can prompt for necessary input
class InputRequester: public NamedInteractor {
public:
    /// constructor
    explicit InputRequester(string d, void (*f)(StreamInteractor*) = nullptr, StreamInteractor* fObj = nullptr);
    /// action when selected
    void doIt() override;
    /// add new argument
    virtual void addArg(const string& s, const string& dflt = "", const string& descrip = "", NamedInteractor* filter = nullptr);
    /// add new argument, assuming descriptions come from filter
    virtual void addArg(NamedInteractor* filter, const string& s = "") { addArg(s,"","",filter); }
    /// set argument parameters
    virtual void setArgOpts(unsigned int i, string s, string dflt = "", NamedInteractor* filter = nullptr);
    /// get option name
    virtual string getArgname(unsigned int i) const;
    ///get my name/description
    string getDescription() override;

    static InputRequester exitMenu;

protected:
    vector<string> argNames;                    ///< names of arguments
    vector<string> argDescrips;                 ///< extended descriptions of arguments
    vector<string> defaultArgs;                 ///< default values for arguments
    vector<NamedInteractor*> inputFilters;      ///< input data filters
    StreamInteractor* myFuncObject;             ///< object called with function
    void (*myFunc)(StreamInteractor*);          ///< function to do when selected
};


/// option display/activity flags
enum Selector_Option_Flags {
    SELECTOR_NORMAL = 0,        ///< normal mode
    SELECTOR_HIDDEN = 1<<0,     ///< option is hidden in menu
    SELECTOR_DISABLED = 1<<1,   ///< option is inactive in menu
    SELECTOR_SYNONYM = 1<<2     ///< option is a synonym for another option
};

/// default soft-matching routine
bool nameselector_default_softmatch(const string& a, const string& b);

/// class for selecting an item from a list and applying an associated action
class NameSelector: public InputRequester {
public:
    /// constructor
    explicit NameSelector(string t, string promptval = "Selection", bool persist = false);
    /// add selection choice
    virtual void addChoice(string d, string nm = "", Selector_Option_Flags o = SELECTOR_NORMAL, string mname = "", StreamInteractor* action = nullptr);
    /// display available options
    virtual void displayOptions();
    /// get choice from input queue, return selected item on stack
    void doIt() override;
    /// get my name/description
    string getDescription() override;
    /// set default choice
    virtual void setDefault(string s) { defaultArgs[0] = s; }
    /// set catchall action
    virtual void setCatchall(StreamInteractor* SI) { catchAll = SI; }
    /// prevent adding arguments (doesn't make sense in this context)
    void addArg(const string&, const string& = "", const string& = "", NamedInteractor* = nullptr) override { throw; }
    /// prevent adding arguments (doesn't make sense in this context)
    void addArg(NamedInteractor*, const string& = "") override { throw; }
    /// add a synonym for an existing argument
    virtual void addSynonym(string arg0, string syn);
    /// set soft-matching function (set to nullptr to disable soft matching)
    void setSoftmatch(bool (*f)(const string& a, const string& b) = &nameselector_default_softmatch) { softmatch = f; }
    static string barf_control;
    static string exit_control;

protected:
    map<string,unsigned int> nameMap;      ///< map from choice names to selected content
    vector<string> choiceNames;                 ///< choice names
    vector<string> choiceDescrips;              ///< choice descriptions
    vector<string> choiceOut;                   ///< output for each choice
    vector<StreamInteractor*> actions;          ///< output action for each choice
    vector<Selector_Option_Flags> oflags;       ///< option display flags
    StreamInteractor* catchAll;                 ///< catch-all action for unidentified choices
    bool isPersistent;                          ///< whether menu is persistent (repeats after selection is made)
    bool (*softmatch)(const string& a, const string& b);      ///< function for "soft matching" comparison of selections
};

/// Text menu of selectable items (simplified NameSelector specifically for actions)
class OptionsMenu: public NameSelector {
public:
    /// constructor
    explicit OptionsMenu(string t, bool persist = true): NameSelector(t,"Selection",persist) { }
    /// destructor
    virtual ~OptionsMenu() {}
    /// add choice to selections list
    virtual void addChoice(NamedInteractor* M, string nm = "", Selector_Option_Flags o = SELECTOR_NORMAL);
    /// prevent adding choices through base mechanism
    void addChoice(string, string, Selector_Option_Flags, string, StreamInteractor*) override { throw; }
};



// standard utility functions

/// print contents of control queue
void menutils_PrintQue(StreamInteractor*);
/// print contents of control queue
void menutils_PrintStack(StreamInteractor*);
/// push stack size onto stack
void menutils_StackSize(StreamInteractor*);
/// drop top stack item
void menutils_Drop(StreamInteractor*);
/// duplicate stack item
void menutils_Dup(StreamInteractor*);
/// drop top n stack items
void menutils_DropN(StreamInteractor*);
/// clear stack
void menutils_ClearStack(StreamInteractor*);
/// swap top two stack items
void menutils_Swap(StreamInteractor*);
/// rotate n stack items, bringing nth item to top
void menutils_Rot(StreamInteractor*);
/// select c?a:b
void menutils_Select(StreamInteractor*);
/// move string on stack to command stream for execution
void menutils_Exec(StreamInteractor*);
/// add error code ("BARF") to control queue
void menutils_Barf(StreamInteractor*);
/// add exit code ("EXIT") to control queue
void menutils_Exit(StreamInteractor*);

#endif

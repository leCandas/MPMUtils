/// \file HDF5_Table_Cache.hh Template utility class for memory-cache buffered HDF5 tables IO
// -- Michael P. Mendenhall, LLNL 2019

#ifndef HDF5_TABLE_CACHE_HH
#define HDF5_TABLE_CACHE_HH

#include "HDF5_StructInfo.hh"
#include "DataSource.hh"
using std::multimap;

/// Cacheing HDF5 table reader
template<typename T>
class HDF5_Table_Cache: virtual public DataSource<T> {
public:
    /// Constructor, from name of table and struct offsets/sizes
    HDF5_Table_Cache(const HDF5_Table_Spec& ts = HDF5_table_setup<T>(), hsize_t nc = 1024): Tspec(ts), nchunk(nc)  { }

    /// get next table row; return whether successful or failed (end-of-file)
    bool next(T& val) override;
    /// skip ahead number of entries
    bool skip(size_t n) override;
    /// Re-start at beginning of stream
    void reset() override { setFile(_infile_id); }
    /// Remaining entries estimate
    size_t entries() override { return nLoad >= 0 && hsize_t(nLoad) < nRows? nLoad - nread : nRows - nread; }

    /// (re)set read file
    void setFile(hid_t f);
    /// get number of rows read
    hsize_t getNRead() const { return nread; }
    /// get number of rows available
    hsize_t getNRows() const { return nRows; }
    /// get identifying number for value type
    static int64_t getIdentifier(const T& i) { return i.evt; }
    /// set identifying number for value type
    static void setIdentifier(T& i, int64_t n) { i.evt = n; }

    /// load next "event" of entries with same identifer into vector; return event identifier loaded
    int64_t loadEvent(vector<T>& v);
    /// load all data into map by event number
    void loadAll(multimap<int64_t, T>& dat);

    HDF5_Table_Spec Tspec;      ///< configuration for table to read
    int nLoad = -1;             ///< entries loading limit; set >= 0 to apply

protected:
    hid_t _infile_id = 0;       ///< file to read from
    T next_read;                ///< next item read in for event list reads
    int64_t id_current_evt = -1;  ///< event identifier of next_read

    vector<T> cached;           ///< cached read data
    size_t cache_idx = 0;       ///< index in cached data
    hsize_t nread = 0;          ///< number of rows read
    hsize_t nRows = 0;          ///< number of rows in table
    hsize_t nfields = 0;        ///< number of fields in table
    hsize_t nchunk;             ///< cacheing chunk size
};

/// Cacheing HDF5 table writer
template<typename T>
class HDF5_Table_Writer: virtual public DataSink<const T> {
public:
    /// Constructor, from name of table and struct offsets/sizes
    HDF5_Table_Writer(const HDF5_Table_Spec& ts = HDF5_table_setup<T>(), hsize_t nc = 1024, int cmp = 9): Tspec(ts), nchunk(nc), compress(cmp) { }
    /// Destructor
    ~HDF5_Table_Writer() { HDF5_Table_Writer::signal(DATASTREAM_END); }

    /// write table row
    void push(const T& val) override;
    /// write table rows
    void push(const vector<T>& vals);
    /// get number of rows written
    hsize_t getNWrite() const { return nwrite; }
    /// (re)set output file
    void setFile(hid_t f);
    /// create table in output file
    void initTable() { makeTable(Tspec, _outfile_id, nchunk, compress); }
    /// accept data flow signal
    void signal(datastream_signal_t sig) override;

    HDF5_Table_Spec Tspec;      ///< configuration for table to read

protected:
    hid_t _outfile_id = 0;      ///< file to write to
    hsize_t nwrite = 0;         ///< number of rows written

    vector<T> cached;           ///< cached output data
    hsize_t nchunk;             ///< cacheing chunk size
    int compress;               ///< output compression level
};

/// Combined HDF5 reader/writer for transferring select events subset
template<typename T>
class HDF5_Table_Transfer {
public:
    /// Constructor
    explicit HDF5_Table_Transfer(const HDF5_Table_Spec& ts, hsize_t nc = 1024): tableIn(ts,nc), tableOut(ts,nc) { }
    /// Transfer all entries with specified ID (assumed ascending), optionally re-numbering; return false at EOF.
    bool transferID(int64_t id, int64_t newID = -1);
    /// Transfer a (sorted ascending) list of ID-numbered rows, optionally renumbering
    bool transferIDs(const vector<int64_t>& ids, int64_t newID = -1);

    T row;                              ///< table row being transferred
    HDF5_Table_Cache<T> tableIn;        ///< input table
    HDF5_Table_Writer<T> tableOut;      ///< output table
};

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

template<typename T>
void HDF5_Table_Writer<T>::push(const vector<T>& vals) {
    cached.insert(cached.end(), vals.begin(), vals.end());
    if(cached.size() >= nchunk) signal(DATASTREAM_FLUSH);
    nwrite += vals.size();
}

template<typename T>
void HDF5_Table_Writer<T>::push(const T& val) {
    cached.push_back(val);
    if(cached.size() >= nchunk) signal(DATASTREAM_FLUSH);
    nwrite++;
}

template<typename T>
void HDF5_Table_Writer<T>::setFile(hid_t f) {
    signal(DATASTREAM_FLUSH);
    _outfile_id = f;
}

template<typename T>
void HDF5_Table_Writer<T>::signal(datastream_signal_t sig) {
    if(sig < DATASTREAM_FLUSH) return;
    if(_outfile_id && cached.size()) {
        herr_t err = H5TBappend_records(_outfile_id,  Tspec.table_name.c_str(), cached.size(),
                                        sizeof(T),  Tspec.offsets, Tspec.field_sizes, cached.data());
        if(err < 0) throw std::runtime_error("Failed to append records to HDF5 table '" + Tspec.table_name + "'");
    }
    cached.clear();
}
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

template<typename T>
void HDF5_Table_Cache<T>::setFile(hid_t f) {
    _infile_id = f;
    cached.clear();
    cache_idx = nread = nRows = 0;
    if(f) {
        if(H5Lexists(_infile_id,  Tspec.table_name.c_str(), H5P_DEFAULT)) {
            herr_t err = H5TBget_table_info(_infile_id,  Tspec.table_name.c_str(), &nfields, &nRows);
            if(err < 0) throw std::exception();
        } else {
            printf("Warning: table '%s' not present in file.\n", Tspec.table_name.c_str());
            _infile_id = 0;
        }
    }
    id_current_evt = -1;
}

template<typename T>
bool HDF5_Table_Cache<T>::next(T& val) {
    if(!_infile_id) return false;

    if(cache_idx >= cached.size()) { // cache exhausted, needs new data
        if(nread == nRows || nread == hsize_t(nLoad)) {  // input exhausted.
            nread = 0;          // Next `next()` call will return to start of file.
            cache_idx = 0;
            cached.clear();
            return false;
        }

        hsize_t nToRead = std::min(nchunk, hsize_t(entries()));
        if(!nToRead) return false;

        cached.resize(nToRead);
        cache_idx = 0;
        herr_t err = H5TBread_records(_infile_id, Tspec.table_name.c_str(), nread, nToRead,
                                      sizeof(T),  Tspec.offsets, Tspec.field_sizes, cached.data());
        if(err < 0) throw std::runtime_error("Unexpected failure reading HDF5 file");
        nread += nToRead;
    }

    val = cached.at(cache_idx++);
    return true;
}

template<typename T>
bool HDF5_Table_Cache<T>::skip(size_t n) {
    if(!n) return true;
    if(!_infile_id) return false;

    if(cache_idx + n < cached.size()) {
        cache_idx += n;
        return true;
    }

    if(cache_idx < cached.size()) {
        n -= cached.size() - cache_idx;
        cache_idx = 0;
        cached.clear();
    }

    if(nread + n > nRows) {
        nread = nRows;
        return false;
    }
    nread += n;
    return true;
}

template<typename T>
int64_t HDF5_Table_Cache<T>::loadEvent(vector<T>& v) {
    v.clear();
    if(id_current_evt == -2) { // = -1 on the first time; -2 at file end
        id_current_evt = -1; // reset to restart at begin of data
        return -2;
    }

    if(id_current_evt != -1) v.push_back(next_read);

    while(true) {
        if(!next(next_read)) {
            auto i = id_current_evt;
            id_current_evt = -2;
            return i;
        }

        auto nextid = getIdentifier(next_read);
        if(id_current_evt == -1) id_current_evt = nextid;
        else if(nextid != id_current_evt) {
            id_current_evt = nextid;
            break;
        }
        v.push_back(next_read);
    }
    return id_current_evt;
}

template<typename T>
void HDF5_Table_Cache<T>::loadAll(multimap<int64_t, T>& dat) {
    T val;
    while(next(val)) dat.emplace(getIdentifier(val),val);
}

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

template<typename T>
bool HDF5_Table_Transfer<T>::transferID(int64_t id, int64_t newID) {
    int64_t current_id;
    if(!tableIn.getNRead() && !tableIn.next(row)) return false;
    while((current_id = tableIn.getIdentifier(row)) <= id) {
        if(current_id == id) {
            if(newID >= 0) tableIn.setIdentifier(row, newID);
            tableOut.push(row);
        }
        if(!tableIn.next(row)) return false;
    }
    return true;
}

template<typename T>
bool HDF5_Table_Transfer<T>::transferIDs(const vector<int64_t>& ids, int64_t newID) {
    for(auto id: ids) {
        if(!transferID(id, newID)) return false;
        if(newID >= 0) newID++;
    }
    tableOut.signal(DATASTREAM_FLUSH);
    return true;
}

#endif

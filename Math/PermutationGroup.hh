/// \file PermutationGroup.hh Group of fixed-size permutations

#ifndef PERMUTATIONGROUP_HH
#define PERMUTATIONGROUP_HH

#include "FiniteGroup.hh"

/// Compile-time-evaluable factorial function
constexpr inline size_t factorial(size_t i) { return i > 1? i*factorial(i-1) : 1; }

/// Permutation
template<size_t N>
class Permutation: protected array<int,N> {
public:
    /// parent class
    typedef array<int,N> super;

    /// Default constructor for identity permutation
    constexpr Permutation(): super(_id()) { }
    /// Permutation from array
    constexpr Permutation(const super& a): super(a) { assert(validate()); }

    /// element access
    int operator[](size_t i) const { return ((super&)*this)[i]; }
    /// swap two elements
    void swap(size_t i, size_t j) { std::swap((*this)[i], (*this)[j]); }

    /// equality comparison
    bool operator==(const Permutation& P) const { return (super&)*this == (super&)P; }
    /// inequality comparison
    bool operator!=(const Permutation& P) const { return (super&)*this != (super&)P; }
    /// ordering comparison
    bool operator<(const Permutation& P) const { return (super&)*this < (super&)P; }

    /// get inverse
    Permutation inverse() const {
        Permutation e;
        int j = 0;
        for(auto i: *this) e[i] = j++;
        return e;
    }

    /// out-of-place multiplication: permute first N elements of generic array A
    template<class A>
    A operator*(const A& a) const {
        A b = a;
        size_t j = 0;
        for(auto i: *this) b[j++] = a[i];
        return b;
    }
    /// inplace multiplication
    Permutation& operator*=(const Permutation& P) { return *this = (*this)*P; }
    /// inplace division
    Permutation& operator/=(const Permutation& P) { return *this *= P.inverse(); }
    /// out-of-place division
    Permutation operator/(const Permutation& P) const { auto p = *this; return p /= P; }

    /// enumeration index for permutation
    static constexpr size_t idx(const Permutation& e) {
        if(N<2) return 0;

        array<int, N-1> e0{};
        size_t i = 0;
        size_t j = N;
        for(auto& c: e0) {
            c = e[i++];
            if(c == N-1) j = c = e[N-1];
        }
        return Permutation<N-1>::idx(e0) + (j < N? j+1 : 0)*factorial(N-1);
    }
    /// index of permutation object
    size_t idx() const { return idx(*this); }
    /// permutation number i of N!
    static constexpr Permutation element(size_t i) {
        assert(i < factorial(N));
        Permutation p{};
        if(N<2 || !i) return p;

        auto nsub = factorial(N-1);
        auto j = i/nsub;
        if(j) p.swap(j-1, N-1);
        return Permutation<N-1>::element(i%nsub) * p;
    }

protected:
    /// build identity permutation
    static constexpr super _id() {
        super a{};
        int i = 0;
        for(auto& x: a) x = i++;
        return a;
    }

    /// verify this is valid permutation
    bool validate() const {
        set<int> i;
        for(auto c: *this) {
            if(c < 0 || c >= int(N)) return false;
            if(!i.insert(c).second) return false;
        }
        return i.size() == N;
    }

    /// mutable element access
    int& operator[](size_t i) { return ((super&)*this)[i]; }

    friend class Permutation<N-1>; // for applying sub-permutation
};
/// Null permutation special case, in case not optimized out
template<>
inline size_t Permutation<0>::idx(const Permutation&) { return 0; }
/// Null permutation special case, in case not optimized out
template<>
inline Permutation<0> Permutation<0>::element(size_t) { return {}; }

/// output representation for permutation
template<size_t N>
std::ostream& operator<<(std::ostream& o, const Permutation<N>& P) {
    o << "P_"<<N << "[" << P.idx() << "]";
    return o;
}

/// Symmetric Group of all permutations of N elements
template<size_t N>
class SymmetricGroup: public Permutation<N> {
public:
    /// Number of elements
    static constexpr size_t order = factorial(N);
    /// Permutation representation
    typedef Permutation<N> elem_t;

    /// group order
    static constexpr size_t getOrder() { return order; }

    /// identity element
    static constexpr elem_t identity() { return elem_t{}; }

    /// Get element inverse
    static constexpr elem_t inverse(elem_t a) { return a.inverse(); }

    /// Get group element c = ab
    static constexpr elem_t apply(elem_t a, elem_t b) { return a*b; }

    /// element iteration start
    static constexpr auto begin() { return esg_siterator<SymmetricGroup<N>>(); }
    /// element iteration end
    static constexpr auto end() { return esg_siterator<SymmetricGroup<N>>(getOrder()); }
};

/// Signed Permutation (combines permute with +/- sign flip)
template<size_t N>
class SignedPermutation: protected array<int,N> {
public:
    /// parent class
    typedef array<int,N> super;

    /// Default constructor for identity permutation
    constexpr SignedPermutation(): super(_id()) { }
    /// Permutation from array
    constexpr SignedPermutation(const super& a): super(a) { assert(validate()); }

    /// extract permutation component
    operator Permutation<N>() const {
        super a = (super&)*this;
        for(auto& c: a) c = abs(c)-1;
        return Permutation<N>(a);
    }

    /// get inverse
    SignedPermutation inverse() const {
        SignedPermutation e;
        int j = 1;
        for(auto i: *this) {
            e[abs(i)-1] = i < 0? -j : j;
            j++;
        }
        return e;
    }

    /// out-of-place multiplication: permute first N elements of generic array A
    template<class A>
    A operator*(const A& a) const {
        A b = a;
        size_t j = 0;
        for(auto i: *this) b[j++] = i < 0? -a[abs(i)-1] : a[abs(i)-1];
        return b;
    }

    /// inplace multiplication
    SignedPermutation& operator*=(const SignedPermutation& P) { return *this = (*this)*P; }
    /// inplace division
    SignedPermutation& operator/=(const SignedPermutation& P) { return *this *= P.inverse(); }
    /// out-of-place division
    SignedPermutation operator/(const SignedPermutation& P) const { auto p = *this; return p /= P; }
    /// swap all signs
    SignedPermutation operator-() const { auto p = *this; for(auto& c: p) c = -c; return p; }

protected:
    /// build identity permutation
    static constexpr super _id() {
        super a{};
        int i = 1;
        for(auto& x: a) x = i++;
        return a;
    }

    /// verify this is valid permutation
    bool validate() const {
        set<int> i;
        for(auto c: *this) {
            if(abs(c) < 1 || abs(c) > N) return false;
            if(!i.insert(abs(c)).second) return false;
        }
        return i.size() == N;
    }

    /// mutable element access
    int& operator[](size_t i) { return ((super&)*this)[i]; }

    friend class SignedPermutation<N-1>; // for applying sub-permutation
};

#endif
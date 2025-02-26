/*
 * BlockCMat.hh, part of the MPMUtils package.
 * Copyright (c) 2007-2014 Michael P. Mendenhall
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

/// \file "BlockCMat.hh" Block Circulant matrices, combining Circulant FFTW with internal LAPACKE matrix calculations
#ifndef BLOCKCMAT_HH
/// Make sure this header is only loaded once
#define BLOCKCMAT_HH

#include "CMatrix.hh"
#include "VarMat.hh"
#ifdef WITH_LAPACKE
#include "LAPACKE_Matrix.hh"
#endif

typedef VarMat<CMatrix> BlockCMat;

BlockCMat makeBlockCMatIdentity(size_t n, size_t mc);
BlockCMat makeBlockCMatRandom(size_t n, size_t mc);

/// singular-value decomposition of block circulant matrix
class BlockCMat_SVD: public BinaryOutputObject {
public:
    /// constructor
    explicit BlockCMat_SVD(const BlockCMat& BC);
    /// destructor
    ~BlockCMat_SVD();

    /// generate inverse at given singular value threshold
    const BlockCMat& calc_pseudo_inverse(double epsilon = 0);

    /// return sorted list of singular values, for threshold determination
    const vector<double>& singular_values() const { return svalues.getData(); }

    /// Dump binary data to file
    void writeToFile(ostream& o) const;
    /// Read binary data from file
    static BlockCMat_SVD* readFromFile(std::istream& s);

    /// sub-block singular values
    double getSV(size_t i) const;
    /// get enumerated right singular vector
    VarVec<double> getRightSVec(size_t i) const;

protected:
    /// empty constructor without calculation
    BlockCMat_SVD() {}

    /// prepare sorted singular values
    void sort_singular_values();

    size_t M, N, Mc, Ms;
    #ifdef WITH_LAPACKE
    vector< LAPACKE_Matrix_SVD<double,lapack_complex_double>* > block_SVDs;
    #endif
    VarVec<double> svalues;     ///< sorted singular values
    VarVec<size_t> sloc;        ///< location of sorted singular-values in sub-matrix
    BlockCMat* PsI;             ///< pseudo-inverse
    double PsI_epsilon;         ///< threshold for singular vectors
};

#endif

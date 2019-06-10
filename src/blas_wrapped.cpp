#include "blas_wrapped.hpp"
#include <iostream>
#include <type_traits>

template<typename P>
void copy(int *n, P *x, int *incx, P *y, int *incy, environment const environ)
{
  assert(n);
  assert(x);
  assert(incx);
  assert(y);
  assert(incy);
  assert(*incx >= 0);
  assert(*incy >= 0);
  assert(*n >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    dcopy_(n, x, incx, y, incy);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    scopy_(n, x, incx, y, incy);
  }
  else
  {
    int y_ptr = 0;
    for (int i = 0; i < *n; i += *incx)
    {
      y[y_ptr] = x[i];
      y_ptr += *incy;
    }
  }
}

template<typename P>
P dot(int *n, P *x, int *incx, P *y, int *incy, environment const environ)
{
  assert(n);
  assert(x);
  assert(incx);
  assert(y);
  assert(incy);
  assert(*incx >= 0);
  assert(*incy >= 0);
  assert(*n >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    return ddot_(n, x, incx, y, incy);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    return sdot_(n, x, incx, y, incy);
  }
  else
  {
    P ans     = 0.0;
    int y_ptr = 0;
    for (int i = 0; i < *n; i += *incx)
    {
      ans += x[i] * y[y_ptr];
      y_ptr += *incy;
    }
    return ans;
  }
}

template<typename P>
void axpy(int *n, P *alpha, P *x, int *incx, P *y, int *incy,
          environment const environ)
{
  assert(n);
  assert(alpha);
  assert(x);
  assert(incx);
  assert(y);
  assert(incy);
  assert(*incx >= 0);
  assert(*incy >= 0);
  assert(*n >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    daxpy_(n, alpha, x, incx, y, incy);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    saxpy_(n, alpha, x, incx, y, incy);
  }
  else
  {
    int x_ptr = 0;
    for (int i = 0; i < *n; i += *incy)
    {
      y[i] = y[i] + x[i] * (*alpha);
    }
  }
}

template<typename P>
void scal(int *n, P *alpha, P *x, int *incx, environment const environ)
{
  assert(n);
  assert(alpha);
  assert(x);
  assert(incx);
  assert(*n >= 0);
  assert(*incx >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    dscal_(n, alpha, x, incx);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    sscal_(n, alpha, x, incx);
  }
  else
  {
    for (int i = 0; i < *n; i += *incx)
    {
      x[i] *= *alpha;
    }
  }
}

//
// Simple helper for non-float types
//
template<typename P>
static void
basic_gemm(P *A, bool trans_A, int const lda, P *B, bool trans_B, int const ldb,
           P *C, int const ldc, int const m, int const k, int const n)
{
  assert(m > 0);
  assert(k > 0);
  assert(n > 0);
  assert(lda > 0); // FIXME Tyler says these could be more thorough
  assert(ldb > 0);
  assert(ldc > 0);

  int const nrows_A = trans_A ? k : m;
  int const ncols_A = trans_A ? m : k;

  int const nrows_B = trans_B ? n : k;
  int const ncols_B = trans_B ? k : n;

  for (auto i = 0; i < m; ++i)
  {
    for (auto j = 0; j < n; ++j)
    {
      P result = 0.0;
      for (auto z = 0; z < k; ++z)
      {
        // result += A[i,k] * B[k,j]
        int const A_loc = trans_A ? i * lda + z : z * lda + i;
        int const B_loc = trans_B ? z * ldb + j : j * ldb + z;
        result += A[A_loc] * B[B_loc];
      }
      // C[i,j] += result
      C[j * ldc + i] += result;
    }
  }
}

template<typename P>
void gemv(char const *trans, int *m, int *n, P *alpha, P *A, int *lda, P *x,
          int *incx, P *beta, P *y, int *incy, environment const environ)
{
  assert(trans);
  assert(m);
  assert(n);
  assert(alpha);
  assert(A);
  assert(lda);
  assert(x);
  assert(incx);
  assert(beta);
  assert(y);
  assert(incy);
  assert(*m >= 0);
  assert(*n >= 0);
  assert(*lda >= 0);
  assert(*incx >= 0);
  assert(*incy >= 0);
  assert(*trans == 't' || *trans == 'n');

  if constexpr (std::is_same<P, double>::value)
  {
    dgemv_(trans, m, n, alpha, A, lda, x, incx, beta, y, incy);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    sgemv_(trans, m, n, alpha, A, lda, x, incx, beta, y, incy);
  }
  else
  {
    bool const trans_A = (*trans == 't') ? true : false;
    bool const trans_x = false;
    int const one      = 1;
    basic_gemm(A, trans_A, *lda, x, trans_x, *n, y, *n, *m, *n, one);
  }
}

template<typename P>
void gemm(char const *transa, char const *transb, int *m, int *n, int *k,
          P *alpha, P *A, int *lda, P *B, int *ldb, P *beta, P *C, int *ldc,
          environment const environ)
{
  assert(transa);
  assert(transb);
  assert(m);
  assert(n);
  assert(k);
  assert(alpha);
  assert(A);
  assert(lda);
  assert(B);
  assert(ldb);
  assert(beta);
  assert(C);
  assert(ldc);
  assert(*m >= 0);
  assert(*n >= 0);
  assert(*k >= 0);
  assert(*transa == 't' || *transa == 'n');
  assert(*transb == 't' || *transb == 'n');

  if constexpr (std::is_same<P, double>::value)
  {
    dgemm_(transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    sgemm_(transa, transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
  }
  else
  {
    bool const trans_A = (*transa == 't') ? true : false;
    bool const trans_B = (*transb == 't') ? true : false;
    basic_gemm(A, trans_A, *lda, B, trans_B, *ldb, C, *ldc, *m, *k, *n);
  }
}

template<typename P>
void getrf(int *m, int *n, P *A, int *lda, int *ipiv, int *info,
           environment const environ)
{
  assert(m);
  assert(n);
  assert(A);
  assert(lda);
  assert(ipiv);
  assert(info);
  assert(*lda >= 0);
  assert(*m >= 0);
  assert(*n >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    dgetrf_(m, n, A, lda, ipiv, info);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    sgetrf_(m, n, A, lda, ipiv, info);
  }
  else
  { // not instantiated; should never be reached
    std::cerr << "getrf not implemented for non-floating types" << std::endl;
    assert(false);
  }
}

template<typename P>
void getri(int *n, P *A, int *lda, int *ipiv, P *work, int *lwork, int *info,
           environment const environ)
{
  assert(n);
  assert(A);
  assert(lda);
  assert(ipiv);
  assert(work);
  assert(lwork);
  assert(info);
  assert(*lda >= 0);
  assert(*n >= 0);

  if constexpr (std::is_same<P, double>::value)
  {
    dgetri_(n, A, lda, ipiv, work, lwork, info);
  }
  else if constexpr (std::is_same<P, float>::value)
  {
    sgetri_(n, A, lda, ipiv, work, lwork, info);
  }
  else
  { // not instantiated; should never be reached
    std::cerr << "getri not implemented for non-floating types" << std::endl;
    assert(false);
  }
}

template void copy(int *n, float *x, int *incx, float *y, int *incy,
                   environment const environ);
template void copy(int *n, double *x, int *incx, double *y, int *incy,
                   environment const environ);
template void
copy(int *n, int *x, int *incx, int *y, int *incy, environment const environ);

template float dot(int *n, float *x, int *incx, float *y, int *incy,
                   environment const environ);
template double dot(int *n, double *x, int *incx, double *y, int *incy,
                    environment const environ);
template int
dot(int *n, int *x, int *incx, int *y, int *incy, environment const environ);

template void axpy(int *n, float *alpha, float *x, int *incx, float *y,
                   int *incy, environment const environ);
template void axpy(int *n, double *alpha, double *x, int *incx, double *y,
                   int *incy, environment const environ);
template void axpy(int *n, int *alpha, int *x, int *incx, int *y, int *incy,
                   environment const environ);

template void
scal(int *n, float *alpha, float *x, int *incx, environment const environ);
template void
scal(int *n, double *alpha, double *x, int *incx, environment const environ);
template void
scal(int *n, int *alpha, int *x, int *incx, environment const environ);

template void gemv(char const *trans, int *m, int *n, float *alpha, float *A,
                   int *lda, float *x, int *incx, float *beta, float *y,
                   int *incy, environment const environ);
template void gemv(char const *trans, int *m, int *n, double *alpha, double *A,
                   int *lda, double *x, int *incx, double *beta, double *y,
                   int *incy, environment const environ);
template void gemv(char const *trans, int *m, int *n, int *alpha, int *A,
                   int *lda, int *x, int *incx, int *beta, int *y, int *incy,
                   environment const environ);

template void gemm(char const *transa, char const *transb, int *m, int *n,
                   int *k, float *alpha, float *A, int *lda, float *B, int *ldb,
                   float *beta, float *C, int *ldc, environment const environ);
template void gemm(char const *transa, char const *transb, int *m, int *n,
                   int *k, double *alpha, double *A, int *lda, double *B,
                   int *ldb, double *beta, double *C, int *ldc,
                   environment const environ);
template void gemm(char const *transa, char const *transb, int *m, int *n,
                   int *k, int *alpha, int *A, int *lda, int *B, int *ldb,
                   int *beta, int *C, int *ldc, environment const environ);

template void getrf(int *m, int *n, float *A, int *lda, int *ipiv, int *info,
                    environment const environ);
template void getrf(int *m, int *n, double *A, int *lda, int *ipiv, int *info,
                    environment const environ);

template void getri(int *n, float *A, int *lda, int *ipiv, float *work,
                    int *lwork, int *info, environment const environ);
template void getri(int *n, double *A, int *lda, int *ipiv, double *work,
                    int *lwork, int *info, environment const environ);

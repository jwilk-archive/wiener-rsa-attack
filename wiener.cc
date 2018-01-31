// Copyright © 2006-2018 Jakub Wilk <jwilk@jwilk.net>
// SPDX-License-Identifier: MIT
//
// Implementacja ataku Wienera na RSA, na podstawie:
// M.J. Wiener, “Cryptanalysis of Short RSA Secret Exponents”
// <http://www3.sympatico.ca/wienerfamily/Michael/MichaelPapers/ShortSecretExponents.pdf>
//  
// Dane wejściowe:
// 1. Liczba n = pq; p > q; p, q pierwsze.
// 2. Liczba e względnie pierwsza z L = NWW(p - 1, q - 1). 
//    (Można też przyjąć L = (p - 1)(q - 1))
// Wynik:
// 1. Liczba d ≡ e^-1 (mod L)
// 2. Liczby p, q.
//
// Przy założeniu, że L = (p-1)(q-1), q < p < 2q, atak udaje się jeżeli 
// d < (n^0.25)/3, e < n, ed > n.
//
// Główna idea:
//   Niech
//     G = (p-1)(q-1) / L,
//     K = (de-1) / L.
//   Niech
//     g = G / NWD(G, K),
//     k = K / NWD(G, K).
//   Wtedy
//     edg = k(p - 1)(q -1) + 1,
//     e / n = (k / dg) * (1 - t),
//   gdzie
//     t = 1/p + 1/q + 1/pq.
//   Ponieważ t jest małe, dobrym przybliżeniem k / dg jest e / n.

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <gmpxx.h>

using namespace std;

typedef mpz_class integer;

inline bool even(const integer &n)
// n mod 2 == 0?
{
  return mpz_divisible_2exp_p(n.get_mpz_t(), 1);
}

inline void divmod(integer &q, integer &r, const integer &n, const integer &d)
// q := n div d
// r := n mod d
{
  mpz_tdiv_qr(q.get_mpz_t(), r.get_mpz_t(), n.get_mpz_t(), d.get_mpz_t());
}

inline void usage()
{
  cerr << 
    "weiner [-v] [-s] <n> <e>" << endl << 
    "  <n>  modulus" << endl <<
    "  <e>  private exponent" << endl << endl;
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  unsigned int verbose = 0;
  bool sharpened = false;
  argc--; argv++;
  while (argc > 0 && **argv == '-')
  {
    char *arg;
    for (arg = *argv + 1; *arg >= 'a' && *arg <= 'z'; arg++)
      switch (*arg)
      {
        case 'v': 
          verbose++;
          break;
        case 's':
          sharpened = true;
          break;
      }
    argc--;
    argv++;
  }
  if (argc < 2)
    usage();
  integer n = integer(argv[0]);
  integer e = integer(argv[1]);
  cout << "n = " << n << endl;
  cout << "e = " << e << endl;
  if (n < 9 || e <= 0)
  {
    cerr << "Invalid parameters" << endl;
    return EXIT_FAILURE;
  }
  integer m = sharpened ? (n - sqrt(4*n) + 1) : n; 
  // m: dobre przybliżenie (p-1)(q-1) = pq - p - q + 1 < m:
  // · pq, lub
  // · floor(pq - 2sqrt(pq) + 1)
  assert(m >= 4);
  
  integer 
    Q, Q_prev,
    // e/m = [..., Q', Q, ...]
    R_num, R_den, 
    R_num_prev, R_den_prev,
    // R = e/m - [..., Q', Q]
    F_num, F_den, 
    F_num_prev, F_den_prev, 
    F_num_pprev, F_den_pprev;
    // F ≈ e/m;
    // F = [..., Q', Q]     lub
    //   = [..., Q', Q + 1]
  for (unsigned int i = 0; ; i++)
  {
    Q_prev = Q;
    F_num_pprev = F_num_prev;
    F_den_pprev = F_den_prev;
    F_num_prev = F_num;
    F_den_prev = F_den;
    R_num_prev = R_num;
    R_den_prev = R_den;
    if (verbose)
      cout << ">>> Step #" << i << endl;
    integer k, dg;
    if (i == 0)
    {
      Q = e / m;
      R_num = e % m;
      R_den = m;
      k = Q;
      dg = 1;
    }
    else 
    {
      if (R_num_prev == 0)
        break;
      divmod(Q, R_num, R_den_prev, R_num_prev);
      R_den = R_num_prev;
      if (i == 1)
      {
        k = Q_prev * Q + 1;
        dg = Q;
      }
      else
      {
        k = Q * F_num_prev + F_num_pprev;
        dg = Q * F_den_prev + F_den_pprev;
      }
    }
    F_num = k;
    F_den = dg;
    if (verbose)
    {
      cout << "Q = " << Q << endl;
      cout << "R = " << R_num << " / " << R_den << endl;
      cout << "F = " << F_num << " / " << F_den << endl;
    }
    else
      cout << Q << " " << flush;
    if (i == 0)
      k = dg = 1;
    else if (i % 2 == 0)
    {
      k += F_num_prev;
      dg += F_den_prev;
    }
    if (verbose)
      cout << "k / dg = " << k << " / " << dg << endl;
    integer phi_n, g;
    if (k == 0)
      break;
    // edg = k(p-1)(q-1) + g, więc:
    //   (p-1)(q-1) = edg div k,
    //   g = edg mod k,
    // o ile tylko k > g (na co wystarczy, żeby ed > n):
    divmod(phi_n, g, e * dg, k);
    if (verbose)
    {
      cout << "phi(n) = " << phi_n << endl;
      cout << "g = " << g << endl;
    }
    if (g == 0)
    {
      if (verbose > 1)
        cout << ">> Failure: g should be a positive integer" << endl;
      continue;
    }
    // p+q = pq - (p-1)(q-1) + 1:
    integer p_plus_q = n - phi_n + 1;
    if (p_plus_q < 0)
      break;
    if (verbose)
      cout << "p + q = " << p_plus_q << endl;
    if (!even(p_plus_q))
    {
      if (verbose > 1)
        cout << ">>> Failure: (p + q)/2 should be a positive integer" << endl;
      continue;
    }
    integer half_p_plus_q = p_plus_q >> 1;
    // ((p-q)/2)^2 = ((p+q)/2)^2 - pq:
    integer sqr_half_p_minus_q = half_p_plus_q * half_p_plus_q - n;
    if (sqr_half_p_minus_q < 0)
      break;
    if (verbose)
      cout << "((p - q)/2)^2 = " << sqr_half_p_minus_q << endl;
    integer half_p_minus_q = sqrt(sqr_half_p_minus_q);
    if (half_p_minus_q * half_p_minus_q != sqr_half_p_minus_q)
    {
      if (verbose > 1)
        cout << ">>> Failure: (p - q)/2 should be a positive integer" << endl;
      continue;
    }
    if (verbose)
      cout << ">>> Secret key has been found!";
    cout << endl;
    cout << "d = " << dg / g << endl;
    if (verbose)
    {
      cout << "p = " << half_p_plus_q + half_p_minus_q << endl;
      cout << "q = " << half_p_plus_q - half_p_minus_q << endl;
    }
    cout << endl;
    assert(half_p_plus_q * half_p_plus_q - half_p_minus_q * half_p_minus_q == n);
    return EXIT_SUCCESS;
  }
  cout << endl << ">>> The secret key could not be found" << endl << endl;
  return EXIT_FAILURE;
}

// vim:ts=2 sts=2 sw=2 et

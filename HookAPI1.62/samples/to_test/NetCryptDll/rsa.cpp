/* RSA.C - RSA routines for RSAREF
 */
#include "stdafx.h"
#include <memory.h>
#include "rsaref.h"
#include "big_num.h"

static int RSAPublicBlock PROTO_LIST 
  ((unsigned char *, unsigned int *, unsigned char *, unsigned int,
    R_RSA_PUBLIC_KEY *));
static int RSAPrivateBlock PROTO_LIST 
  ((unsigned char *, unsigned int *, unsigned char *, unsigned int,
    R_RSA_PRIVATE_KEY *));

/* RSA public-key encryption, according to PKCS #1.
 */
int RSAPublicEncrypt
  (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PUBLIC_KEY *publicKey, R_RANDOM_STRUCT *randomStruct)
{
  int status;
  unsigned char byte, pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen;
  
  modulusLen = (publicKey->bits + 7) / 8;
  if (inputLen + 11 > modulusLen)
    return (RE_LEN);
  
  pkcsBlock[0] = 0;
  pkcsBlock[1] = 2;/* block type 2 */

  for (i = 2; i < modulusLen - inputLen - 1; i++) {
    /* Find nonzero random byte.
     */
    do {
      R_GenerateBytes (&byte, 1, randomStruct);
    } while (byte == 0);
    pkcsBlock[i] = byte;
  }
  /* separator */
  pkcsBlock[i++] = 0;
  
  R_memcpy ((POINTER)&pkcsBlock[i], (POINTER)input, inputLen);

  /* encrypt */
  status = RSAPublicBlock
	  (output, outputLen, pkcsBlock, modulusLen, publicKey);
  
  /* Zeroize sensitive information.
   */
  byte = 0;
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));
  
  return (status);
}

/* RSA public-key decryption, according to PKCS #1.
 */
int RSAPublicDecrypt (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PUBLIC_KEY *publicKey)
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen, pkcsBlockLen;
  
  modulusLen = (publicKey->bits + 7) / 8;
  if (inputLen > modulusLen)
    return (RE_LEN);
  
  if (status = RSAPublicBlock
      (pkcsBlock, &pkcsBlockLen, input, inputLen, publicKey))
    return (status);
  
  if (pkcsBlockLen != modulusLen)
    return (RE_LEN);
  
  /* Require block type 1.
   */
  if ((pkcsBlock[0] != 0) || (pkcsBlock[1] != 1))
   return (RE_DATA);

  for (i = 2; i < modulusLen-1; i++)
    if (pkcsBlock[i] != 0xff)
      break;
    
  /* separator */
  if (pkcsBlock[i++] != 0)
    return (RE_DATA);
  
  *outputLen = modulusLen - i;
  
  if (*outputLen + 11 > modulusLen)
    return (RE_DATA);

  R_memcpy ((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);
  
  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));
  
  return (0);
}

/* RSA private-key encryption, according to PKCS #1.
 */
int RSAPrivateEncrypt (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PRIVATE_KEY *privateKey)
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen;

// this code is very dangerous, be carefully , add by david
//********************************************
//  if(privateKey->bits==0)
//	  privateKey->bits = 1024;
//********************************************

  modulusLen = (privateKey->bits + 7) / 8;
  if (inputLen + 11 > modulusLen)
    return (RE_LEN);
  
  pkcsBlock[0] = 0;
  /* block type 1 */
  pkcsBlock[1] = 1;

  for (i = 2; i < modulusLen - inputLen - 1; i++)
    pkcsBlock[i] = 0xff;

  /* separator */
  pkcsBlock[i++] = 0;
  
  R_memcpy ((POINTER)&pkcsBlock[i], (POINTER)input, inputLen);
  
  status = RSAPrivateBlock
    (output, outputLen, pkcsBlock, modulusLen, privateKey);

  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));

  return (status);
}

/* RSA private-key decryption, according to PKCS #1.
 */
int RSAPrivateDecrypt (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PRIVATE_KEY *privateKey)
{
  int status;
  unsigned char pkcsBlock[MAX_RSA_MODULUS_LEN];
  unsigned int i, modulusLen, pkcsBlockLen;
  

// this code is very dangerous, be carefully , add by david
//********************************************
//  if(privateKey->bits==0)
//	  privateKey->bits = 1024;
//********************************************

  modulusLen = (privateKey->bits + 7) / 8;
  if (inputLen > modulusLen)
    return (RE_LEN);
  
  if (status = RSAPrivateBlock
      (pkcsBlock, &pkcsBlockLen, input, inputLen, privateKey))
    return (status);
  
  if (pkcsBlockLen != modulusLen)
    return (RE_LEN);
  
  /* Require block type 2.
   */
//  if ((pkcsBlock[0] != 0) || (pkcsBlock[1] != 2))
//   return (RE_DATA);

  for (i = 2; i < modulusLen-1; i++)
    /* separator */
    if (pkcsBlock[i] == 0)
      break;
    
  i++;
  if (i >= modulusLen)
    return (RE_DATA);
    
  *outputLen = modulusLen - i;
  
  if (*outputLen + 11 > modulusLen)
    return (RE_DATA);

  R_memcpy ((POINTER)output, (POINTER)&pkcsBlock[i], *outputLen);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)pkcsBlock, 0, sizeof (pkcsBlock));
  
  return (0);
}

/* Raw RSA public-key operation. Output has same length as modulus.

   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
static int RSAPublicBlock (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PUBLIC_KEY *publicKey)
{
  NN_DIGIT c[MAX_NN_DIGITS], e[MAX_NN_DIGITS], m[MAX_NN_DIGITS],
    n[MAX_NN_DIGITS];
  unsigned int eDigits, nDigits;

  NN_Decode (m, MAX_NN_DIGITS, input, inputLen);
  NN_Decode (n, MAX_NN_DIGITS, publicKey->modulus, MAX_RSA_MODULUS_LEN);
  NN_Decode (e, MAX_NN_DIGITS, publicKey->exponent, MAX_RSA_MODULUS_LEN);
  nDigits = NN_Digits (n, MAX_NN_DIGITS);
  eDigits = NN_Digits (e, MAX_NN_DIGITS);
  
  if (NN_Cmp (m, n, nDigits) >= 0)
    return (RE_DATA);
  
  /* Compute c = m^e mod n.
   */
  NN_ModExp (c, m, e, eDigits, n, nDigits);

  *outputLen = (publicKey->bits + 7) / 8;
  NN_Encode (output, *outputLen, c, nDigits);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)c, 0, sizeof (c));
  R_memset ((POINTER)m, 0, sizeof (m));

  return (0);
}

/* Raw RSA private-key operation. Output has same length as modulus.

   Assumes inputLen < length of modulus.
   Requires input < modulus.
 */
static int RSAPrivateBlock (unsigned char *output, unsigned int *outputLen, unsigned char *input, unsigned int inputLen, R_RSA_PRIVATE_KEY *privateKey)
{
  NN_DIGIT c[MAX_NN_DIGITS], cP[MAX_NN_DIGITS], cQ[MAX_NN_DIGITS],
    dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS], mP[MAX_NN_DIGITS],
    mQ[MAX_NN_DIGITS], n[MAX_NN_DIGITS], p[MAX_NN_DIGITS], q[MAX_NN_DIGITS],
    qInv[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  unsigned int cDigits, nDigits, pDigits;
  
  NN_Decode (c, MAX_NN_DIGITS, input, inputLen);
  NN_Decode (n, MAX_NN_DIGITS, privateKey->modulus, MAX_RSA_MODULUS_LEN);
  NN_Decode (p, MAX_NN_DIGITS, privateKey->prime[0], MAX_RSA_PRIME_LEN);
  NN_Decode (q, MAX_NN_DIGITS, privateKey->prime[1], MAX_RSA_PRIME_LEN);
  NN_Decode 
    (dP, MAX_NN_DIGITS, privateKey->primeExponent[0], MAX_RSA_PRIME_LEN);
  NN_Decode 
    (dQ, MAX_NN_DIGITS, privateKey->primeExponent[1], MAX_RSA_PRIME_LEN);
  NN_Decode (qInv, MAX_NN_DIGITS, privateKey->coefficient, MAX_RSA_PRIME_LEN);
  cDigits = NN_Digits (c, MAX_NN_DIGITS);
  nDigits = NN_Digits (n, MAX_NN_DIGITS);
  pDigits = NN_Digits (p, MAX_NN_DIGITS);

  if (NN_Cmp (c, n, nDigits) >= 0)
    return (RE_DATA);
  
  /* Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. (Assumes q has
     length at most pDigits, i.e., p > q.)
   */
  NN_Mod (cP, c, cDigits, p, pDigits);
  NN_Mod (cQ, c, cDigits, q, pDigits);
  NN_ModExp (mP, cP, dP, pDigits, p, pDigits);
  NN_AssignZero (mQ, nDigits);
  NN_ModExp (mQ, cQ, dQ, pDigits, q, pDigits);
  
  /* Chinese Remainder Theorem:
       m = ((((mP - mQ) mod p) * qInv) mod p) * q + mQ.
   */
  if (NN_Cmp (mP, mQ, pDigits) >= 0)
    NN_Sub (t, mP, mQ, pDigits);
  else {
    NN_Sub (t, mQ, mP, pDigits);
    NN_Sub (t, p, t, pDigits);
  }
  NN_ModMult (t, t, qInv, p, pDigits);
  NN_Mult (t, t, q, pDigits);
  NN_Add (t, t, mQ, nDigits);

  *outputLen = (privateKey->bits + 7) / 8;
  NN_Encode (output, *outputLen, t, nDigits);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)c, 0, sizeof (c));
  R_memset ((POINTER)cP, 0, sizeof (cP));
  R_memset ((POINTER)cQ, 0, sizeof (cQ));
  R_memset ((POINTER)dP, 0, sizeof (dP));
  R_memset ((POINTER)dQ, 0, sizeof (dQ));
  R_memset ((POINTER)mP, 0, sizeof (mP));
  R_memset ((POINTER)mQ, 0, sizeof (mQ));
  R_memset ((POINTER)p, 0, sizeof (p));
  R_memset ((POINTER)q, 0, sizeof (q));
  R_memset ((POINTER)qInv, 0, sizeof (qInv));
  R_memset ((POINTER)t, 0, sizeof (t));

  return (0);
}

/*
 * key generation functions
 */

static int RSAFilter PROTO_LIST
  ((NN_DIGIT *, unsigned int, NN_DIGIT *, unsigned int));
static int RelativelyPrime PROTO_LIST
  ((NN_DIGIT *, unsigned int, NN_DIGIT *, unsigned int));

/* Generates an RSA key pair with a given length and public exponent.
 */
int R_GeneratePEMKeys (R_RSA_PUBLIC_KEY *publicKey, R_RSA_PRIVATE_KEY *privateKey, R_RSA_PROTO_KEY *protoKey, R_RANDOM_STRUCT *randomStruct)
{
  NN_DIGIT d[MAX_NN_DIGITS], dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS],
    e[MAX_NN_DIGITS], n[MAX_NN_DIGITS], p[MAX_NN_DIGITS], phiN[MAX_NN_DIGITS],
    pMinus1[MAX_NN_DIGITS], q[MAX_NN_DIGITS], qInv[MAX_NN_DIGITS],
    qMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS], u[MAX_NN_DIGITS],
    v[MAX_NN_DIGITS];
  int status;
  unsigned int nDigits, pBits, pDigits, qBits;
  
  if ((protoKey->bits < MIN_RSA_MODULUS_BITS) || 
      (protoKey->bits > MAX_RSA_MODULUS_BITS))
    return (RE_MODULUS_LEN);
  nDigits = (protoKey->bits + NN_DIGIT_BITS - 1) / NN_DIGIT_BITS;
  pDigits = (nDigits + 1) / 2;
  pBits = (protoKey->bits + 1) / 2;
  qBits = protoKey->bits - pBits;

  /* NOTE: for 65537, this assumes NN_DIGIT is at least 17 bits. */
  NN_ASSIGN_DIGIT
    (e, protoKey->useFermat4 ? (NN_DIGIT)65537 : (NN_DIGIT)3, nDigits);

  /* Generate prime p between 3*2^(pBits-2) and 2^pBits-1, searching
       in steps of 2, until one satisfies gcd (p-1, e) = 1.
   */
  NN_Assign2Exp (t, pBits-1, pDigits);
  NN_Assign2Exp (u, pBits-2, pDigits);
  NN_Add (t, t, u, pDigits);
  NN_ASSIGN_DIGIT (v, 1, pDigits);
  NN_Sub (v, t, v, pDigits);
  NN_Add (u, u, v, pDigits);
  NN_ASSIGN_DIGIT (v, 2, pDigits);
  do {
    if (status = GeneratePrime (p, t, u, v, pDigits, randomStruct))
      return (status);
  }
  while (! RSAFilter (p, pDigits, e, 1));
  
  /* Generate prime q between 3*2^(qBits-2) and 2^qBits-1, searching
       in steps of 2, until one satisfies gcd (q-1, e) = 1.
   */
  NN_Assign2Exp (t, qBits-1, pDigits);
  NN_Assign2Exp (u, qBits-2, pDigits);
  NN_Add (t, t, u, pDigits);
  NN_ASSIGN_DIGIT (v, 1, pDigits);
  NN_Sub (v, t, v, pDigits);
  NN_Add (u, u, v, pDigits);
  NN_ASSIGN_DIGIT (v, 2, pDigits);
  do {
    if (status = GeneratePrime (q, t, u, v, pDigits, randomStruct))
      return (status);
  }
  while (! RSAFilter (q, pDigits, e, 1));
  
  /* Sort so that p > q. (p = q case is extremely unlikely.)
   */
  if (NN_Cmp (p, q, pDigits) < 0) {
    NN_Assign (t, p, pDigits);
    NN_Assign (p, q, pDigits);
    NN_Assign (q, t, pDigits);
  }

  /* Compute n = pq, qInv = q^{-1} mod p, d = e^{-1} mod (p-1)(q-1),
     dP = d mod p-1, dQ = d mod q-1.
   */
  NN_Mult (n, p, q, pDigits);
  NN_ModInv (qInv, q, p, pDigits);
  
  NN_ASSIGN_DIGIT (t, 1, pDigits);
  NN_Sub (pMinus1, p, t, pDigits);
  NN_Sub (qMinus1, q, t, pDigits);
  NN_Mult (phiN, pMinus1, qMinus1, pDigits);

  NN_ModInv (d, e, phiN, nDigits);
  NN_Mod (dP, d, nDigits, pMinus1, pDigits);
  NN_Mod (dQ, d, nDigits, qMinus1, pDigits);
  
  publicKey->bits = privateKey->bits = protoKey->bits;
  NN_Encode (publicKey->modulus, MAX_RSA_MODULUS_LEN, n, nDigits);
  NN_Encode (publicKey->exponent, MAX_RSA_MODULUS_LEN, e, 1);
  R_memcpy 
    ((POINTER)privateKey->modulus, (POINTER)publicKey->modulus,
     MAX_RSA_MODULUS_LEN);
  R_memcpy
    ((POINTER)privateKey->publicExponent, (POINTER)publicKey->exponent,
     MAX_RSA_MODULUS_LEN);
  NN_Encode (privateKey->exponent, MAX_RSA_MODULUS_LEN, d, nDigits);
  NN_Encode (privateKey->prime[0], MAX_RSA_PRIME_LEN, p, pDigits);
  NN_Encode (privateKey->prime[1], MAX_RSA_PRIME_LEN, q, pDigits);
  NN_Encode (privateKey->primeExponent[0], MAX_RSA_PRIME_LEN, dP, pDigits);
  NN_Encode (privateKey->primeExponent[1], MAX_RSA_PRIME_LEN, dQ, pDigits);
  NN_Encode (privateKey->coefficient, MAX_RSA_PRIME_LEN, qInv, pDigits);
   
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)d, 0, sizeof (d));
  R_memset ((POINTER)dP, 0, sizeof (dP));
  R_memset ((POINTER)dQ, 0, sizeof (dQ));
  R_memset ((POINTER)p, 0, sizeof (p));
  R_memset ((POINTER)phiN, 0, sizeof (phiN));
  R_memset ((POINTER)pMinus1, 0, sizeof (pMinus1));
  R_memset ((POINTER)q, 0, sizeof (q));
  R_memset ((POINTER)qInv, 0, sizeof (qInv));
  R_memset ((POINTER)qMinus1, 0, sizeof (qMinus1));
  R_memset ((POINTER)t, 0, sizeof (t));
  
  return (0);
}

/* Returns nonzero iff GCD (a-1, b) = 1.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits < MAX_NN_DIGITS, bDigits < MAX_NN_DIGITS.
 */
static int RSAFilter (NN_DIGIT *a, unsigned int aDigits, NN_DIGIT *b, unsigned int bDigits)
{
  int status;
  NN_DIGIT aMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  
  NN_ASSIGN_DIGIT (t, 1, aDigits);
  NN_Sub (aMinus1, a, t, aDigits);
  
  status = RelativelyPrime (aMinus1, aDigits, b, bDigits);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)aMinus1, 0, sizeof (aMinus1));
  
  return (status);
}

/* Returns nonzero iff a and b are relatively prime.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits >= bDigits, aDigits < MAX_NN_DIGITS.
 */
static int RelativelyPrime (NN_DIGIT *a, unsigned int aDigits, NN_DIGIT *b, unsigned int bDigits)
{
  int status;
  NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS];
  
  NN_AssignZero (t, aDigits);
  NN_Assign (t, b, bDigits);
  NN_Gcd (t, a, t, aDigits);
  NN_ASSIGN_DIGIT (u, 1, aDigits);

  status = NN_EQUAL (t, u, aDigits);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)t, 0, sizeof (t));
  
  return (status);
}

/* Generates Diffie-Hellman parameters.
 */
int R_GenerateDHParams (R_DH_PARAMS *params, unsigned int primeBits, unsigned int subPrimeBits, R_RANDOM_STRUCT *randomStruct)
{
  int status;
  NN_DIGIT g[MAX_NN_DIGITS], p[MAX_NN_DIGITS], q[MAX_NN_DIGITS],
    t[MAX_NN_DIGITS], u[MAX_NN_DIGITS], v[MAX_NN_DIGITS];
  unsigned int pDigits;

  pDigits = (primeBits + NN_DIGIT_BITS - 1) / NN_DIGIT_BITS;
  
  /* Generate subprime q between 2^(subPrimeBits-1) and
       2^subPrimeBits-1, searching in steps of 2.
   */
  NN_Assign2Exp (t, subPrimeBits-1, pDigits);
  NN_Assign (u, t, pDigits);
  NN_ASSIGN_DIGIT (v, 1, pDigits);
  NN_Sub (v, t, v, pDigits);
  NN_Add (u, u, v, pDigits);
  NN_ASSIGN_DIGIT (v, 2, pDigits);
  if (status = GeneratePrime (q, t, u, v, pDigits, randomStruct))
    return (status);
  
  /* Generate prime p between 2^(primeBits-1) and 2^primeBits-1,
       searching in steps of 2*q.
   */
  NN_Assign2Exp (t, primeBits-1, pDigits);
  NN_Assign (u, t, pDigits);
  NN_ASSIGN_DIGIT (v, 1, pDigits);
  NN_Sub (v, t, v, pDigits);
  NN_Add (u, u, v, pDigits);
  NN_LShift (v, q, 1, pDigits);
  if (status = GeneratePrime (p, t, u, v, pDigits, randomStruct))
    return (status);
  
  /* Generate generator g for subgroup as 2^((p-1)/q) mod p.
   */
  NN_ASSIGN_DIGIT (g, 2, pDigits);
  NN_Div (t, u, p, pDigits, q, pDigits);
  NN_ModExp (g, g, t, pDigits, p, pDigits);

  params->generatorLen = params->primeLen = DH_PRIME_LEN (primeBits);
  NN_Encode (params->prime, params->primeLen, p, pDigits);
  NN_Encode (params->generator, params->generatorLen, g, pDigits);

  return (0);
}

/* Sets up Diffie-Hellman key agreement. Public value has same length
   as prime.
 */
int R_SetupDHAgreement
  (unsigned char *publicValue, unsigned char *privateValue, unsigned int privateValueLen, R_DH_PARAMS *params, R_RANDOM_STRUCT *randomStruct)
{
  int status;
  NN_DIGIT g[MAX_NN_DIGITS], p[MAX_NN_DIGITS], x[MAX_NN_DIGITS],
    y[MAX_NN_DIGITS];
  unsigned int pDigits, xDigits;

  NN_Decode (p, MAX_NN_DIGITS, params->prime, params->primeLen);
  pDigits = NN_Digits (p, MAX_NN_DIGITS);
  NN_Decode (g, pDigits, params->generator, params->generatorLen);

  /* Generate private value.
   */
  if (status = R_GenerateBytes (privateValue, privateValueLen, randomStruct))
    return (status);
  NN_Decode (x, pDigits, privateValue, privateValueLen);
  xDigits = NN_Digits (x, pDigits);
  
  /* Compute y = g^x mod p.
   */
  NN_ModExp (y, g, x, xDigits, p, pDigits);

  NN_Encode (publicValue, params->primeLen, y, pDigits);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)x, 0, sizeof (x));

  return (0);
}

/* Computes agreed key from the other party's public value, a private
   value, and Diffie-Hellman parameters. Other public value and
   agreed-upon key have same length as prime.

   Requires otherPublicValue < prime.
 */
int R_ComputeDHAgreedKey
  (unsigned char *agreedKey, unsigned char *otherPublicValue, unsigned char *privateValue, unsigned int privateValueLen, R_DH_PARAMS *params)
{
  NN_DIGIT p[MAX_NN_DIGITS], x[MAX_NN_DIGITS], y[MAX_NN_DIGITS],
    z[MAX_NN_DIGITS];
  unsigned int pDigits, xDigits;

  NN_Decode (p, MAX_NN_DIGITS, params->prime, params->primeLen);
  pDigits = NN_Digits (p, MAX_NN_DIGITS);
  NN_Decode (x, pDigits, privateValue, privateValueLen);
  xDigits = NN_Digits (x, pDigits);
  NN_Decode (y, pDigits, otherPublicValue, params->primeLen);

  if (NN_Cmp (y, p, pDigits) >= 0)
    return (RE_DATA);
  
  /* Compute z = y^x mod p.
   */
  NN_ModExp (z, y, x, xDigits, p, pDigits);

  NN_Encode (agreedKey, params->primeLen, z, pDigits);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)x, 0, sizeof (x));
  R_memset ((POINTER)z, 0, sizeof (z));

  return (0);
}

/*
 * memory funciton
 */

void R_memset (POINTER output, int value, unsigned int len)
{
  if (len)
    memset (output, value, len);
}

void R_memcpy (POINTER output, POINTER input, unsigned int len)
{
  if (len)
    memcpy (output, input, len);
}

int R_memcmp (POINTER firstBlock, POINTER secondBlock, unsigned int len)
{
  if (len)
    return (memcmp (firstBlock, secondBlock, len));
  else
    return (0);
}

/* ******************************************************************
*  Custom.c
*  Implements overrides for ESP32s3 in WolfSSL library
*  ****************************************************************** */

#include "esp_random.h"
#include "wolfssl/wolfcrypt/random.h"
#include "wolfssl/wolfcrypt/error-crypt.h"

// To use Espressif random number generator in place of the Wolf one
// See: https://www.wolfssl.com/documentation/manuals/wolfssl/chapter02.html#custom_rand_generate_block
// pOutput: buffer where random numbers are written
// szOutput: size of output (number of random bytes to write)
// returns: 
//   0 => success
//   BAD_FUNC_ARG => pOutput is null
//   RNG_FAILURE_E => random number generator isn't ready. 
int wolf_rng_gen(unsigned char* pOutput, unsigned int szOutput)
{
  if (NULL == pOutput)
    return BAD_FUNC_ARG;

  if (szOutput == 0)
    return 0;

  // Note, this is only a source of true random numbers when an
  // entropy source is available (e.g., WiFi radio). Since we should
  // only be handling certificates using the radio, that should be
  // okay. However, the radio might be turned on when generating 
  // a device key. Make sure it is. 
  esp_fill_random(pOutput, szOutput);
  
  return 0; 
}
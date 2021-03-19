#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <stm32l4xx.h>
#include <stm32l4s5xx.h>
#include <stm32l4xx_hal.h>

#include <stm32l4xx_hal_cryp.h>
#include <stm32l4xx_hal_cryp_ex.h>
#include <stm32l4xx_hal_rng.h>
#include <stm32l4xx_hal_rng_ex.h>

/* AES initial schedule */
static CRYP_InitTypeDef initSched = {
    CRYP_DATATYPE_32B,
    CRYP_KEYSIZE_256B,
    CRYP_ALGOMODE_ENCRYPT,
    CRYP_CHAINMODE_AES_GCM_GMAC,
    CRYP_KEY_WRITE_DISABLE,
    CRYP_GCM_INIT_PHASE,
};

static RNG_HandleTypeDef rngInit;

/* AES key schedule */

static CRYP_HandleTypeDef keySched;

static void cryptoPrepareEncrypt() 
{

}

static void cryptoInit()
{

}

static void cryptoChangeKey(char *key)
{

}

void main(void)
{
    HAL_RNG_Init(&rngInit);
    HAL_CRYP_Init(&keySched);
}

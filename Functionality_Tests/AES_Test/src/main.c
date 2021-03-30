#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/ctr_mode.h>
#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/sha256.h>

/**
 * @brief
 * @return
 * @note
 * @param key IN/OUT
 */
static int cryptoChangeKey(char *key) {
    TCSha256State_t hash = malloc(sizeof(struct tc_sha256_state_struct));
    if(hash == NULL) {
        return TC_CRYPTO_FAIL;
    }

    int err = tc_sha256_init(hash);
    if(err == TC_CRYPTO_FAIL) {
        free(hash);
        return TC_CRYPTO_FAIL;
    }

    err = tc_sha256_update(hash, key, strlen(key));
    if(err = TC_CRYPTO_FAIL) {
        free(hash);
        return TC_CRYPTO_FAIL;
    }

    err = tc_sha256_final(key, hash);
    free(hash);
    return err;
}

/**
 * @brief Encrypts a message using CTR mode, appending the 32-bit IV to the front of the ciphertext.
 * @return returns TC_CRYPTO_SUCCESS (1) on success, TC_CRYPTO_FAIL (0) on failure
 * @note Requires cryptoInit to have been called and requires a functional RNG.
 * @param plaintext IN -- input plaintext to encrypt
 * @param ciphertext OUT -- produced ciphertext. Must be at least (plaintext + 4) bytes
 * @param key IN -- encryption key
 * @param rng IN -- An initialized and functional RNG
 */

static int cryptoEncrypt(char* plaintext, char* ciphertext, char* key, TCCtrPrng_t rng) {
    TCAesKeySched_t encryptSched = malloc(sizeof(struct tc_aes_key_sched_struct));
    if(encryptSched == NULL) {
        return TC_CRYPTO_FAIL;
    }


    int err = tc_aes128_set_encrypt_key(encryptSched, key);
    if(err == TC_CRYPTO_FAIL) {
        free(encryptSched);
        return TC_CRYPTO_FAIL;
    }
    
    uint32_t nonce;

    err = tc_ctr_prng_generate(rng, NULL, 0, nonce, sizeof(int));
    if(err == TC_CRYPTO_FAIL) {
        free(encryptSched);
        return TC_CRYPTO_FAIL;
    }

    int size = strlen(plaintext);

    err = tc_ctr_mode((ciphertext+4), size, plaintext, size, nonce, encryptSched);
    free(encryptSched);
    return err;
}

/**
 * @brief Decrypts a message using CTR mode,, using the 32-bit IV at the front of the ciphertext.
 * @return returns TC_CRYPTO_SUCCESS (1) on success, TC_CRYPTO_FAIL (0) on failure
 * @param plaintext OUT -- produced plaintext
 * @param ciphertext OUT -- input ciphertext to decrypt
 * @param key IN -- encryption key
 */

static int cryptoDecrypt(char* ciphertext, char* plaintext, char* key) {
    TCAesKeySched_t decryptSched = malloc(sizeof(struct tc_aes_key_sched_struct));
    if(decryptSched == NULL) {
        return TC_CRYPTO_FAIL;
    }
    
    int err = tc_aes128_set_decrypt_key(decryptSched, key);

    if(err == TC_CRYPTO_FAIL) {
        free(decryptSched);
        return TC_CRYPTO_FAIL;
    }

    uint32_t nonce = *(int*)ciphertext;

    int size = strlen(ciphertext) - 4;

    err = tc_ctr_mode(plaintext, size, ciphertext+4, size, nonce, decryptSched);
    free(decryptSched);
    return err;
}


static TCCtrPrng_t cryptoInit(uint8_t *entropy, int entropyLen) {
    return rng;
}

void main(void) {
    char* message = "This is an example string."
    char* key = malloc(100);
    strcpy(key, "password1");
    char* ciphertext = malloc(100);
    char* wrongDecoded = malloc(100);
    char* decoded = malloc(100);

    TCCtrPrng_t rng;
    tc_ctr_prng_init(rng, entropy, entropyLen, NULL, 0)

    cryptoEncrypt(message, ciphertext, key, rng);
    cryptoDecrypt(ciphertext, wrongDecoded, "wrongpassword");
    cryptoDecrypt(ciphertext, decoded, key);
}

/* Target-OS NSS startup and known-answer checks, using bundled test vectors. */
#include "nss.h"
#include "pk11pub.h"
#include "keyhi.h"
#include "pkcs11n.h"
#include "prerror.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static unsigned int read_vector(const char *directory, const char *name,
                                unsigned char *bytes, unsigned int capacity)
{
    char path[1024];
    FILE *file;
    size_t count;
    if (strlen(directory) + strlen(name) + 2 > sizeof(path)) return 0;
    sprintf(path, "%s/%s", directory, name);
    file = fopen(path, "rb");
    if (!file) return 0;
    count = fread(bytes, 1, capacity, file);
    if (ferror(file) || fgetc(file) != EOF) count = 0;
    fclose(file);
    return (unsigned int)count;
}

static int chacha_vector(PK11SlotInfo *slot, const char *directory)
{
    unsigned char key[32], iv[12], aad[64], plain[512], expected[528];
    unsigned char cipher[528], decoded[512];
    unsigned int keylen, ivlen, aadlen, plainlen, expectedlen, length;
    CK_NSS_AEAD_PARAMS aead;
    SECItem key_item, params;
    PK11SymKey *sym;
    int ok;
    keylen = read_vector(directory, "key0", key, sizeof(key));
    ivlen = read_vector(directory, "iv0", iv, sizeof(iv));
    aadlen = read_vector(directory, "aad0", aad, sizeof(aad));
    plainlen = read_vector(directory, "plaintext0", plain, sizeof(plain));
    /* The payload builder decodes bltest's base64 ciphertext0. */
    expectedlen = read_vector(directory, "ciphertext.bin", expected, sizeof(expected));
    if (keylen != 32 || ivlen != 12 || !aadlen || !plainlen ||
        expectedlen != plainlen + 16) return 0;
    key_item.type = siBuffer; key_item.data = key; key_item.len = keylen;
    sym = PK11_ImportSymKey(slot, CKM_NSS_CHACHA20_POLY1305,
                           PK11_OriginUnwrap, CKA_ENCRYPT, &key_item, NULL);
    if (!sym) return 0;
    aead.pNonce = iv; aead.ulNonceLen = ivlen;
    aead.pAAD = aad; aead.ulAADLen = aadlen; aead.ulTagLen = 16;
    params.type = siBuffer; params.data = (unsigned char *)&aead;
    params.len = sizeof(aead);
    ok = PK11_Encrypt(sym, CKM_NSS_CHACHA20_POLY1305, &params, cipher, &length,
                      sizeof(cipher), plain, plainlen) == SECSuccess &&
         length == expectedlen && !memcmp(cipher, expected, length);
    if (ok)
        ok = PK11_Decrypt(sym, CKM_NSS_CHACHA20_POLY1305, &params, decoded,
                          &length, sizeof(decoded), cipher, expectedlen) == SECSuccess &&
             length == plainlen && !memcmp(decoded, plain, length);
    /* Authentication failures must be rejected, including on big-endian PPC. */
    if (ok) {
        cipher[0] ^= 1;
        ok = PK11_Decrypt(sym, CKM_NSS_CHACHA20_POLY1305, &params, decoded,
                          &length, sizeof(decoded), cipher, expectedlen) == SECFailure;
    }
    PK11_FreeSymKey(sym);
    return ok;
}

/* Exercise the P-256 implementation whose byte-order helpers are adapted
 * for the original SDK, including rejection of a corrupted signature. */
static int p256_signature(PK11SlotInfo *slot, unsigned char *digest)
{
    unsigned char oid[] = { 0x06,0x08,0x2a,0x86,0x48,0xce,0x3d,0x03,0x01,0x07 };
    unsigned char bytes[160];
    SECItem parameters = { siBuffer, oid, sizeof(oid) };
    SECItem hash = { siBuffer, digest, 32 };
    SECItem signature = { siBuffer, bytes, sizeof(bytes) };
    SECKEYPublicKey *public_key = NULL;
    SECKEYPrivateKey *private_key = PK11_GenerateKeyPair(
        slot, CKM_EC_KEY_PAIR_GEN, &parameters, &public_key, PR_FALSE, PR_FALSE, NULL);
    int ok = private_key && public_key;
    if (ok)
        ok = PK11_Sign(private_key, &signature, &hash) == SECSuccess &&
             signature.len == 64 &&
             PK11_Verify(public_key, &signature, &hash, NULL) == SECSuccess;
    if (ok) {
        signature.data[0] ^= 1;
        ok = PK11_Verify(public_key, &signature, &hash, NULL) == SECFailure;
    }
    if (private_key) SECKEY_DestroyPrivateKey(private_key);
    if (public_key) SECKEY_DestroyPublicKey(public_key);
    return ok;
}

int main(int argc, char **argv)
{
    static const unsigned char sha256[] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    unsigned char digest[32], random_bytes[32];
    PK11SlotInfo *slot;
    setbuf(stdout, NULL);
    if (argc != 2) return 1;
    puts("NSS: initialize original-OS runtime");
    if (NSS_NoDB_Init(NULL) != SECSuccess) {
        printf("NSS initialization error: %d\n", PR_GetError());
        return 2;
    }
    if (PK11_HashBuf(SEC_OID_SHA256, digest, (const unsigned char *)"abc", 3) != SECSuccess ||
        memcmp(digest, sha256, sizeof(digest))) return 3;
    puts("NSS: SHA-256 known answer passed");
    if (PK11_GenerateRandom(random_bytes, sizeof(random_bytes)) != SECSuccess) return 4;
    slot = PK11_GetInternalSlot();
    if (!slot || !chacha_vector(slot, argv[1])) {
        printf("NSS ChaCha20-Poly1305 error: %d\n", PR_GetError());
        return 5;
    }
    if (!p256_signature(slot, digest)) {
        printf("NSS P-256 error: %d\n", PR_GetError());
        PK11_FreeSlot(slot);
        return 9;
    }
    puts("NSS: P-256 key generation, signing and tamper rejection passed");
    PK11_FreeSlot(slot);
    puts("NSS: ChaCha20-Poly1305 known answer and tamper rejection passed");
    if (NSS_Shutdown() != SECSuccess) return 6;
    if (mkdir("nss-db", 0700) || NSS_InitReadWrite("sql:nss-db") != SECSuccess) {
        printf("NSS SQL database initialization error: %d\n", PR_GetError());
        return 7;
    }
    if (NSS_Shutdown() != SECSuccess) return 8;
    puts("NSS: private SQL database initialization and shutdown passed");
    if (mkdir("nss-dbm", 0700) || NSS_InitReadWrite("dbm:nss-dbm") != SECSuccess) {
        printf("NSS DBM database initialization error: %d\n", PR_GetError());
        return 10;
    }
    if (NSS_Shutdown() != SECSuccess) return 11;
    if (NSS_InitReadWrite("dbm:nss-dbm") != SECSuccess) return 12;
    if (NSS_Shutdown() != SECSuccess) return 13;
    puts("NSS: DBM database creation, reopen and shutdown passed");
    puts("NSS: all early-platform checks passed");
    return 0;
}

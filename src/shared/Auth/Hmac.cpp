/*
 * This file is part of the OregonCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Auth/Hmac.h"
#include "BigNumber.h"

void HmacHash::hmac_internal_setup()
{
    uint8 temp[SEED_KEY_SIZE] = { 0x38, 0xA7, 0x83, 0x15, 0xF8, 0x92, 0x25, 0x30, 0x71, 0x98, 0x67, 0xB1, 0x8C, 0x4, 0xE2, 0xAA };

    memset(m_digest, 0, sizeof(m_digest));
    memcpy(&m_key, &temp, SEED_KEY_SIZE);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX_init(&m_ctx);
    HMAC_Init_ex(&m_ctx, &m_key, SEED_KEY_SIZE, EVP_sha1(), NULL);
#else
    m_ctx = HMAC_CTX_new();
    HMAC_Init_ex(m_ctx, &m_key, SEED_KEY_SIZE, EVP_sha1(), NULL);
#endif

}

HmacHash::HmacHash()
{
    hmac_internal_setup();
}

HmacHash::HmacHash(uint32 /*len*/, uint8* /*seed*/)
{
    hmac_internal_setup();
}

HmacHash::~HmacHash()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX_cleanup(&m_ctx);
#else
    HMAC_CTX_free(m_ctx);
#endif
}

void HmacHash::UpdateBigNumber(BigNumber* bn)
{
    UpdateData(bn->AsByteArray(), bn->GetNumBytes());
}

void HmacHash::UpdateData(const uint8* data, int length)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_Update(&m_ctx, data, length);
#else
    HMAC_Update(m_ctx, data, length);
#endif
}

void HmacHash::UpdateData(const std::string& str)
{
    UpdateData((uint8 const*)str.c_str(), str.length());
}

void HmacHash::Initialize()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_Init_ex(&m_ctx, &m_key, SEED_KEY_SIZE, EVP_sha1(), NULL);
#else
    HMAC_Init_ex(m_ctx, &m_key, SEED_KEY_SIZE, EVP_sha1(), NULL);
#endif
}

void HmacHash::Finalize()
{
    uint32 length = 0;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_Final(&m_ctx, (uint8*)m_digest, &length);
#else
    HMAC_Final(m_ctx, (uint8*)m_digest, &length);
#endif
    ASSERT(length == SHA_DIGEST_LENGTH);
}

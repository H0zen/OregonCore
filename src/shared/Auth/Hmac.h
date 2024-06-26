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

#ifndef _AUTH_HMAC_H
#define _AUTH_HMAC_H

#include "Common.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>

class BigNumber;

#define SEED_KEY_SIZE 16

class HmacHash
{
    public:
        HmacHash();
        HmacHash(uint32 len, uint8* seed);
        ~HmacHash();
        void UpdateBigNumber(BigNumber* bn);
        void UpdateData(const uint8* data, int length);
        void UpdateData(const std::string& str);
        void Initialize();
        void Finalize();
        const uint8* GetDigest()
        {
            return m_digest;
        };
        const int GetLength()
        {
            return SHA_DIGEST_LENGTH;
        };
    private:
        void hmac_internal_setup();

#if OPENSSL_VERSION_NUMBER < 0x10100000L
        HMAC_CTX m_ctx;
#else
        HMAC_CTX* m_ctx;
#endif
        uint8 m_key[SEED_KEY_SIZE];
        uint8 m_digest[SHA_DIGEST_LENGTH];
};
#endif


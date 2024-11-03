/* -*- c++ -*- */
/* 
 * Copyright 2022 Graham J. Norbury
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <string>

/* DES */
#include <bitset>
#include <unordered_map>
#include <sstream>
#include <iomanip>

#include "p25_crypt_algs.h"
#include "op25_msg_types.h"

// constructor
p25_crypt_algs::p25_crypt_algs(log_ts& logger, int debug, int msgq_id) :
    logts(logger),
    d_debug(debug),
    d_msgq_id(msgq_id),
    d_pr_type(PT_UNK),
    d_algid(0x80),
    d_keyid(0),
    d_mi{0},
    d_key_iter(d_keys.end()),
    d_adp_position(0),
    d_des_position(0) {
}

// destructor
p25_crypt_algs::~p25_crypt_algs() {
}

// remove all stored keys
void p25_crypt_algs::reset(void) {
    d_keys.clear();
}

// add or update a key
void p25_crypt_algs::key(uint16_t keyid, uint8_t algid, const std::vector<uint8_t> &key) {
    if ((keyid == 0) || (algid == 0x80))
        return;

    d_keys[keyid] = key_info(algid, key);
}

// generic entry point to prepare for decryption
bool p25_crypt_algs::prepare(uint8_t algid, uint16_t keyid, protocol_type pr_type, uint8_t *MI) {
    bool rc = false;
    d_algid = algid;
    d_keyid = keyid;
    memcpy(d_mi, MI, sizeof(d_mi));

    d_key_iter = d_keys.find(keyid);
    if (d_key_iter == d_keys.end()) {
        if (d_debug >= 10) {
            fprintf(stderr, "%s p25_crypt_algs::prepare: keyid[0x%x] not found\n", logts.get(d_msgq_id), keyid);
        }
        return rc;
    }
    if (d_debug >= 10) {
        fprintf(stderr, "%s p25_crypt_algs::prepare: keyid[0x%x] found\n", logts.get(d_msgq_id), keyid);
    }

    switch (algid) {
        case 0xaa: // ADP RC4
            d_adp_position = 0;
            d_pr_type = pr_type;
            adp_keystream_gen();
            rc = true;
            break;
        case 0x81: // DES-OFB
            d_des_position = 0;
            d_pr_type = pr_type;
            des_keystream_gen();
            rc = true;
            break;
    
        default:
            break;
    }
    return rc;
}

// generic entry point to perform decryption
bool p25_crypt_algs::process(packed_codeword& PCW, frame_type fr_type, int voice_subframe) {
    bool rc = false;

    if (d_key_iter == d_keys.end())
        return false;

    switch (d_algid) {
        case 0xaa: // ADP RC4
            rc = adp_process(PCW, fr_type, voice_subframe);
            break;
        case 0x81: //DES-OFB
            rc = des_ofb_process(PCW, fr_type, voice_subframe);
            break;
    
        default:
            break;
    }

    return rc;
}

// ADP RC4 decryption
bool p25_crypt_algs::adp_process(packed_codeword& PCW, frame_type fr_type, int voice_subframe) {
    bool rc = true;
    size_t offset = 256;

    if (d_key_iter == d_keys.end())
        return false;

    switch (fr_type) {
        case FT_LDU1:
            offset = 0;
            break;
        case FT_LDU2:
            offset = 101;
            break;
        case FT_4V_0:
            offset += 7 * voice_subframe;
            break;
        case FT_4V_1:
            offset += 7 * (voice_subframe + 4);
            break;
        case FT_4V_2:
            offset += 7 * (voice_subframe + 8);
            break;
        case FT_4V_3:
            offset += 7 * (voice_subframe + 12);
            break;
        case FT_2V:
            offset += 7 * (voice_subframe + 16);
            break;
        default:
            rc = false;
            break;
    }
    if (d_pr_type == PT_P25_PHASE1) {
        //FDMA
        offset += (d_adp_position * 11) + 267 + ((d_adp_position < 8) ? 0 : 2); // voice only; skip LCW and LSD
        d_adp_position = (d_adp_position + 1) % 9;
        for (int j = 0; j < 11; ++j) {
            PCW[j] = adp_keystream[j + offset] ^ PCW[j];
        }
    } else if (d_pr_type == PT_P25_PHASE2) {
        //TDMA
        for (int j = 0; j < 7; ++j) {
            PCW[j] = adp_keystream[j + offset] ^ PCW[j];
        }
        PCW[6] &= 0x80; // mask everything except the MSB of the final codeword
    }

    return rc;
}

bool p25_crypt_algs::des_ofb_process(packed_codeword& PCW, frame_type fr_type, int voice_subframe) {
    bool rc = true;
    size_t offset = 0;
    
    switch (fr_type) {
		/* FDMA */
        case FT_LDU1:
            offset = 0;
            break;
        case FT_LDU2:
            offset = 101;
            break;
        /* TDMA */
        case FT_4V_0:
            offset += 7 * voice_subframe;
            break;
        case FT_4V_1:
            offset += 7 * (voice_subframe + 4);
            break;
        case FT_4V_2:
            offset += 7 * (voice_subframe + 8);
            break;
        case FT_4V_3:
            offset += 7 * (voice_subframe + 12);
            break;
        case FT_2V:
            offset += 7 * (voice_subframe + 16);
            break;
        default:
            rc = false;
            break;
    }
    
	//fprintf(stderr, "\t\t\t\t  ");
	if(d_pr_type == PT_P25_PHASE1) {
		//FDMA
	    offset += (d_des_position * 11) + 3 + ((d_des_position < 8) ? 0 : 2); // voice only; skip LCW and LSD
        d_des_position = (d_des_position + 1) % 9;
        for (int j = 0; j < 11; ++j) {
            PCW[j] = des_ks[j + offset] ^ PCW[j];
            //fprintf(stderr, "%02X ", PCW[j]);
        }
	} else if (d_pr_type == PT_P25_PHASE2) {
        //TDMA - NOT IMPLEMENTED
        /*
        for (int j = 0; j < 7; ++j) {
            PCW[j] = adp_keystream[j + offset] ^ PCW[j];
        }
        PCW[6] &= 0x80; // mask everything except the MSB of the final codeword
        */
    }
    //fprintf(stderr, "\n");
    return rc;
}

// ADP RC4 helper routine to swap two bytes
void p25_crypt_algs::adp_swap(uint8_t *S, uint32_t i, uint32_t j) {
    uint8_t temp = S[i];
    S[i] = S[j];
    S[j] = temp;
}

// ADP RC4 create keystream using supplied key and message_indicator
void p25_crypt_algs::adp_keystream_gen() {
    uint8_t adp_key[13], S[256], K[256];
    uint32_t i, j, k;

    if (d_key_iter == d_keys.end())
        return;

    // Find key value from keyid and set up to create keystream
    std::vector<uint8_t>::const_iterator kval_iter = d_key_iter->second.key.begin();
    for (i = 0; i < (uint32_t)std::max(5-(int)(d_key_iter->second.key.size()), 0); i++) {
        adp_key[i] = 0;             // pad with leading 0 if supplied key too short 
    }
    for ( ; i < 5; i++) {
        adp_key[i] = *kval_iter++;  // copy up to 5 bytes into key array
    }

    j = 0;
    for (i = 5; i < 13; ++i) {
        adp_key[i] = d_mi[i - 5];   // append MI bytes
    }

    for (i = 0; i < 256; ++i) {
        K[i] = adp_key[i % 13];
    }

    for (i = 0; i < 256; ++i) {
        S[i] = i;
    }

    for (i = 0; i < 256; ++i) {
        j = (j + S[i] + K[i]) & 0xFF;
        adp_swap(S, i, j);
    }

    i = j = 0;

    for (k = 0; k < 469; ++k) {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        adp_swap(S, i, j);
        adp_keystream[k] = S[(S[i] + S[j]) & 0xFF];
    }
}

std::string p25_crypt_algs::hex2bin(std::string s) 
{ 
	// hexadecimal to binary conversion 
	std::unordered_map<char, std::string> mp; 
	mp['0'] = "0000"; 
	mp['1'] = "0001"; 
	mp['2'] = "0010"; 
	mp['3'] = "0011"; 
	mp['4'] = "0100"; 
	mp['5'] = "0101"; 
	mp['6'] = "0110"; 
	mp['7'] = "0111"; 
	mp['8'] = "1000"; 
	mp['9'] = "1001"; 
	mp['A'] = "1010"; 
	mp['B'] = "1011"; 
	mp['C'] = "1100"; 
	mp['D'] = "1101"; 
	mp['E'] = "1110"; 
	mp['F'] = "1111"; 
	std::string bin = ""; 
	for (size_t i = 0; i < s.size(); i++) { 
		bin += mp[s[i]]; 
	} 
	return bin; 
} 
std::string p25_crypt_algs::bin2hex(std::string s) 
{ 
	// binary to hexadecimal conversion 
	std::unordered_map<std::string, std::string> mp;
	mp["0000"] = "0"; 
	mp["0001"] = "1"; 
	mp["0010"] = "2"; 
	mp["0011"] = "3"; 
	mp["0100"] = "4"; 
	mp["0101"] = "5"; 
	mp["0110"] = "6"; 
	mp["0111"] = "7"; 
	mp["1000"] = "8"; 
	mp["1001"] = "9"; 
	mp["1010"] = "A"; 
	mp["1011"] = "B"; 
	mp["1100"] = "C"; 
	mp["1101"] = "D"; 
	mp["1110"] = "E"; 
	mp["1111"] = "F"; 
	std::string hex = "";
	for (size_t i = 0; i < s.length(); i += 4) { 
		std::string ch = "";
		ch += s[i]; 
		ch += s[i + 1]; 
		ch += s[i + 2]; 
		ch += s[i + 3]; 
		hex += mp[ch]; 
	} 
	return hex; 
} 

std::string p25_crypt_algs::permute(std::string k, int* arr, int n)
{ 
	std::string per = "";
	for (int i = 0; i < n; i++) { 
		per += k[arr[i] - 1]; 
	} 
	return per; 
} 

std::string p25_crypt_algs::shift_left(std::string k, int shifts)
{ 
	std::string s = "";
	for (int i = 0; i < shifts; i++) { 
		for (int j = 1; j < 28; j++) { 
			s += k[j]; 
		} 
		s += k[0]; 
		k = s; 
		s = ""; 
	} 
	return k; 
} 

std::string p25_crypt_algs::xor_(std::string a, std::string b)
{ 
	std::string ans = "";
	for (size_t i = 0; i < a.size(); i++) { 
		if (a[i] == b[i]) { 
			ans += "0"; 
		} 
		else { 
			ans += "1"; 
		} 
	} 
	return ans; 
} 
std::string p25_crypt_algs::encrypt(std::string pt, std::vector<std::string> rkb, std::vector<std::string> rk)
{ 
	// Hexadecimal to binary 
	pt = hex2bin(pt); 

	// Initial Permutation Table 
	int initial_perm[64] = { 58, 50, 42, 34, 26, 18, 10, 2, 
							60, 52, 44, 36, 28, 20, 12, 4, 
							62, 54, 46, 38, 30, 22, 14, 6, 
							64, 56, 48, 40, 32, 24, 16, 8, 
							57, 49, 41, 33, 25, 17, 9, 1, 
							59, 51, 43, 35, 27, 19, 11, 3, 
							61, 53, 45, 37, 29, 21, 13, 5, 
							63, 55, 47, 39, 31, 23, 15, 7 }; 
	// Initial Permutation 
	pt = permute(pt, initial_perm, 64); 
	//cout << "After initial permutation: " << bin2hex(pt) << endl; 

	// Splitting 
	std::string left = pt.substr(0, 32);
	std::string right = pt.substr(32, 32);
	//cout << "After splitting: L0=" << bin2hex(left) << " R0=" << bin2hex(right) << endl; 

	// Expansion D-box Table 
	int exp_d[48] = { 32, 1, 2, 3, 4, 5, 4, 5, 
					6, 7, 8, 9, 8, 9, 10, 11, 
					12, 13, 12, 13, 14, 15, 16, 17, 
					16, 17, 18, 19, 20, 21, 20, 21, 
					22, 23, 24, 25, 24, 25, 26, 27, 
					28, 29, 28, 29, 30, 31, 32, 1 }; 

	// S-box Table 
	int s[8][4][16] = { { 14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7, 
						0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8, 
						4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0, 
						15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13 }, 
						{ 15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10, 
						3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5, 
						0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15, 
						13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9 }, 

						{ 10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8, 
						13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1, 
						13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7, 
						1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12 }, 
						{ 7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15, 
						13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9, 
						10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4, 
						3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14 }, 
						{ 2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9, 
						14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6, 
						4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14, 
						11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3 }, 
						{ 12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11, 
						10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8, 
						9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6, 
						4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13 }, 
						{ 4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1, 
						13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6, 
						1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2, 
						6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12 }, 
						{ 13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7, 
						1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2, 
						7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8, 
						2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11 } }; 

	// Straight Permutation Table 
	int per[32] = { 16, 7, 20, 21, 
					29, 12, 28, 17, 
					1, 15, 23, 26, 
					5, 18, 31, 10, 
					2, 8, 24, 14, 
					32, 27, 3, 9, 
					19, 13, 30, 6, 
					22, 11, 4, 25 }; 

	//cout << endl; 
	for (int i = 0; i < 16; i++) { 
		// Expansion D-box 
		std::string right_expanded = permute(right, exp_d, 48);

		// XOR RoundKey[i] and right_expanded 
		std::string x = xor_(rkb[i], right_expanded);

		// S-boxes 
		std::string op = "";
		for (int i = 0; i < 8; i++) { 
			int row = 2 * int(x[i * 6] - '0') + int(x[i * 6 + 5] - '0'); 
			int col = 8 * int(x[i * 6 + 1] - '0') + 4 * int(x[i * 6 + 2] - '0') + 2 * int(x[i * 6 + 3] - '0') + int(x[i * 6 + 4] - '0'); 
			int val = s[i][row][col]; 
			op += char(val / 8 + '0'); 
			val = val % 8; 
			op += char(val / 4 + '0'); 
			val = val % 4; 
			op += char(val / 2 + '0'); 
			val = val % 2; 
			op += char(val + '0'); 
		} 
		// Straight D-box 
		op = permute(op, per, 32); 

		// XOR left and op 
		x = xor_(op, left); 

		left = x; 

		// Swapper 
		if (i != 15) { 
			swap(left, right); 
		} 
		//cout << "Round " << i + 1 << " " << bin2hex(left) << " " << bin2hex(right) << " " << rk[i] << endl; 
	} 

	// Combination 
	std::string combine = left + right;

	// Final Permutation Table 
	int final_perm[64] = { 40, 8, 48, 16, 56, 24, 64, 32, 
						39, 7, 47, 15, 55, 23, 63, 31, 
						38, 6, 46, 14, 54, 22, 62, 30, 
						37, 5, 45, 13, 53, 21, 61, 29, 
						36, 4, 44, 12, 52, 20, 60, 28, 
						35, 3, 43, 11, 51, 19, 59, 27, 
						34, 2, 42, 10, 50, 18, 58, 26, 
						33, 1, 41, 9, 49, 17, 57, 25 }; 

	// Final Permutation 
	std::string cipher = bin2hex(permute(combine, final_perm, 64));
	return cipher; 
}

void p25_crypt_algs::string2ByteArray(const std::string& s, uint8_t array[], int offset) {
	int byteCount = s.size() / 2; // Number of bytes in the hex string

	for (int i = 0; i < byteCount; ++i) {
		// Take two characters from the hex string and convert to byte
		std::string byteString = s.substr(i * 2, 2);
		array[offset+i] = static_cast<uint8_t>(stoul(byteString, nullptr, 16));
	}
}

std::string p25_crypt_algs::byteArray2string(uint8_t array[]) {
	std::stringstream hexStream;

	for (int i = 0; i < 8; ++i) {
		// Convert each byte to a 2-character hex string
		hexStream << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)array[i];
	}

	return hexStream.str();
}

void p25_crypt_algs::des_keystream_gen() {
	std::string key, mi, ct;
	uint8_t des_key[8];
	uint32_t i;
	
	if (d_key_iter == d_keys.end())
		return;

	// Find key value from keyid and set up to create keystream
	std::vector<uint8_t>::const_iterator kval_iter = d_key_iter->second.key.begin();
	for (i = 0; i < (uint32_t)std::max(8 - (int)(d_key_iter->second.key.size()), 0); i++) {
		des_key[i] = 0;             // pad with leading 0 if supplied key too short 
	}
	for (; i < 8; i++) {
		des_key[i] = *kval_iter++;  // copy up to 8 bytes into key array
	}

	// Append supplied key array to string
	key = byteArray2string(des_key);

	/* Append supplied MI to string */
	mi = byteArray2string(d_mi);
	//fprintf(stderr, "%s MI: 0x%s \n", logts.get(d_msgq_id), mi.c_str());
	//fprintf(stderr, "%s DES key: 0x%s \n", logts.get(d_msgq_id), key.c_str());

	// Key Generation, Hex to Binary
	key = hex2bin(key);

	// Parity bit drop table 
	int keyp[56] = { 57, 49, 41, 33, 25, 17, 9,
					1, 58, 50, 42, 34, 26, 18,
					10, 2, 59, 51, 43, 35, 27,
					19, 11, 3, 60, 52, 44, 36,
					63, 55, 47, 39, 31, 23, 15,
					7, 62, 54, 46, 38, 30, 22,
					14, 6, 61, 53, 45, 37, 29,
					21, 13, 5, 28, 20, 12, 4 };

	// getting 56 bit key from 64 bit using the parity bits 
	key = permute(key, keyp, 56); // key without parity 

	// Number of bit shifts 
	int shift_table[16] = { 1, 1, 2, 2,
							2, 2, 2, 2,
							1, 2, 2, 2,
							2, 2, 2, 1 };

	// Key-Compression Table 
	int key_comp[48] = { 14, 17, 11, 24, 1, 5,
						3, 28, 15, 6, 21, 10,
						23, 19, 12, 4, 26, 8,
						16, 7, 27, 20, 13, 2,
						41, 52, 31, 37, 47, 55,
						30, 40, 51, 45, 33, 48,
						44, 49, 39, 56, 34, 53,
						46, 42, 50, 36, 29, 32 };

	// Splitting 
	std::string left = key.substr(0, 28);
	std::string right = key.substr(28, 28);

	std::vector<std::string> rkb; // rkb for RoundKeys in binary 
	std::vector<std::string> rk; // rk for RoundKeys in hexadecimal 
	for (int i = 0; i < 16; i++) {
		// Shifting 
		left = shift_left(left, shift_table[i]);
		right = shift_left(right, shift_table[i]);

		// Combining 
		std::string combine = left + right;

		// Key Compression 
		std::string RoundKey = permute(combine, key_comp, 48);

		rkb.push_back(RoundKey);
		rk.push_back(bin2hex(RoundKey));
	}

	/* OFB mode */
	for (int i = 0; i < 28; i++) {
		if (i == 0)
			// First run using supplied IV
			ct = encrypt(mi, rkb, rk);
		else
			ct = encrypt(ct, rkb, rk);

		if (i > 1) {
			// Discard first 2 keystreams
			int offset = (i - 2) * 8;

			// Append keystream to ks_array
			string2ByteArray(ct, des_ks, offset);
		}
	}
}

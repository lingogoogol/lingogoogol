#ifndef LGO_ENCODER_I_H
#define LGO_ENCODER_I_H

#include "d.h"
#include "../arr/.h"
#include "../file/.h"

namespace lgo {
	template<base64_type type, typename T1>
	strd base64_encode(const T1& in) {
		auto get_base64url_char{
			[](intf16 index)->char {
				if (index < 26) {
					return 'A' + index;
				}
				index -= 26;
				if (index < 26) {
					return 'a' + index;
				}
				index -= 26;
				if (index < 10) {
					return '0' + index;
				}
				index -= 10;
				if constexpr (type == base64_type::normal) {
					if (index == 0) {
						return '+';
					}
					else {
						return '/';
					}
				}
				else if constexpr (type == base64_type::url) {
					if (index == 0) {
						return '-';
					}
					else {
						return '_';
					}
				}
			}
		};
		constexpr intf64 type_bits{ size<byte>() * 8 };
		strd out{};
		intf64 begin{ 0 };
		while (begin < in.size() * 8 - 6) {
			intf64 end{ begin + 6 };
			intf64 temp1{ begin % type_bits };
			intf64 temp2{ end % type_bits };
			intf64 i1{ div_rdown(begin, type_bits) };
			intf64 i2{ div_rdown(end, type_bits) };
			bool cross{ i2 - i1 != 0 };
			out.push(get_base64url_char((((temp2 == 0 && !cross ? 0 :
				in[i1] >> (cross ? 0 : type_bits - temp2)) << (cross ? temp2 : 0)) |
				(temp2 == 0 ? 0 : in[i2] >> (type_bits - temp2))) % 0x40));
			begin += 6;
		}
		out.push(get_base64url_char((in[in.len() - 1] <<
			(begin + 6 - in.size() * 8)) % 0x40));
		return out;
	}

	template<base64_type type>
	dmem base64_decode(const strd& in) {
		auto get_Base64url_index{
			[](char character)->byte {
				if (character >= 'A' && character <= 'Z') return character - 'A';
				if (character >= 'a' && character <= 'z') return character - 'a' + 26;
				if (character >= '0' && character <= '9') return character - '0' + 52;
				if constexpr (type == base64_type::normal) {
					if (character == '+') return 62;
					if (character == '/') return 63;
					else throw U"�����T���s�X�C";
				}
				else if constexpr (type == base64_type::url) {
					if (character == '-') return 62;
					if (character == '_') return 63;
					else throw U"�����T���s�X�C";
				}
			}
		};
		dmem out{};
		intf64 in_pos{ 0 };
		intf64 out_pos{ 0 };
		while (in_pos < in.len()) {
			out[out_pos] = get_Base64url_index(in[in_pos++]) << 2;
			out[out_pos++] |= get_Base64url_index(in[in_pos]) >> 4;
			auto tmp{ sc<byte>(get_Base64url_index(in[in_pos++]) << 4) };
			if (tmp != 0)
				out[out_pos] = tmp;
			if (in_pos < in.len())
				out[out_pos++] |= get_Base64url_index(in[in_pos]) >> 2;
			else
				break;
			out[out_pos] = get_Base64url_index(in[in_pos++]) << 6;
			if (in_pos < in.len())
				out[out_pos++] |= get_Base64url_index(in[in_pos++]);
			else
				break;
		}
		return out;
	}

	template<is_mem t_in>
	smem<20> sha1(t_in in) {
		auto transform{
			[](dmem& in, intf64 block_num, arrs<uint32, 5>& out)->void {
				for (intf64 block{ 0 }; block < block_num; block++) {
					arrs<uint32, 80> words{ in.getpos(block * 64).get<arrs<uint32, 16>>() };
					for (intf64 i{ 16 }; i < 80; ++i) {
						words[i] = lro(words[i - 3] ^
							words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
					}
					arrs<uint32, 5> work{ out };
					for (intf64 i{ 0 }; i < 80; ++i) {
						uint32 f{};
						uint32 k{};
						if (i < 20) {
							f = (work[1] & work[2]) | (~work[1] & work[3]);
							k = 0x5a827999;
						}
						else if (i < 40) {
							f = work[1] ^ work[2] ^ work[3];
							k = 0x6ed9eba1;
						}
						else if (i < 60) {
							f = (work[1] & work[2]) | (work[1] & work[3]) | (work[2] & work[3]);
							k = 0x8f1bbcdc;
						}
						else {
							f = work[1] ^ work[2] ^ work[3];
							k = 0xca62c1d6;
						}
						uint32 temp{ lro(work[0], 5) + f + work[4] + k + words[i] };
						work[4] = work[3];
						work[3] = work[2];
						work[2] = lro(work[1], 30);
						work[1] = work[0];
						work[0] = temp;
					}
					for (intf64 i{ 0 }; i < 5; ++i) {
						out[i] += work[i];
					}
				}
			}
		};
		arrs<uint32, 5> out{ 0x67452301, 0xefcdab89,
			0x98badcfe, 0x10325476, 0xc3d2e1f0 };
		intf64 remain_len{ in.len() % 64 };
		intf64 body_num{ div_rdown(in.len(), 64) };
		intf64 padded_num{ sc<intf64>(1) + (remain_len > 55 ? 1 : 0) };
		intf64 padded_len{ padded_num * 64 };
		transform(in, body_num, out);
		dmem end{};
		for (intf64 i{ 0 }; i < remain_len; ++i) {
			end.push(in[body_num * 64 + i]);
		}
		end.push(sc<uint8>(0x80));
		for (intf64 i{ 1 }; i < padded_len - remain_len; ++i) {
			end.push(sc<uint8>(0x00));
		}
		end.pad(sc<uintf64>(in.len() * 8), padded_len - 8);
		transform(end, padded_num, out);
		return smem<20>{ out };
	}
	
	template<is_mem t_in>
	smem<32> sha256(t_in in) {
		auto transform{
			[] (dmem& in, intf64 block_num, arrs<uint32, 8>& out)->void {
				const arrs<uint32, 64> k{
					0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
					0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
					0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
					0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
					0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
					0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
					0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
					0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
					0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
					0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
					0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
					0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
					0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
					0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
					0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
					0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
				};
				for (intf64 block{ 0 }; block < block_num; block++) {
					arrs<uint32, 64> words{ in.getpos(block * 64).get<arrs<uint32, 16>>() };
					for (intf64 i{ 16 }; i < 64; ++i) {
						uint32 bits1{ words[i - sc<intf64>(15)] };
						uint32 bits2{ words[i - sc<intf64>(2)] };
						words[i] = words[i - sc<intf64>(16)] + words[i - sc<intf64>(7)] +
							(rro(bits1, 7) ^ rro(bits1, 18) ^ (bits1 >> 3)) +
							(rro(bits2, 17) ^ rro(bits2, 19) ^ (bits2 >> 10));
					}
					arrs<uint32, 8> work{ out };
					for (intf64 i{ 0 }; i < 64; ++i) {
						dnum<byte> temp1{ dnum<byte>{ words[i] } + dnum<byte>{ k[i] } +
							dnum<byte>{ work[7] } + dnum<byte>{ rro(work[4], 6) ^
							rro(work[4], 11) ^ rro(work[4], 25) } +
							dnum<byte>{ (work[4] & work[5]) ^ (~work[4] & work[6]) } };
						dnum<byte> temp2{ dnum<byte>{ rro(work[0], 2) ^
							rro(work[0], 13) ^ rro(work[0], 22) } +
							dnum<byte>{ (work[0] & work[1]) ^ (work[0] & work[2]) ^ (work[1] & work[2]) } };
						for (intf64 word{ 7 }; word >= 1; word--)
							work[word] = std::move(work[word - sc<intf64>(1)]);
						work[4] = dnum<byte>{ work[4] } + temp1;
						work[0] = temp1 + temp2;
					}
					for (intf64 i{ 0 }; i < 8; ++i) {
						out[i] += work[i];
					}
				}
			}
		};
		arrs<uint32, 8> out{
			0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
		};
		intf64 remain_len{ in.len() % 64 };
		intf64 body_num{ in.len() / 64 };
		intf64 padded_num{ sc<intf64>(1) + (remain_len > 55 ? 1 : 0) };
		intf64 padded_len{ padded_num * 64 };
		transform(in, body_num, out);
		dmem end{};
		for (intf64 i{ 0 }; i < remain_len; ++i) {
			end.push(in[body_num * 64 + i]);
		}
		end.push(sc<uint8>(0x80));
		for (intf64 i{ 1 }; i < padded_len - remain_len; ++i) {
			end.push(sc<uint8>(0x00));
		}
		end.pad(sc<uintf64>(in.len() * 8), padded_len - 8);
		transform(end, padded_num, out);
		return smem<32>{ out };
	}

	template<is_num t_in>
	dmem emsa_pkcs1_v1_5_encode(t_in in, intf64 out_len) {
		smem<32> hashed{ sha256(in) };
		arrs<uint8, 51> t{ 0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
			0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20 };
		for (intf64 i{ 0 }; i < 32; ++i) {
			t[19 + i] = hashed.get<uint8>();
		}
		if (out_len < 62) {
			throw U"�Q�n�����G�ӵu�C";
		}
		dmem out{ sc<uint16>(0x0001) };
		for (intf64 i{ 0 }; i < out_len - 54; ++i) {
			out.push(sc<uint8>(0xff));
		}
		out.push(sc<uint8>(0x00));
		for (intf64 i{ 0 }; i < 51; ++i) {
			out.push(t[i]);
		}
		return out;
	}

	template<typename T1>
	dnum<uint64> rsasp1(const dnum<uint64>& n, dnum<uint64> d, T1&& m) {
		arrd<dnum<uint64>> twos{};
		twos.resize(n.size() * 8);
		twos[0] = std::forward<T1>(m) % n;
		for (intf64 i{ 1 }; i < n.size() * 8; ++i) {
			twos[i] = std::move(((twos[i - 1] * twos[i - 1]) % n).resize(n.size() * 8 - 1, 0));
		}
		dnum<uint64> out{ 1 };
		for (intf64 i{ 0 }; i < n.size() * 8; ++i) {
			if (d % dnum<uint64>{ 2 } == dnum<uint64>{ 1 })
				out = std::move(((out * twos[i]) % n).resize(n.size() * 8 - 1, 0));
			d >>= 1;
		}
		return out;
	}

	inline dnum<uint64> rsassa_pkcs1_v1_5_sign(dmem in,
		const dnum<uint64>& n, const dnum<uint64>& d) {
		return rsasp1(n, d, dnum<uint64>{ emsa_pkcs1_v1_5_encode(in, n.size()) });
	}
}

#endif
#ifndef LGO_ENCODER_F_H
#define LGO_ENCODER_F_H

#include <cstdint>

#include "../mem/.h"
#include "../num/.h"
#include "../str/.h"

namespace lgo {
	enum class base64_type;

	template<base64_type type, typename T1>
	strd base64_encode(const T1& in);
	template<base64_type type>
	dmem base64_decode(const strd& in);
	template<is_mem t_in>
	smem<20> sha1(t_in in);
	template<is_mem t_in>
	smem<32> sha256(t_in in);
	template<is_mem t_in>
	dmem emsa_pkcs1_v1_5_encode(t_in in, intf64 out_len);
	template<typename T1>
	dnum<uint64> rsasp1(const dnum<uint64>& n, dnum<uint64> d, T1&& m);
	dnum<uint64> rsassa_pkcs1_v1_5_sign(dmem in,
		const dnum<uint64>& n, const dnum<uint64>& d);
}

#endif
#ifndef LGO_IO_NET_DEF
#define LGO_IO_NET_DEF

#include "asio.hpp"

LGO_EXPORT namespace lgo {
	class tcp_client: public lgo {
	public:
		tcp_client(const strmdc& ip, intpc port);
		~tcp_client();

		template<typename t_out, typename... t_param>
		auto get(t_param&&... param) -> t_out;
		template<typename t_self, typename t_in, typename... t_param>
		auto pad(this t_self&& self, const t_in& in, t_param&&... param) -> t_self&&;
	private:
		boost::asio::ip::tcp::socket m_socket{ boost_asio() };
		memmd m_remain{};
	};
}

#endif
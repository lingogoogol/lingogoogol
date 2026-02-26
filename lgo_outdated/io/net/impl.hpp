#ifndef LGO_IO_NET_IMPL
#define LGO_IO_NET_IMPL

#include "def.hpp"

namespace lgo {
	inline tcp_client::tcp_client(const strmdc& host, intpc port) {
		try {
			m_socket.connect({ boost::asio::ip::tcp::resolver{ boost_asio() }.resolve(host.sc<std::string>(), strmdc{}
			.pad_resize(port, int_base<intpc::val_v<U"A">>{}).sc<std::string>())->endpoint() });
		}
		catch (boost::system::system_error e) {
			std::cout << e.what() << '\n';
			throw e_other{ U""_strmdc };
		}
		return;
	}

	inline tcp_client::~tcp_client() {
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
		m_socket.close();
		return;
	};

	template<typename t_out, typename... t_param>
	inline auto tcp_client::get(t_param&&... param) -> t_out {
		memmd buf{ m_remain.r() };
		while (true) {
			try {
				t_out out{ buf.get_try<t_out>(::lgo::f<t_param>(param)...) };
				m_remain = buf.subcs(buf.getpos());
				return out;
			}
			catch (get_not_enough e) {
				intpc orig_len{ buf.len() };
				buf.resize(buf.len() + e.shortage());
				try {
					buf.resize(buf.len() - e.shortage() + ::lgo::sc<intpc>(m_socket
					.read_some(boost::asio::buffer((&buf[orig_len]).basic(), e.shortage().basic()))));
				}
				catch (const boost::system::system_error& e) {
					std::cout << e.what() << '\n';
					throw e_other{ U""_strmdc };
				}
			}
		}
	}

	template<typename t_self, typename t_in, typename... t_param>
	inline auto tcp_client::pad(this t_self&& self
	, const t_in& in, t_param&&... param) -> t_self&& {
		memmd buf{};
		buf.pad_resize(in, ::lgo::f<t_param>(param)...);
		boost::asio::write(self.m_socket, boost::asio::buffer(buf.data().basic(), buf.len().basic()));
		return self.f<t_self>();
	}
}

#endif
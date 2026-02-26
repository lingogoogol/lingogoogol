#define LGO_EXPORT
#include <lgo/basic/.hpp>
#include <lgo/io/.hpp>
#include <lgo/math/.hpp>
#include <lgo/encode/.hpp>
#undef LGO_EXPORT

#include <iostream>
#include <csignal>
#include <exception>

using namespace lgo::literal;

volatile std::sig_atomic_t emitted_signal{};

void signal_handler(int signal) {
	emitted_signal = signal;
	return;
}

void set_signal(int signal) {
	if (std::signal(signal, signal_handler) == SIG_ERR) {
		std::cout << "Failed to set signal handler: "
		<< signal << '.' << std::endl;
	}
	return;
}

int main() {
	try {
		set_signal(SIGTERM);
		set_signal(SIGSEGV);
		set_signal(SIGINT);
		set_signal(SIGILL);
		set_signal(SIGABRT);
		set_signal(SIGFPE);
		lgo::memms<lgo::intpc::val_v<U"10">> a{};
		a.pad(U"0"_intpc);
		lgo::cmd.pad(U"-9999.0"_numzsc);
		std::cout << std::endl;
		if (emitted_signal) {
			std::cout << "An signal was emitted." << std::endl;
		}
	}
	catch (...) {
		std::cout << "An exception was thrown." << std::endl;
	}
	while (true) {}
	return 0;
}
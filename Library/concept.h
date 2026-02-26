#ifndef LIBRARY_CONCEPT_H
#define LIBRARY_CONCEPT_H

template<typename T1, typename... T2>
concept initializable = requires (T2... a) {
	T1{ a... };
};

#endif
#ifndef CONCEPT_H
#define CONCEPT_H

#include "concept_f.h"

template<typename T1, typename... T2>
concept initializable = requires (T2... a) {
	T1{ a... };
};

#endif
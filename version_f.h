#ifndef VERSION_F_H
#define VERSION_F_H

#include <fstream>

template<int T1>
class Version;

template<int U1>
std::fstream& operator<<(std::fstream& fs, const Version<U1>& version);
template<int U1>
std::fstream& operator>>(std::fstream& fs, Version<U1>& version);

#endif
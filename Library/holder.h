#ifndef LIBRARY_HOLDER_H
#define LIBRARY_HOLDER_H

#include <map>
#include <functional>

#include "debug_f.h"
#include "holder_f.h"

template<typename T1>
class Copy_holder<T1*> {
public:
	template<typename U1>
	Copy_holder(U1* value_param, std::function<void(U1*)> destructor_param);
	Copy_holder(const Copy_holder<T1*>& holder);
	~Copy_holder();
	Copy_holder<T1*>& operator=(const Copy_holder<T1*>& holder);
	T1* operator*();
	T1** operator->();
private:
	unsigned int ID{};
	T1* value{};
	std::function<void(T1*)> destructor{};
	static unsigned int ID_to_use;
	static std::map<unsigned int, unsigned int> copy_count;
};

template<typename T1>
unsigned int Copy_holder<T1*>::ID_to_use{ 0 };
template<typename T1>
std::map<unsigned int, unsigned int> Copy_holder<T1*>::copy_count{};

template<typename U1>
Copy_holder(U1*, std::function<void(U1*)>)->Copy_holder<U1*>;

template<typename T1>
template<typename U1>
Copy_holder<T1*>::Copy_holder(U1* value_param, std::function<void(U1*)> destructor_param) :
	value{ value_param }, destructor{ destructor_param } {
	unsigned int first_ID_to_use{ ID_to_use };
	while (copy_count.count(ID_to_use) == 1) {
		ID_to_use++;
		if (ID_to_use == first_ID_to_use)
			handle_error(U"Copy_holder：太多物件，ID不夠用。");
	}
	copy_count.insert(std::pair<unsigned int, unsigned int>(first_ID_to_use, 1));
	ID = ID_to_use;
	ID_to_use++;
	return;
}

template<typename T1>
Copy_holder<T1*>::Copy_holder(const Copy_holder<T1*>& holder) :
	ID{ holder.ID }, value{ holder.value }, destructor{ holder.destructor } {
	copy_count[ID]++;
	return;
}

template<typename T1>
Copy_holder<T1*>::~Copy_holder() {
	if (copy_count[ID] == 1) {
		copy_count.erase(ID);
		destructor(value);
		delete value;
	}
	else
		copy_count[ID]--;
	return;
}

template<typename T1>
Copy_holder<T1*>& Copy_holder<T1*>::operator=(const Copy_holder<T1*>& holder) {
	ID = holder.ID;
	value = holder.value;
	destructor = holder.destructor;
	copy_count[ID]++;
	return *this;
}

template<typename T1>
T1* Copy_holder<T1*>::operator*() {
	return value;
}

template<typename T1>
T1** Copy_holder<T1*>::operator->() {
	return &value;
}

#endif
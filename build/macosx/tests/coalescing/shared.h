template<class T> inline T& shared_value() { static T value = 0; return value; }

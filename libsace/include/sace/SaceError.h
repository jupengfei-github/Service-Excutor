#ifndef _SACE_ERROR_H_
#define _SACE_ERROR_H_

#include <stdexcept>

class UnsupportedOperation : public runtime_error {
public:
	UnsupportedOperation (const string& msg):runtime_error(msg) {}
};

class RemoteException : public runtime_error {
public:
	RemoteException (const string& msg):runtime_error(msg) {}
};

class InvalidOperation : public runtime_error {
public:
	InvalidOperation (const string& msg):runtime_error(msg) {}
};

#endif
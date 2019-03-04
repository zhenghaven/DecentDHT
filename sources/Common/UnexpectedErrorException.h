#pragma once

#ifdef DEBUG

#include <DecentApi/Common/RuntimeException.h>

namespace Decent
{
	namespace Dht
	{
		class UnexpectedErrorException : public RuntimeException
		{
		public:
			UnexpectedErrorException(const char* file, int line) :
				RuntimeException("Unexpected error in file: " + std::string(file) + ", line: " + std::to_string(line) + ". ")
			{}

		private:

		};
	}
}

#define DEBUG_ASSERT(X) if(!static_cast<bool>(X)) { throw Decent::Dht::UnexpectedErrorException(__FILE__, __LINE__); }

#else

#define DEBUG_ASSERT(X)

#endif // DEBUG



#pragma once

#include <DecentApi/Common/RuntimeException.h>

namespace Decent
{
	namespace Dht
	{
		namespace AccessCtrl
		{
			class ParseError : public RuntimeException
			{
			public:
				explicit ParseError(const std::string& what_arg) :
					RuntimeException(what_arg)
				{}

				explicit ParseError(const char* what_arg) :
					RuntimeException(what_arg)
				{}
			};
		}
	}
}

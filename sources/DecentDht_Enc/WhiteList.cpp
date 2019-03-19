#include <DecentApi/Common/Ra/WhiteList/HardCoded.h>

using namespace Decent::Ra::WhiteList;

HardCoded::HardCoded() :
	StaticTypeList(WhiteListType({
		std::make_pair("3g1NYpHp/qwf+eXnb6Mx6xhyY75LDnHKD+YcadAMIN0=", sk_nameDecentServer)
		}))
{
}

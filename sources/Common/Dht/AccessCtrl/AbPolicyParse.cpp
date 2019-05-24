#include "AbPolicyParse.h"

#include <DecentApi/Common/make_unique.h>

#include "AbPolicy.h"

using namespace Decent::Dht;
using namespace Decent::Dht::AccessCtrl;

std::unique_ptr<AbPolicyBase> AccessCtrl::Parse(std::vector<uint8_t>::const_iterator& it, std::vector<uint8_t>::const_iterator end)
{
	if (it == end)
	{
		throw ParseError("Failed to parse attributed-based policy - binary array is empty.");
	}

	switch (*it)
	{
	case AbPolicyAny::sk_flag:
		return Tools::make_unique<AbPolicyAny>(it, end);
	case AbPolicyAttribute::sk_flag:
		return Tools::make_unique<AbPolicyAttribute>(it, end);
	case AbPolicyAnd::sk_flag:
	{
		++it;
		std::unique_ptr<AbPolicyBase> left = Parse(it, end);
		std::unique_ptr<AbPolicyBase> right = Parse(it, end);
		return std::move(left) & std::move(right);
	}
	case AbPolicyOr::sk_flag:
	{
		++it;
		std::unique_ptr<AbPolicyBase> left = Parse(it, end);
		std::unique_ptr<AbPolicyBase> right = Parse(it, end);
		return std::move(left) | std::move(right);
	}
	case AbPolicyNot::sk_flag:
	{
		++it;
		std::unique_ptr<AbPolicyBase> var = Parse(it, end);
		return Tools::make_unique<AbPolicyNot>(std::move(var));
	}
	default:
	{
		std::string uncgz(1, static_cast<const char>(*it));
		throw ParseError("Failed to parse attributed-based policy - unrecognized flag, " + uncgz + ".");
	}
	}
}

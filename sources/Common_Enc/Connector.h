#pragma once

#include <sgx_error.h>

#ifdef __cplusplus
extern "C" {
#endif
	sgx_status_t ocall_decent_dht_cnt_mgr_close_cnt(void* cnt_ptr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

namespace Decent
{
	namespace Dht
	{
		typedef sgx_status_t(*CntBuilderType)(void**);

		struct Connector
		{
			Connector(CntBuilderType cntBuilder) :
				m_ptr(nullptr)
			{
				if ((*cntBuilder)(&m_ptr) != SGX_SUCCESS)
				{
					m_ptr = nullptr;
				}
			}

			~Connector()
			{
				ocall_decent_dht_cnt_mgr_close_cnt(m_ptr);
			}
			void* m_ptr;
		};
	}
}

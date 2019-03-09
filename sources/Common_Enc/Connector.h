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
		struct EnclaveConnector
		{
			template<class Tp, class... Args>
			EnclaveConnector(Tp cntBuilder, Args&&... args) :
				m_ptr(nullptr)
			{
				if ((*cntBuilder)(&m_ptr, std::forward<Args>(args)...) != SGX_SUCCESS)
				{
					m_ptr = nullptr;
				}
			}

			~EnclaveConnector()
			{
				ocall_decent_dht_cnt_mgr_close_cnt(m_ptr);
			}
			void* m_ptr;
		};
	}
}

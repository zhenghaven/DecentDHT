#pragma once

#include <DecentApi/DecentAppEnclave/AppStates.h>
#include <DecentApi/Common/MbedTls/BigNumber.h>

#if defined(ENCLAVE_PLATFORM_SGX)
#	include <sgx_quote.h>
#else
typedef struct _spid_t
{
	uint8_t             id[16];
} sgx_spid_t;
#endif

namespace Decent
{
	namespace Dht
	{
		template<typename IdType, size_t KeySizeByte, typename AddrType, bool checkCircleRange>
		class LocalNode;

		class EnclaveStore;
		class DhtSecureConnectionMgr;

		class DhtStates : public Ra::AppStates
		{
		public: //public member:
			static constexpr size_t sk_keySizeByte = static_cast<size_t>(32);

			typedef LocalNode<MbedTlsObj::BigNumber, sk_keySizeByte, uint64_t, false> DhtLocalNodeType;
			typedef std::shared_ptr<DhtLocalNodeType> DhtLocalNodePtrType;

		public:
			DhtStates(Ra::AppCertContainer & certCntnr, Ra::KeyContainer & keyCntnr, Ra::WhiteList::DecentServer & serverWl, GetLoadedWlFunc getLoadedFunc, EnclaveStore& dhtStore, DhtSecureConnectionMgr& cntPool) :
				AppStates(certCntnr, keyCntnr, serverWl, getLoadedFunc),
				m_dhtNode(),
				m_dhtStore(dhtStore),
				m_cntPool(cntPool),
				m_iasCntor(nullptr)
			{}
			
			virtual ~DhtStates()
			{}

			const DhtLocalNodePtrType& GetDhtNode() const
			{
				return m_dhtNode;
			}

			DhtLocalNodePtrType& GetDhtNode()
			{
				return m_dhtNode;
			}

			const EnclaveStore& GetDhtStore() const
			{
				return m_dhtStore;
			}

			EnclaveStore& GetDhtStore()
			{
				return m_dhtStore;
			}

			const DhtSecureConnectionMgr& GetConnectionMgr() const
			{
				return m_cntPool;
			}

			DhtSecureConnectionMgr& GetConnectionMgr()
			{
				return m_cntPool;
			}

			void SetIasConnector(void* ptr)
			{
				m_iasCntor = ptr;
			}

			void* GetIasConnector() const
			{
				return m_iasCntor;
			}

			void SetEnclaveId(uint64_t enclaveId)
			{
				m_enclaveId = enclaveId;
			}

			uint64_t GetEnclaveId() const
			{
				return m_enclaveId;
			}

			void SetSpid(std::shared_ptr<sgx_spid_t> spid)
			{
				m_spid = spid;
			}

			std::shared_ptr<const sgx_spid_t> GetSpid() const
			{
				return m_spid;
			}

		private:
			DhtLocalNodePtrType m_dhtNode;
			EnclaveStore& m_dhtStore;
			DhtSecureConnectionMgr& m_cntPool;

			void* m_iasCntor;
			uint64_t m_enclaveId;
			std::shared_ptr<sgx_spid_t> m_spid;
		};
	}
}

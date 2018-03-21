#pragma once

extern "C"
{
#include "bcm_host.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/mmal_logging.h"
}

#include <memory>
#include <exception>
#include <cstdint>

namespace MMAL
{
	// forward decls
	class Port;

	template<typename T>
	class Base
	{
	protected:
		T* Ptr = nullptr;
	public:
		Base(T* raw) :
			Ptr(raw)
		{
			if (!Ptr)
				throw std::runtime_error("Pointer is null");
		}
		virtual ~Base() {}

		T* GetRawPtr() const
		{
			return Ptr;
		}
	};

	class Port :
			public Base<MMAL_PORT_T>
	{
	public:
		using Base::Base;

		size_t GetBufferCount() const { return Ptr->buffer_num; }
		void SetBufferCount(size_t count) { Ptr->buffer_num = count; }

		size_t GetBufferSize() const { return Ptr->buffer_size; }
		void SetBufferSize(size_t size) { Ptr->buffer_size = size; }

		MMAL_ES_FORMAT_T& Format()
		{
			return *Ptr->format;
		}

		void CopyFormat(MMAL_ES_FORMAT_T& src)
		{
			mmal_format_copy(Ptr->format, &src);
		}

		bool CommitFormatChanges()
		{
			return mmal_port_format_commit(Ptr) == MMAL_SUCCESS;
		}

		template<typename T>
		bool SetParameter(const T& p)
		{
			return mmal_port_parameter_set(Ptr, &p.hdr) == MMAL_SUCCESS;
		}

		template<typename T>
		bool SetParameter(T id, uint32_t value)
		{
			return mmal_port_parameter_set_uint32(Ptr, id, value) == MMAL_SUCCESS;
		}

		template<typename T>
		bool SetParameter(T id, bool value)
		{
			return mmal_port_parameter_set_boolean(Ptr, id, value ? 1 : 0) == MMAL_SUCCESS;
		}

		bool Enable(MMAL_PORT_BH_CB_T callback)
		{
			return mmal_port_enable(Ptr, callback) == MMAL_SUCCESS;
		}

		bool Disable()
		{
			return mmal_port_disable(Ptr) == MMAL_SUCCESS;
		}

		std::shared_ptr<Pool> CreatePool()
		{
			return CreatePool(Ptr->buffer_num, Ptr->buffer_size);
		}

		std::shared_ptr<Pool> CreatePool(size_t buffer_count, size_t buffer_size)
		{
			return std::make_shared<Pool>(mmal_port_pool_create(Ptr, buffer_count, buffer_size), );
		}
	};

	class Pool :
			public Base<MMAL_POOL_T>
	{
		Port* Owner;
	public:
		Pool(MMAL_BOOL_T* raw, Port* owner) :
			Base(raw),
			Owner(owner)
		{
		}

		~Pool()
		{
			mmal_port_pool_destroy(Owner, Ptr);
		}
	};

	class Connection :
			public Base<MMAL_CONNECTION_T>
	{
	public:
		using Base::Base;

		~Connection()
		{
			if (Ptr)
			{
				mmal_connection_destroy(Ptr);
			}
		}

		template<typename SPtrT>
		static std::unique_ptr<Connection> Connect(SPtrT<Port> p1, SPtrT<Port> p2, size_t flags = MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT)
		{
			MMAL_CONNECTION_T* connection = nullptr;

			mmal_connection_create(&connection, p1->GetRawPtr(), p2->GetRawPtr(), flags);

			return std::make_unique<Connection>(connection);
		}

		bool Enable()
		{
			return mmal_connection_enable(Ptr) == MMAL_SUCCESS;
		}

		bool Disable()
		{
			return mmal_connection_disable(Ptr) == MMAL_SUCCESS;
		}
	};

	class Component
	{
		MMAL_COMPONENT_T* Ptr = nullptr;
	public:
		Component(const char* name)
		{
			if (mmal_component_create(name, &Ptr) != MMAL_SUCCESS)
				throw std::runtime_error("Failed to create component");
		}

		~Component()
		{
			if (Ptr)
			{
				mmal_component_destroy(Ptr);
			}
		}

		std::shared_ptr<Port> GetInputPort(size_t port) const
		{
			if (port < Ptr->input_num)
				return std::make_shared<Port>(Ptr->input[port]);

			return nullptr;
		}

		std::shared_ptr<Port> GetOutputPort(size_t port) const
		{
			if (port < Ptr->output_num)
				return std::make_shared<Port>(Ptr->output[port]);

			return nullptr;
		}

		std::shared_ptr<Port> GetControlPort() const
		{
			return std::make_shared<Port>(Ptr->control);
		}

		bool Enable()
		{
			return mmal_component_enable(Ptr) == MMAL_SUCCESS;
		}

		bool Disable()
		{
			return mmal_component_disable(Ptr) == MMAL_SUCCESS;
		}
	};
}

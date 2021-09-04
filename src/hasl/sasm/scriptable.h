#pragma once
#include "pch.h"

namespace hasl::sasm
{
	class scriptable
	{
	public:
		HASL_DCM(scriptable);
	public:
		virtual v_t get_dims() const = 0;
		void set_pos(const v_t& pos)
		{
			m_pos = pos;
		}
		void set_vel(const v_t& vel)
		{
			m_pos = vel;
			m_pos.clamp(0.f, m_speed);
		}
		void set_state(const std::string& state)
		{
			m_state = state;
			validate();
		}
		const v_t& get_pos() const
		{
			return m_pos;
		}
		const v_t& get_vel() const
		{
			return m_vel;
		}
		float get_speed() const
		{
			return m_speed;
		}
	protected:
		std::string m_state;
		v_t m_pos, m_vel;
		float m_speed;
	protected:
		scriptable(const std::unordered_map<std::string, void*>& states, const std::string& state) :
			m_states(states),
			m_state(state)
		{
			validate();
		}
	protected:
		template<typename T>
		const T* const get_state() const
		{
			return HASL_CAST(T*, m_states[m_state]);
		}
		template<typename T>
		T* const get_state()
		{
			return HASL_CAST(T*, m_states[m_state]);
		}
	private:
		std::unordered_map<std::string, void*> m_states;
	private:
		void validate() const
		{
			HASL_ASSERT(m_states.contains(m_state), "Invalid hasl::sasm::scriptable state");
		}
	};
}


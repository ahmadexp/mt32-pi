//
// power.cpp
//
// mt32-pi - A baremetal MIDI synthesizer for Raspberry Pi
// Copyright (C) 2020-2023 Dale Whinham <daleyo@gmail.com>
//
// This file is part of mt32-pi.
//
// mt32-pi is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// mt32-pi is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// mt32-pi. If not, see <http://www.gnu.org/licenses/>.
//

#include <circle/bcmpropertytags.h>
#include <circle/cputhrottle.h>
#include <circle/logger.h>
#include <circle/timer.h>

#include "power.h"

LOGMODULE("power");

// Bits in the throttled status response
constexpr u32 UnderVoltageOccurredBit = 1 << 16;
constexpr u32 ThrottlingOccurredBit   = 1 << 18;
constexpr unsigned int ThrottledStatusUpdateInterval = 4 * HZ;

CPower::CPower()
	: m_nPowerSaveTimeout(300),
	  m_nLastActivityTime(0),
	  m_nLastThrottledStatusTime(0),
	  m_State(TState::Normal),
	  m_LastThrottledStatus(0)
{
}

void CPower::Update()
{
	unsigned int nTicks = CTimer::Get()->GetTicks();

	// Begin the power-saving transition. The derived class may need time to
	// quiesce peripherals and other CPU cores before the clock can be changed.
	if (m_State == TState::Normal && m_nPowerSaveTimeout && (nTicks - m_nLastActivityTime) >= m_nPowerSaveTimeout * HZ)
	{
		m_State = TState::EnteringPowerSaving;
		OnEnterPowerSavingMode();
	}

	// Only reduce the clock once all users of clock-sensitive peripherals have
	// acknowledged the transition.
	if (m_State == TState::EnteringPowerSaving && IsReadyForPowerSavingMode())
	{
		CCPUThrottle::Get()->SetSpeed(TCPUSpeed::CPUSpeedLow);
		m_State = TState::PowerSaving;
	}

	// Firmware property tags are relatively expensive. Circle checks these at
	// the same cadence in CCPUThrottle::Update().
	if ((nTicks - m_nLastThrottledStatusTime) >= ThrottledStatusUpdateInterval)
	{
		UpdateThrottledStatus();
		m_nLastThrottledStatusTime = nTicks;
	}
}

void CPower::Awaken()
{
	m_nLastActivityTime = CTimer::Get()->GetTicks();

	if (m_State == TState::Normal)
		return;

	// The clock is still at maximum while the system is only entering power
	// saving mode.
	if (m_State == TState::PowerSaving)
		CCPUThrottle::Get()->SetSpeed(TCPUSpeed::CPUSpeedMaximum);

	m_State = TState::Normal;

	OnExitPowerSavingMode();
}

void CPower::OnEnterPowerSavingMode()
{
	LOGNOTE("Entering power saving mode");
}

void CPower::OnExitPowerSavingMode()
{
	LOGNOTE("Leaving power saving mode");
}

bool CPower::IsReadyForPowerSavingMode() const
{
	return true;
}

void CPower::OnThrottleDetected()
{
	LOGWARN("CPU throttling by firmware detected; check power supply/cooling");
}

void CPower::OnUnderVoltageDetected()
{
	LOGWARN("Undervoltage detected; check power supply");
}

void CPower::UpdateThrottledStatus()
{
	// Get throttled status from the firmware and clear status bits
	TPropertyTagSimple ThrottledStatus;
	ThrottledStatus.nValue = 0xFFFF;
	if (!m_Tags.GetTag(PROPTAG_GET_THROTTLED, &ThrottledStatus, sizeof(ThrottledStatus), sizeof(ThrottledStatus.nValue)))
		return;

	bool bNewVal = ThrottledStatus.nValue & ThrottlingOccurredBit;
	bool bOldVal = m_LastThrottledStatus & ThrottlingOccurredBit;

	if (bNewVal && bOldVal != bNewVal)
		OnThrottleDetected();

	bNewVal = ThrottledStatus.nValue & UnderVoltageOccurredBit;
	bOldVal = m_LastThrottledStatus & UnderVoltageOccurredBit;

	if (bNewVal && bOldVal != bNewVal)
		OnUnderVoltageDetected();

	m_LastThrottledStatus = ThrottledStatus.nValue;
}

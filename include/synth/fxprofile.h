//
// fxprofile.h
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

#ifndef _fxprofile_h
#define _fxprofile_h

#include <optional>

struct TFXProfile
{
	std::optional<float> nGain;

	std::optional<bool> bReverbActive;
	std::optional<float> nReverbDamping;
	std::optional<float> nReverbLevel;
	std::optional<float> nReverbRoomSize;
	std::optional<float> nReverbWidth;

	std::optional<bool> bChorusActive;
	std::optional<float> nChorusDepth;
	std::optional<float> nChorusLevel;
	std::optional<int> nChorusVoices;
	std::optional<float> nChorusSpeed;
};

#endif

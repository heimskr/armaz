#pragma once

//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

namespace Armaz {
	constexpr unsigned SETWAY_LEVEL_SHIFT = 1;

	constexpr unsigned L1_DATA_CACHE_SETS        = 256;
	constexpr unsigned L1_DATA_CACHE_WAYS        =   2;
	constexpr unsigned L1_SETWAY_WAY_SHIFT       =  31; // 32-Log2(L1_DATA_CACHE_WAYS)
	constexpr unsigned L1_DATA_CACHE_LINE_LENGTH =  64;
	constexpr unsigned L1_SETWAY_SET_SHIFT       =   6; // Log2(L1_DATA_CACHE_LINE_LENGTH)

	constexpr unsigned L2_CACHE_SETS        = 1024;
	constexpr unsigned L2_CACHE_WAYS        =   16;
	constexpr unsigned L2_SETWAY_WAY_SHIFT  =   28; // 32-Log2(L2_CACHE_WAYS)
	constexpr unsigned L2_CACHE_LINE_LENGTH =   64;
	constexpr unsigned L2_SETWAY_SET_SHIFT  =    6; // Log2(L2_CACHE_LINE_LENGTH)

	void dataSyncBarrier();
	void cleanDataCache();
	void invalidateInstructionCache();
	void instructionSyncBarrier();
	void syncDataAndInstructionCache();
}

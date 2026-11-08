#pragma once

#include <array>
#include <cstddef>
#include <istream>
#include <new>
#include <string>
#include <utility>

namespace ModernStaggerLock
{
	inline constexpr std::size_t kMaxConfigFileBytes = 4 * 1024 * 1024;
	inline constexpr std::size_t kMaxConfigTotalBytes = 16 * 1024 * 1024;

	inline bool ReadConfigText(std::istream& a_input, std::string& a_text, std::size_t a_maxBytes = kMaxConfigFileBytes) noexcept
	{
		try {
			std::string text;
			std::array<char, 4096> buffer;
			for (;;) {
				const auto remaining = a_maxBytes - text.size();
				const auto requested = remaining < buffer.size() ? remaining + 1 : buffer.size();
				a_input.read(buffer.data(), static_cast<std::streamsize>(requested));
				const auto count = static_cast<std::size_t>(a_input.gcount());
				if (count > remaining || a_input.bad() || (a_input.fail() && !a_input.eof())) {
					return false;
				}
				text.append(buffer.data(), count);
				if (a_input.eof()) {
					a_text = std::move(text);
					return true;
				}
			}
		} catch (const std::bad_alloc&) {
			return false;
		} catch (const std::ios_base::failure&) {
			return false;
		}
	}
}

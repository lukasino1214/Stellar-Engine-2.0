#pragma once

#include "types.hpp"

namespace Stellar {
	class UUID {
	public:
		UUID();
		UUID(u64 _uuid);
		UUID(const UUID&) = default;

		operator u64() const { return uuid; }
		u64 uuid;
	};
}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Stellar::UUID> {
		auto operator()(const Stellar::UUID& uuid) const -> std::size_t {
			return *reinterpret_cast<const Stellar::u64*>(&uuid);
		}
	};

}
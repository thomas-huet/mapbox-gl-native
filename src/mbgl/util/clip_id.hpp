#pragma once

#include <bitset>
#include <forward_list>
#include <iosfwd>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace mbgl {

struct ClipID {
    ClipID() {
    }
    ClipID(const std::string& mask_, const std::string& reference_)
        : mask(mask_), reference(reference_) {
    }

    std::bitset<8> mask;
    std::bitset<8> reference;

    bool operator==(const ClipID& other) const {
        return mask == other.mask && reference == other.reference;
    }

    ClipID& operator|=(const ClipID& other) {
        mask |= other.mask;
        reference |= other.reference;
        return *this;
    }
};

::std::ostream& operator<<(::std::ostream& os, const ClipID& rhs);

} // namespace mbgl

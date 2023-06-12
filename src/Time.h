
namespace kev {

class Duration {
    long value = 0;
public:
    constexpr Duration(long value) : value {value} {}
    constexpr long unsafeGetValue() const { return value; }

    friend constexpr bool operator< (const Duration&, const Duration&);
    friend constexpr bool operator==(const Duration&, const Duration&);
};

inline constexpr bool operator==(const Duration& lhs, const Duration& rhs){return lhs.value == rhs.value;}
inline constexpr bool operator!=(const Duration& lhs, const Duration& rhs){return !operator==(lhs,rhs);}
inline constexpr bool operator< (const Duration& lhs, const Duration& rhs){return lhs.value < rhs.value;}
inline constexpr bool operator> (const Duration& lhs, const Duration& rhs){return  operator< (rhs,lhs);}
inline constexpr bool operator<=(const Duration& lhs, const Duration& rhs){return !operator> (lhs,rhs);}
inline constexpr bool operator>=(const Duration& lhs, const Duration& rhs){return !operator< (lhs,rhs);}
inline constexpr Duration operator+(const Duration& lhs, const Duration& rhs) {
  return Duration{lhs.unsafeGetValue() + rhs.unsafeGetValue()};
}

namespace literals {

inline constexpr auto operator""_ms(unsigned long long t) -> Duration { return Duration{static_cast<long>(t)}; }
inline constexpr auto operator""_s(unsigned long long t) -> Duration { return Duration{static_cast<long>(t * 1000ul)}; }
inline constexpr auto operator""_min(unsigned long long t) -> Duration { return Duration{static_cast<long>(t * 60ul * 1000ul)}; }

} // namespace literals


class Timestamp {
    unsigned long value = 0;
public:
    constexpr Timestamp() = default;
    constexpr Timestamp(unsigned long value) : value{value} {}
    friend constexpr Duration operator-(const Timestamp&, const Timestamp&);
    friend constexpr Timestamp operator+(const Timestamp&, const Duration&);

    constexpr explicit operator bool() { return value != 0ul; }
};

constexpr
Duration operator-(const Timestamp& lhs, const Timestamp& rhs) {
    return {static_cast<long>(lhs.value - rhs.value)};
}

constexpr
Timestamp operator+(const Timestamp& stamp, const Duration& dur) {
    return {stamp.value + static_cast<unsigned long>(dur.unsafeGetValue())};
}

}

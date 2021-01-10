
namespace kev {

class Duration {
    long value = 0;
public:
    Duration(long value) : value {value} {}
    long unsafeGetValue() const { return value; }

    friend bool operator< (const Duration&, const Duration&);
    friend bool operator==(const Duration&, const Duration&);
};

inline bool operator==(const Duration& lhs, const Duration& rhs){return lhs.value == rhs.value;}
inline bool operator!=(const Duration& lhs, const Duration& rhs){return !operator==(lhs,rhs);}
inline bool operator< (const Duration& lhs, const Duration& rhs){return lhs.value < rhs.value;}
inline bool operator> (const Duration& lhs, const Duration& rhs){return  operator< (rhs,lhs);}
inline bool operator<=(const Duration& lhs, const Duration& rhs){return !operator> (lhs,rhs);}
inline bool operator>=(const Duration& lhs, const Duration& rhs){return !operator< (lhs,rhs);}


class Timestamp {
    unsigned long value = 0;
public:
    Timestamp() = default;
    Timestamp(unsigned long value) : value{value} {}
    friend Duration operator-(const Timestamp&, const Timestamp&);
    friend Timestamp operator+(const Timestamp&, const Duration&);

    explicit operator bool() { return value != 0ul; }
};

Duration operator-(const Timestamp& lhs, const Timestamp& rhs) {
    return {static_cast<long>(lhs.value - rhs.value)};
}

Timestamp operator+(const Timestamp& stamp, const Duration& dur) {
    return {stamp.value + static_cast<unsigned long>(dur.unsafeGetValue())};
}

}

#pragma once
#include "Helpers/StringHelpers.hpp"
#include <cmath>
#include <cstdint>
#include <string>
#include <Eigen/Core>
#include <boost/multiprecision/cpp_int.hpp>
#include <cereal/types/vector.hpp>

namespace FixedPoint { 
    template <typename T> T abs(const T& a) {
        if(a < T(0))
            return -a;
        return a;
    }

    template <typename T> T sqrt(const T& a) {
        return T(boost::multiprecision::sqrt(a.get_underlying_val() << a.bit_fraction_count()), true);
    }

    template <typename T> T lerp(const T& a, const T& b, const T& t) {
        return a + t * (b - a);
    }

    template <typename T> T lerp_double(const T& a, const T& b, double t) {
        if(t == 0.0)
            return a;
        else
            return a + (b - a) / T(1.0 / t);
    }

    template <typename T> T trunc(const T& a) {
        return (a >> a.bit_fraction_count()) << a.bit_fraction_count();
    }

    template <typename T> T negative_round(const T& a) {
        T truncatedVal = FixedPoint::trunc(a);
        if(truncatedVal != a && a < T(0))
            return truncatedVal + T(1);
        return truncatedVal;
    }

    // Gets highest bit in UNDERLYING VALUE, not the integer portion of the number
    template <typename T> int to_highest_bit(const T& a) {
        typename T::UnderlyingType underNum = a.get_underlying_val();
        if(underNum == 0)
            return 0;

        int b = 0;
        while(underNum >>= 1)
            b++;

        return b;
    }

    template <typename T> T log2_int(const T& a) {
        typename T::UnderlyingType b(typename T::UnderlyingType(1u) << (a.bit_fraction_count() - 1));
        typename T::UnderlyingType y(0);
        typename T::UnderlyingType x = a.get_underlying_val();

        if(a < T(0))
            throw std::runtime_error("[FixedPoint::log2] Number outside domain");

        while(x < (typename T::UnderlyingType(1u) << a.bit_fraction_count()))
        {
            x <<= 1;
            y -= (typename T::UnderlyingType(1u)) << a.bit_fraction_count();
        }

        while(x >= (typename T::UnderlyingType(2u) << a.bit_fraction_count()))
        {
            x >>= 1;
            y += (typename T::UnderlyingType(1u)) << a.bit_fraction_count();
        }

        return T(y, true);
    }

    template <typename T> T exp2_int(T x) {
        if(x < T(0))
            return T(1);

        typename T::UnderlyingType x_int = x.get_underlying_val() >> x.bit_fraction_count();
        x -= T(x_int, false);
        assert(x >= T(0) && x < T(1));
    
        return T(typename T::UnderlyingType(1) << static_cast<int>(x_int), false);
    }

    // Base should be a power of 2 to be accurate
    template <typename T> T log_int(const T& a, const T& base) {
        return log2_int(a) / log2_int(base);
    }

    // Base should be a power of 2 to be accurate
    template <typename T> T exp_int(const T& x, const T& base) {
        return exp2_int(x * log2_int(base));
    }

    template <typename T> T exp_int_accurate(uint64_t x, uint64_t base) {
        T toRet(1);
        T tBase(base);
        for(uint64_t i = 0; i < x; i++)
            toRet *= tBase;
        return toRet;
    }

    // https://github.com/Koishi-Satori/EirinFixed/blob/main/include/eirin/math.hpp
    template <typename T> T log2(const T& a) {
        typename T::UnderlyingType b(typename T::UnderlyingType(1u) << (a.bit_fraction_count() - 1));
        typename T::UnderlyingType y(0);
        typename T::UnderlyingType x = a.get_underlying_val();

        if(a < T(0))
            throw std::runtime_error("[FixedPoint::log2] Number outside domain");

        while(x < (typename T::UnderlyingType(1u) << a.bit_fraction_count()))
        {
            x <<= 1;
            y -= (typename T::UnderlyingType(1u)) << a.bit_fraction_count();
        }

        while(x >= (typename T::UnderlyingType(2u) << a.bit_fraction_count()))
        {
            x >>= 1;
            y += (typename T::UnderlyingType(1u)) << a.bit_fraction_count();
        }

        typename T::UnderlyingType z = x;
        for(int i = 0; i < a.bit_fraction_count(); ++i)
        {
            z = (z * z) >> a.bit_fraction_count();
            if(z >= (typename T::UnderlyingType(2u) << a.bit_fraction_count()))
            {
                z >>= 1;
                y += b;
            }
            b = b >> 1;
        }

        return T(y, true);
    }

    // https://github.com/MikeLankamp/fpm/blob/master/include/fpm/math.hpp
    template <typename T> T exp2(T x) {
        if(x < T(0))
            return T(1) / exp2(-x);

        typename T::UnderlyingType x_int = x.get_underlying_val() >> x.bit_fraction_count();
        x -= T(x_int, false);
        assert(x >= T(0) && x < T(1));
    
        T fA(1.8964611454333148e-3);
        T fB(8.9428289841091295e-3);
        T fC(5.5866246304520701e-2);
        T fD(2.4013971109076949e-1);
        T fE(6.9315475247516736e-1);
        T fF(9.9999989311082668e-1);
        return T(typename T::UnderlyingType(1) << static_cast<int>(x_int), false) * (((((fA * x + fB) * x + fC) * x + fD) * x + fE) * x + fF);
    }

    template <typename T> T log(const T& a, const T& base) {
        return log2(a) / log2(base);
    }

    template <typename T> T exp(const T& x, const T& base) {
        return exp2(x * log2(base));
    }

    template <typename T, int bitFractionCount> class Number {
        public:
            typedef T UnderlyingType;

            static int bit_fraction_count() {
                return bitFractionCount;
            }
    
            Number():
                val(0) {}
    
            explicit Number(double initVal) {
                val = T(initVal * std::pow(2, bitFractionCount));
            }
    
            explicit Number(float initVal) {
                val = T(initVal * std::pow(2, bitFractionCount));
            }
    
            explicit Number(int64_t initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            explicit Number(int32_t initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            explicit Number(int16_t initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            explicit Number(uint64_t initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            explicit Number(uint32_t initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            explicit Number(uint16_t initVal) {
                val = T(initVal) << bitFractionCount;
            }

            explicit Number(const std::string& initVal) {
                val = T(initVal) << bitFractionCount;
            }
    
            template <typename Archive> void load(Archive& a) {
                bool isNegative;
                std::vector<uint8_t> v;
                a(isNegative, v);
                boost::multiprecision::import_bits(val, v.begin(), v.end(), 8, true);
                if(isNegative)
                    val = -val;
            }
    
            template <typename Archive> void save(Archive& a) const {
                bool isNegative = val < 0;
                std::vector<uint8_t> v;
                T absVal = boost::multiprecision::abs(val);
                boost::multiprecision::export_bits(absVal, std::back_inserter(v), 8, true);
                a(isNegative, v);
            }
    
            Number operator-(const Number& other) const {
                return Number(val - other.val, true);
            }
    
            Number operator-() const {
                return Number(-val, true);
            }
    
            Number operator/(const Number& other) const {
                return Number((val << bitFractionCount) / other.val, true);
            }
    
            Number operator*(const Number& other) const {
                return Number((val * other.val) >> bitFractionCount, true);
            }
    
            template <int otherDecimalCount> Number multiply_different_precision(const Number<T, otherDecimalCount>& other) const {
                return Number(((val * other.get_underlying_val()) >> other.bit_fraction_count()), true);
            }

            Number multiply_double(double a) const {
                if(std::fabs(a) < 1.0)
                    a = 1.0 / a;
                return (*this) * Number(a);
            }

            Number divide_double(double a) const {
                if(std::fabs(a) < 1.0)
                    a = 1.0 / a;
                return (*this) / Number(a);
            }
    
            const T& get_underlying_val() const {
                return val;
            }
    
            Number operator+(const Number& other) const {
                return Number(val + other.val, true);
            }
    
            Number operator%(const Number& other) const {
                return Number(val % other.val, true);
            }
    
            Number& operator+=(const Number& other) {
                val += other.val;
                return *this;
            }
    
            Number& operator-=(const Number& other) {
                val -= other.val;
                return *this;
            }
    
            Number& operator*=(const Number& other) {
                val *= other.val;
                val >>= bitFractionCount;
                return *this;
            }
    
            Number& operator/=(const Number& other) {
                val <<= bitFractionCount;
                val /= other.val;
                return *this;
            }
    
            Number& operator%=(const Number& other) {
                val %= other.val;
                return *this;
            }
    
            explicit operator float() const {
                return static_cast<float>(val) / std::pow(2, bitFractionCount);
            }
    
            explicit operator double() const {
                return static_cast<double>(val) / std::pow(2, bitFractionCount);
            }
    
            explicit operator int64_t() const {
                return static_cast<int64_t>(val >> bitFractionCount);
            }
    
            explicit operator uint64_t() const {
                return static_cast<uint64_t>(val >> bitFractionCount);
            }
    
            std::string to_underlying_str() const {
                return val.str();
            }
    
            void from_underlying_str(const std::string& str) {
                val = T(str);
            }
    
            std::string display_int_str(size_t scientificNotationDigits, bool fancy = false, std::string* exponentStr = nullptr) const {
                if(scientificNotationDigits <= 0) {
                    T shiftedVal = val >> bitFractionCount;
                    return shiftedVal.str();
                }
                else {
                    T shiftedVal = val >> bitFractionCount;
                    std::string a = shiftedVal.str();
                    if(a.size() > (scientificNotationDigits + 2)) {
                        bool isNegative = val < 0;
                        size_t exponent = a.size() - (isNegative ? 2 : 1);
                        a = a.substr(0, scientificNotationDigits + (isNegative ? 2 : 1));
                        a.insert(a.begin() + (isNegative ? 2 : 1), '.');
                        for(;;) {
                            if(a.back() == '0') {
                                a.pop_back();
                                continue;
                            }
                            else if(a.back() == '.')
                                a.pop_back();
                            break;
                        }
                        if(exponentStr)
                            *exponentStr = std::to_string(exponent);
                        else if(fancy) {
                            a += "×10";
                            std::string expStr = std::to_string(exponent);
                            for(char eChar : expStr) {
                                switch(eChar) {
                                    case '0':
                                        a += "⁰";
                                        break;
                                    case '1':
                                        a += "¹";
                                        break;
                                    case '2':
                                        a += "²";
                                        break;
                                    case '3':
                                        a += "³";
                                        break;
                                    case '4':
                                        a += "⁴";
                                        break;
                                    case '5':
                                        a += "⁵";
                                        break;
                                    case '6':
                                        a += "⁶";
                                        break;
                                    case '7':
                                        a += "⁷";
                                        break;
                                    case '8':
                                        a += "⁸";
                                        break;
                                    case '9':
                                        a += "⁹";
                                        break;
                                    default:
                                        a.push_back(eChar);
                                        break;
                                }
                            }
                        }
                        else
                            a += "e" + std::to_string(exponent);
                    }
                    return a;
                }
            }
    
            friend std::ostream& operator<<(std::ostream& os, Number const& m) {
                return os << static_cast<double>(m);
            }
    
            bool operator<(const Number& other) const {
                return val < other.val;
            }
    
            bool operator>(const Number& other) const {
                return val > other.val;
            }
    
            bool operator>=(const Number& other) const {
                return val >= other.val;
            }
    
            bool operator<=(const Number& other) const {
                return val <= other.val;
            }
    
            bool operator==(const Number& other) const {
                return val == other.val;
            }

            Number operator>>(int shiftNum) const {
                return Number(val >> shiftNum, true);
            }

            Number operator<<(int shiftNum) const {
                return Number(val << shiftNum, true);
            }

            Number(const T& initVal, bool isUnderlyingVal):
                val(initVal)
            {
                if(isUnderlyingVal)
                    val = initVal;
                else
                    val = initVal << bitFractionCount;
            }
        
        private:
            T val;
    };

    template <typename T, int bitFractionCount> class Multiplier {
        public:
            typedef Number<T, bitFractionCount> UnderlyingNumber;

            Multiplier():
                val(1),
                isReciprocal(false)
            {}

            Multiplier(const UnderlyingNumber& initVal):
                val(initVal),
                isReciprocal(false)
            {
                if(val != UnderlyingNumber(0) && abs(val) < UnderlyingNumber(1)) {
                    val = UnderlyingNumber(1) / val;
                    isReciprocal = true;
                }
            }

            Multiplier(const UnderlyingNumber& initVal, bool isReciprocal):
                val(initVal),
                isReciprocal(isReciprocal)
            {
                if(abs(val) == UnderlyingNumber(1))
                    isReciprocal = false;
            }
    
            explicit Multiplier(double initVal) {
                if(std::abs(initVal) < 1.0) {
                    val = UnderlyingNumber(1.0 / initVal);
                    isReciprocal = true;
                }
                else {
                    val = UnderlyingNumber(initVal);
                    isReciprocal = false;
                }
            }
    
            explicit Multiplier(float initVal) {
                if(std::abs(initVal) < 1.0) {
                    val = UnderlyingNumber(1.0 / initVal);
                    isReciprocal = true;
                }
                else {
                    val = UnderlyingNumber(initVal);
                    isReciprocal = false;
                }
            }
    
            explicit Multiplier(int64_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit Multiplier(int32_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit Multiplier(int16_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit Multiplier(uint64_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit Multiplier(uint32_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit Multiplier(uint16_t initVal) {
                val = UnderlyingNumber(initVal);
                isReciprocal = false;
            }
    
            explicit operator float() const {
                if(isReciprocal && val != UnderlyingNumber(0))
                    return 1.0f / static_cast<float>(val);
                else
                    return static_cast<float>(val);
            }
    
            explicit operator double() const {
                if(isReciprocal && val != UnderlyingNumber(0))
                    return 1.0 / static_cast<double>(val);
                else
                    return static_cast<double>(val);
            }
    
            explicit operator int64_t() const {
                if(isReciprocal)
                    return 0;
                else
                    return static_cast<int64_t>(val);
            }
    
            explicit operator uint64_t() const {
                if(isReciprocal)
                    return 0;
                else
                    return static_cast<uint64_t>(val);
            }

            explicit operator UnderlyingNumber() const {
                if(isReciprocal && val != UnderlyingNumber(0))
                    return UnderlyingNumber(1) / val;
                else
                    return val;
            }

            Multiplier multiply_double(double a) const {
                return (*this) * Multiplier(a);
            }

            Multiplier divide_double(double a) const {
                return (*this) / Multiplier(a);
            }

            Multiplier operator*(const Multiplier& a) const {
                if(a.val == UnderlyingNumber(0))
                    return Multiplier(0);
                return (*this) / Multiplier(a.val, !a.isReciprocal);
            }

            Multiplier operator/(const Multiplier& a) const {
                if(val == UnderlyingNumber(0))
                    return Multiplier(0);

                if(!isReciprocal && !a.isReciprocal) {
                    if(abs(val) < abs(a.val))
                        return Multiplier(a.val / val, true);
                    else
                        return Multiplier(val / a.val, false);
                }
                else if(!isReciprocal && a.isReciprocal) {
                    return Multiplier(val * a.val, false);
                }
                else if(isReciprocal && !a.isReciprocal) {
                    return Multiplier(val * a.val, true);
                }
                else {
                    if(abs(val) < abs(a.val))
                        return Multiplier(a.val / val, false);
                    else
                        return Multiplier(val / a.val, true);
                }
            }

            bool operator==(const Multiplier& other) const {
                return (val == other.val) && (isReciprocal == other.isReciprocal || val == UnderlyingNumber(1));
            }

            bool operator!=(const Multiplier& other) const {
                return !((*this) == other);
            }
        private:
            Number<T, bitFractionCount> val;
            bool isReciprocal;
    };

    template <typename Mult> Mult::UnderlyingNumber operator*(const Mult& a, const typename Mult::UnderlyingNumber& b) {
        return static_cast<Mult::UnderlyingNumber>(a * Multiplier(b));
    }

    template <typename Mult> Mult::UnderlyingNumber operator*(const typename Mult::UnderlyingNumber& b, const Mult& a) {
        return static_cast<Mult::UnderlyingNumber>(a * Multiplier(b));
    }

    template <typename Mult> Mult::UnderlyingNumber operator/(const Mult& a, const typename Mult::UnderlyingNumber& b) {
        return static_cast<Mult::UnderlyingNumber>(a / Multiplier(b));
    }

    template <typename Mult> Mult::UnderlyingNumber operator/(const typename Mult::UnderlyingNumber& a, const Mult& b) {
        return static_cast<Mult::UnderlyingNumber>(Multiplier(a) / b);
    }
}

typedef FixedPoint::Number<boost::multiprecision::cpp_int, 32> WorldScalar;
typedef FixedPoint::Multiplier<boost::multiprecision::cpp_int, 32> WorldMultiplier;

namespace Eigen {
  template<> struct NumTraits<WorldScalar> : GenericNumTraits<WorldScalar>
  {
    typedef WorldScalar Real;
    typedef WorldScalar NonInteger;
    typedef WorldScalar Nested;
 
    static inline Real epsilon() { return Real(0); }
    static inline Real dummy_precision() { return Real(0); }
    static inline int digits10() { return 0; }
 
    enum {
      IsInteger = 0,
      IsSigned = 1,
      IsComplex = 0,
      RequireInitialization = 1,
      ReadCost = 6,
      AddCost = 150,
      MulCost = 100
    };
  };
}

namespace FixedPoint {
    template <typename Mult> Eigen::Vector<typename Mult::UnderlyingNumber, 2> multiplier_vec_mult(const Eigen::Vector<typename Mult::UnderlyingNumber, 2>& a, const Mult& m) {
        auto toRet = a;
        toRet.x() = toRet.x() * m;
        toRet.y() = toRet.y() * m;
        return toRet;
    }
    
    template <typename Mult> Eigen::Vector<typename Mult::UnderlyingNumber, 2> multiplier_vec_div(const Eigen::Vector<typename Mult::UnderlyingNumber, 2>& a, const Mult& m) {
        auto toRet = a;
        toRet.x() = toRet.x() / m;
        toRet.y() = toRet.y() / m;
        return toRet;
    }
}

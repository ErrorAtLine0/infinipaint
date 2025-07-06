#pragma once
#include <Eigen/Dense>
#include <cereal/archives/portable_binary.hpp>
#include <nlohmann/json.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include "FixedPoint.hpp"

#include <include/core/SkColor.h>

namespace cereal
{
    // https://stackoverflow.com/questions/22884216/serializing-eigenmatrix-using-cereal-library (with some modifications using constexpr)
    template <class Archive, class Derived> inline
    typename std::enable_if<traits::is_output_serializable<BinaryData<typename Derived::Scalar>, Archive>::value, void>::type
    save(Archive& ar, Eigen::PlainObjectBase<Derived> const & m){
        if constexpr(std::is_same_v<typename Derived::Scalar, WorldScalar>) {
            for(int i = 0; i < m.rows(); i++)
                ar(m.data()[i]);
        }
        else {
            typedef Eigen::PlainObjectBase<Derived> ArrT;
            if constexpr(ArrT::RowsAtCompileTime==Eigen::Dynamic)
                ar(static_cast<uint32_t>(m.rows()));
            if constexpr(ArrT::ColsAtCompileTime==Eigen::Dynamic)
                ar(static_cast<uint32_t>(m.cols()));
            ar(binary_data(m.data(),static_cast<uint64_t>(m.rows()*m.cols()*sizeof(typename Derived::Scalar))));
        }
    }
    
    template <class Archive, class Derived> inline
    typename std::enable_if<traits::is_input_serializable<BinaryData<typename Derived::Scalar>, Archive>::value, void>::type
    load(Archive& ar, Eigen::PlainObjectBase<Derived>& m) {
        if constexpr(std::is_same_v<typename Derived::Scalar, WorldScalar>) {
            for(int i = 0; i < m.rows(); i++)
                ar(m.data()[i]);
        }
        else {
            typedef Eigen::PlainObjectBase<Derived> ArrT;
            const Eigen::Index rows=ArrT::RowsAtCompileTime, cols=ArrT::ColsAtCompileTime;
            uint32_t r = rows;
            uint32_t c = cols;

            if constexpr(rows==Eigen::Dynamic)
                ar(r);
            if constexpr(cols==Eigen::Dynamic)
                ar(c);
            if constexpr(cols==Eigen::Dynamic || rows == Eigen::Dynamic)
                m.resize(r, c);
            ar(binary_data(m.data(),static_cast<uint64_t>(m.rows()*m.cols()*sizeof(typename Derived::Scalar))));
        }
    }
}

namespace Eigen {
    // Some issue in nlohmann json prevents us from using Eigen::PlainObjectBase, so we have to specify the matrix class instead
    template<typename Scalar, int Rows, int Cols> void to_json(nlohmann::json& j, const Eigen::Matrix<Scalar, Rows, Cols>& m) {
        const Eigen::Index rows=m.RowsAtCompileTime, cols=m.ColsAtCompileTime;

        j = nlohmann::json::array();

        if(rows != Eigen::Dynamic && rows == 1) {
            for(Eigen::Index k = 0; k < m.cols(); k++)
                j.emplace_back(m(0, k));
        }
        else if(cols != Eigen::Dynamic && cols == 1) {
            for(Eigen::Index k = 0; k < m.rows(); k++)
                j.emplace_back(m(k, 0));
        }
        else {
            for(size_t i = 0; i < m.rows(); i++) {
                nlohmann::json a = nlohmann::json::array();
                for(Eigen::Index k = 0; k < m.cols(); k++) {
                    a.emplace_back(m(i, k));
                }
                j.emplace_back(a);
            }
        }
    }

    template<typename Scalar, int Rows, int Cols> void from_json(const nlohmann::json& j, Eigen::Matrix<Scalar, Rows, Cols>& m) {
        const Eigen::Index rows=m.RowsAtCompileTime, cols=m.ColsAtCompileTime;

        if(rows != Eigen::Dynamic && rows == 1) {
            m.resize(1, j.size());
            for(Eigen::Index k = 0; k < m.cols(); k++)
                m(0, k) = j[k];
        }
        else if(cols != Eigen::Dynamic && cols == 1) {
            m.resize(j.size(), 1);
            for(Eigen::Index k = 0; k < m.rows(); k++)
                m(k, 0) = j[k];
        }
        else {
            if(j.size() != 0 && j[0].size() != 0) {
                m.resize(j.size(), j[0].size());
                for(Eigen::Index i = 0; i < m.rows(); i++)
                    for(Eigen::Index k = 0; k < m.cols(); k++)
                        m(i, k) = j[i][k];
            }
            else
                m.resize(0, 0);
        }
    }
}

void to_json(nlohmann::json& j, const SkColor4f& d);
void from_json(const nlohmann::json& j, SkColor4f& d);

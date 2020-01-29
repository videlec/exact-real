/**********************************************************************
 *  This file is part of exact-real.
 *
 *        Copyright (C) 2019 Vincent Delecroix
 *        Copyright (C) 2019 Julian Rüth
 *
 *  exact-real is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  exact-real is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with exact-real. If not, see <https://www.gnu.org/licenses/>.
 *********************************************************************/

#include <gmpxx.h>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include "../exact-real/cereal.hpp"
#include "../exact-real/real_number.hpp"
#include "../exact-real/yap/arf.hpp"

#include "external/unique-factory/unique_factory.hpp"

using namespace exactreal;
using std::make_shared;
using std::ostream;
using std::shared_ptr;

namespace {

// An exact rational number
class RationalRealNumber final : public RealNumber {
 public:
  RationalRealNumber() : RationalRealNumber(0) {}
  explicit RationalRealNumber(const mpq_class& value) : value(value) {}

  virtual Arf arf(long prec) const override {
    return (Arf(value.get_num(), 0) / Arf(value.get_den(), 0))(prec, Arf::Round::NEAR);
  }

  explicit operator std::optional<mpq_class>() const override {
    return value;
  }

  RealNumber const& operator>>(ostream& out) const override {
    out << value;
    return *this;
  }

  shared_ptr<const RealNumber> operator*(const RealNumber& rhs) const override {
    if (value == 1) {
      return rhs.shared_from_this();
    }
    if (typeid(rhs) == typeid(*this)) {
      return RealNumber::rational(value * static_cast<const RationalRealNumber*>(&rhs)->value);
    }
    throw std::logic_error("not implemented - multiplication with non-trivial rational");
  }

  template <typename Archive>
  void save(Archive& archive) const {
    archive(cereal::make_nvp("value", CerealWrap{value}));
  }

 private:
  mpq_class value;
};

}  // namespace

namespace exactreal {
shared_ptr<const RealNumber> RealNumber::rational(const mpq_class& value) {
  static unique_factory::UniqueFactory<std::weak_ptr<RationalRealNumber>, mpq_class> factory;
  return factory.get(value, [&]() { return new RationalRealNumber(value); });
}

void save_rational(cereal::JSONOutputArchive& archive, const std::shared_ptr<const RealNumber>& base) {
  std::dynamic_pointer_cast<const RationalRealNumber>(base)->save(archive);
}

void load_rational(cereal::JSONInputArchive& archive, std::shared_ptr<const RealNumber>& base) {
  CerealWrap<mpq_class> value;
  archive(cereal::make_nvp("value", value));
  base = RealNumber::rational(*value);
}

}  // namespace exactreal

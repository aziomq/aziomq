/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef AZIOMQ_ERROR_HPP_
#define AZIOMQ_ERROR_HPP_

#include <boost/system/error_code.hpp>
#include <string>

namespace aziomq {
inline namespace v1 {
    /** \brief custom error_category to map zeromq errors */
    class error_category : public boost::system::error_category {
    public:
        virtual const char* name() const BOOST_SYSTEM_NOEXCEPT;
        virtual std::string message(int ev) const;
    };

    boost::system::error_code make_error_code(int ev = errno);
} // namesapce v1
} // namespace aziomq
#endif // AZIOMQ_ERROR_HPP_


/*
    Copyright (c) 2013-2014 Contributors as noted in the AUTHORS file

    This file is part of aziomq

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#include <sstream>
#include <stdexcept>

namespace boost {
    void assertion_failed(char const * expr, char const * function, char const * file, long line) {
        std::ostringstream stm;
        stm << "Assertion " << expr << " in " << function << "[" << file << ":" << line << "]";
        throw std::runtime_error(stm.str());
    }

    void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line) {
        std::ostringstream stm;
        stm << msg << " - " << expr << " in " << function << "[" << file << ":" << line << "]";
        throw std::runtime_error(stm.str());
    }
}


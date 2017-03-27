// Copyright (c) 2014-2017 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAOCPP_PEGTL_INCLUDE_TEST_VERIFY_RULE_HH
#define TAOCPP_PEGTL_INCLUDE_TEST_VERIFY_RULE_HH

#include <string>
#include <cstdlib>

#include "result_type.hh"
#include "verify_impl.hh"

namespace tao
{
   namespace TAOCPP_PEGTL_NAMESPACE
   {
      template< typename Rule >
      void verify_rule( const std::size_t line, const char * file, const std::string & data, const result_type result, const std::size_t remain )
      {
         verify_impl< Rule, lf_crlf_eol >( line, file, data, result, remain );
      }

   } // namespace TAOCPP_PEGTL_NAMESPACE

} // namespace tao

#endif
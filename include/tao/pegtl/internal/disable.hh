// Copyright (c) 2014-2017 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAOCPP_PEGTL_INCLUDE_INTERNAL_DISABLE_HH
#define TAOCPP_PEGTL_INCLUDE_INTERNAL_DISABLE_HH

#include "../config.hh"

#include "skip_control.hh"
#include "seq.hh"
#include "duseltronik.hh"

#include "../apply_mode.hh"
#include "../rewind_mode.hh"

#include "../analysis/generic.hh"

namespace tao
{
   namespace TAOCPP_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename ... Rules >
         struct disable
         {
            using analyze_t = analysis::generic< analysis::rule_type::SEQ, Rules ... >;

            template< apply_mode, rewind_mode M, template< typename ... > class Action, template< typename ... > class Control, typename Input, typename ... States >
            static bool match( Input & in, States && ... st )
            {
               return duseltronik< seq< Rules ... >, apply_mode::NOTHING, M, Action, Control >::match( in, st ... );
            }
         };

         template< typename ... Rules >
         struct skip_control< disable< Rules ... > > : std::true_type {};

      } // namespace internal

   } // namespace TAOCPP_PEGTL_NAMESPACE

} // namespace tao

#endif
# Actions and States

In its most simple form, a parsing run only returns whether (a portion of) the input matches the grammar.
To actually do something useful during a parsing run it is necessary to to attach (user-defined) *actions* to one or more grammar rules.

Actions are essentially functions that are called during the parsing run whenever the rule they are attached to successfully matched.
When an action is *applied*, the corresponding function receives the *states*, an arbitrary list of (user-defined) objects, as arguments.

## Contents

* [Overview](#overview)
* [Example](#example)
* [States](#states)
* [Apply](#apply)
* [Apply0](#apply0)
* [Inheriting](#inheriting)
* [Specialising](#specialising)
* [Changing Actions](#changing-actions)
  * [Via Rules](#via-rules)
  * [Via Actions](#via-actions)
* [Changing States](#changing-states)
  * [Via Rules](#via-rules-1)
  * [Via Actions](#via-actions-1)
* [Changing Actions and States](#changing-actions-and-states)
* [Match](#match)
* [Nothing](#nothing)
* [Troubleshooting](#troubleshooting)
  * [Boolean Return](#boolean-return)
  * [State Mismatch](#state-mismatch)
* [Legacy Actions](#legacy-actions)

## Overview

Actions are implemented as static member functions called `apply()` or `apply0()` of specialisations of custom class templates (which is not quite as difficult as it sounds).

States are additional function arguments to `tao::pegtl::parse()` that are forwarded to all actions, i.e. the `apply()` and `apply0()` static member functions from above.

To use actions during a parsing run they first need to be implemented.

* Define a custom action class template.
* Specialise the action class template for every rule for which a function is to be called and
  * either implement an `apply()` or `apply0()` static member function,
  * or derive from a class that implements the desired function.

Then the parsing run needs to be set up with the actions and any required states.

* Either pass the action class template as second template parameter to `tao::pegtl::parse()`,
* and/or (advanced) introduce the actions to a parsing run with one of the "[changing](#changing-actions)" techniques.
* Either pass the required state arguments as additional arguments to `tao::pegtl::parse()`,
* and/or (advanced) introduce the states to a parsing run with one of the "[changin](#changing-states)" techniques.

The very first step, defining a custom action class template, usually looks like this.

```c++
template< typename Rule >
struct actions
   : public tao::pegtl::nothing< Rule > {};
```

Instantiations of the primary template for `actions< Rule >` inherit from `tao::pegtl::nothing< Rule >` to indicate that neither `actions< Rule >::apply()` nor `actions< Rule >::apply0()` are to be called when `Rule` is successfully matched during a parsing run, or, in short, that no action is to be applied to `Rule`.

In order to manage the complexity in larger parsers and/or compose multiple grammars that each bring their own actions which in turn expect certain states, it can be useful to [change the actions](#changing-actions) [and/or the states](#changing-states) within a parsing run.

## Example

Here is a very short example that shows the basic way to put together a parsing run with actions.

```c++
// Define a simple grammar consisting of a single rule.
struct grammar
   : tao::pegtl::star< tao::pegtl::any > {};

// Specialise the action class template from earlier.
template<>
struct actions< tao::pegtl::any >
{
   // Implement an apply() function that will be called by
   // the PEGTL every time tao::pegtl::any matches during
   // the parsing run.
   template< typename Input >
   static void apply( const Input& in, std::string& out )
   {
      // Get the portion of the original input that the
      // rule matched this time as string and append it
      // to the result string.
      out += in.string();
   }
};

template< typename Input >
std::string as_string( Input& in )
{
   // Set up the states, here a single std::string as that is
   // what our action requires as additional function argument.
   std::string out;
   // Start the parsing run with our grammar, actions and state.
   tao::pegtl::parse< grammar, actions >( in, out );
   // Do something with the result.
   return out;
}
```

All together the `as_string()` function is a convoluted way of turning an [input](Inputs-and-Parsing.md) into a `std::string` byte-by-byte.

In the following we will take a more in-depth look at states and `apply()` and `apply0()` before diving into more advanced subjects.

## States

There is not much more to say about the states other than what has already been mentioned, namely that they are a list (colloquial list, not `std::list`) of objects that are

* passed by the user as additional arguments to [`tao::pegtl::parse()`](Inputs-and-Parsing.md#parse-function), and then

* passed by the PEGTL as additional arguments to all actions' `apply()` or `apply0()` static member functions.

The additional arguments to `apply()` and `apply0()` can be chosen freely, however all actions must accept the same list of states since they are all called with the same arguments by default.

States are not forwarded with "perfect forwarding" since r-value references don't make much sense when they will be used as action arguments many times.
The `parse()` function still uses universal references to bind to the state arguments in order to allow temporary objects.

## Apply

As seen above, the actual functions that are called when an action is applied are static member functions of the specialisations of the action class template nameed `apply()`.

```c++
template<>
struct actions< my_rule >
{
   template< typename Input >
   static void apply( const Input& in, /* all the states */ )
   {
      // Called whenever matching my_rule during a parsing run
      // succeeds (and actions are not disabled). The argument
      // named 'in' represents the matched part of the input.
      // Can also return bool instead of void.
   }
}
```

The first argument is not the input used in the parsing run, but rather a separate object of distinct type that represents the portion of the input that the rule to which the action is attached just matched. The remaining arguments to `apply()` are the current state arguments.

The exact type of the input class passed to `apply()` is not specified.
It is best practice to "template over" the type of the input as shown above.

Actions can then assume that the input provides (at least) the following interface.
The `Input` template parameter is set to the class of the input used as input in the parsing run at the point where the action is applied.

For illustrative purposes, we will assume that the input passed to `apply()` is of type `action_input`.
Any resemblance to real classes is not a coincidence, see `include/tao/pegtl/internal/action_input.hpp`.

```c++
template< typename Input >
class action_input
{
public:
   using input_t = Input;
   using iterator_t = typename Input::iterator_t;

   [[nodiscard]] bool empty() const noexcept;
   [[nodiscard]] std::size_t size() const noexcept;

   [[nodiscard]] const char* begin() const noexcept;  // Non-owning pointer!
   [[nodiscard]] const char* end() const noexcept;  // Non-owning pointer!

   [[nodiscard]] std::string string() const
   { return std::string( begin(), end() ); }

   [[nodiscard]] std::string_view string_view() const noexcept
   { return std::string_view( begin(), size() ); }

   [[nodiscard]] char peek_char( const std::size_t offset = 0 ) const noexcept;  // begin()[ offset ]
   [[nodiscard]] std::uint8_t peek_uint8( const std::size_t offset = 0 ) const noexcept;  // similar

   [[nodiscard]] pegtl::position position() const noexcept;  // Not efficient with lazy inputs.

   [[nodiscard]] const Input& input() const noexcept;
   [[nodiscard]] const iterator_t& iterator() const noexcept;
};
```

Note that `input()` returns the input from the parsing run which will be at the position after what has just been parsed, i.e. for an action input `ai` the assertion `ai.end() == ai.input().current()` will always hold true.
Conversely `iterator()` returns a pointer or iterator to the beginning of the action input's data, i.e. where the successful match attempt to the rule the action called with the action input is attached to started.

More importantly the `action_input` does **not** own the data it points to, it belongs to the original input used in the parsing run. Therefore **the validity of the pointed-to data might not extend (much) beyond the call to `apply()`**!

When the original input has tracking mode `eager`, the `iterator_t` returned by `action_input::iterator()` will contain the `byte`, `line` and `byte_in_line` counters corresponding to the beginning of the matched input represented by the `action_input`.

When the original input has tracking mode `lazy`, then `action_input::position()` is not efficient because it calculates the line number etc. by scanning the complete original input from the beginning

Actions often need to store and/or reference portions of the input for after the parsing run, for example when an abstract syntax tree is generated.
Some of the syntax tree nodes will contain portions of the input, for example for a variable name in a script language that needs to be stored in the syntax tree just as it occurs in the input data.

The **default safe choice** is to copy the matched portions of the input data that are passed to an action by storing a deep copy of the data as `std::string`, as obtained by the input class' `string()` member function, in the data structures built while parsing.

When the return type of an action, i.e. its `apply()`, is `bool`, it can retro-actively let the library consider the attempt to match the rule to which the action is attached a (local) failure.
For the overall parsing run, there is no difference between a rule returning `false` and an attached action returning `false`, however the action is only called when the rule returned `true`.
When an action returns `false`, the library rewinds the input to where it was when the rule to which the action was attached started its successful match.
This is unlike `match()` static member functions that have to rewind the input themselves.

## Apply0

In cases where the matched part of the input is not required, an action can implement a static member function called `apply0()` instead of `apply()`.
What changes is that `apply0()` will be called without an input as first argument, i.e. only with all the states.

```c++
template<>
struct actions< my_rule >
{
   static void apply0( /* all the states */ )
   {
      // Called whenever matching my_rule during a parsing run
      // succeeds (and actions are not disabled). Can also return
      // bool instead of void.
   }
}
```

Using `apply0()` is never necessary, it is "only" an optimisation with minor benefits at compile time, and potentially more noteworthy benefits at run time.
We recommend implementing `apply0()` over `apply()` whenever both are viable.

Though an infrequently used feature, `apply0()` can also return `bool` instead of `void`, just like `apply()` and with the same implications.

## Inheriting

We will use an example to show how to use existing actions via inheritance.
The grammar for this example consists of a couple of simple rules.

```c++
struct plain
   : tao::pegtl::utf8::range< 0x20, 0x10FFFF > {};

struct escaped
   : one< '\'', '"', '?', '\\', 'a', 'b', 'f', 'n', 'r', 't', 'v' > {};

struct character
   : tao::pegtl::if_must_else< one< '\\' >, escaped, plain > {};

struct text
   : tao::pegtl::must< tao::pegtl::star< character >, tao::pegtl::eof > {};
```

Our goal is for a parsing run with the `text` rule to produce a copy of the input where the backslash escape sequences are replaced by the character they represent.
When the `plain` rule matches, the bytes of the matched UTF-8-encoded code-point can be appended to the result.
When the `escaped` rule matches, the bytes corresponding to the character represented by the escape sequence must be appended to the result.
This can be achieved with appropriate specialisations of `actions` using some classes from `tao/pegtl/[contrib](Contrib-and-Examples.md#contrib)/unescape.hpp`.

```c++
template<>
struct actions< plain >
   : tao::pegtl::append_all {};

template<>
struct actions< escaped >
   : tao::pegtl::unescape_c< escaped_c, '\'', '"', '?', '\\', '\a', '\b', '\f', '\n', '\r', '\t', '\v' > {};
```

For step three the [input for the parsing run](Inputs-and-Parsing.md) is set up as usual.
In addition, the actions are passed as second template parameter, and a `std::string` as second argument to `parse()`.
Here `unescaped` is the state that is required by the `append_all` and `unescape_c` actions; all additional arguments passed to `parse()` are forwarded to all actions.

```c++
std::string unescape( const std::string& escaped )
{
   std::string unescaped;
   tao::pegtl::string_input in( result, __FUNCTION__ );
   tao::pegtl::parse< text, actions >( in, unescaped );
   return unescaped;
}
```

At the end of the parsing run, the complete unescaped string can be found in the aptly named variable.

A more complete example of how to unescape strings can be found in `src/examples/pegtl/unescape.cpp`.

## Specialising

The rule class for which an action class template is specialised *must* exactly match the definition of the rule in the grammar.
For example consider the following rule.

```c++
struct foo
   : public tao::pegtl::plus< tao::pegtl::alpha > {};
```

Now an action class template can be specialised for `foo`, or for `tao::pegtl::alpha, but *not* for `tao::pegtl::plus< tao::pegtl::alpha >`.

This because base classes are not taken into consideration by the C++ language when choosing a specialisation, which might be surprising when being used to pointer arguments to functions where conversions from pointer-to-derived to pointer-to-base are performed implicitly and silently.

So although the function called by the library to match `foo` is the inherited `tao::pegtl::plus< tao::pegtl::alpha >::match()`, the rule class is `foo` and the function known as `foo::match()`, wherefore an action needs to be specialised for `foo` instead of `tao::pegtl::plus< tao::pegtl::alpha >`.

While it is possible to specialise the action class template for `tao::pegtl::alpha`, it might not be a good idea since the action would be applied for *all* occurrences of `tao::pegtl::alpha` in the grammar.
To circumvent this issue a new name can be given to the `tao::pegtl::alpha`, a name that will not be "randomly" used in other places of the grammar.

```c++
struct bar
   : public tao::pegtl::alpha {};

struct foo
   : public tao::pegtl::plus< bar > {};
```

Now an action class template can be specialised for `foo` and `bar`, but again *not* for `tao::pegtl::plus< bar >` or `tao::pegtl::alpha`.

More precisely, it could be specialised for the latter two rules, but wouldn't ever be called unless these rules were used elsewhere in the grammar, a different kettle of fish.

## Changing Actions

The action class template can be changed, and actions enabled or disabled, in ways beyond supplying, or not, an action to `tao::pegtl::parse()` at the start of a parsing run.

### Via Rules

The [`tao::pegtl::enable<>`](Rule-Reference.md#enable-r-) and [`tao::pegtl::disable<>`](Rule-Reference.md#disable-r-) rules behave just like [`seq<>`](Rule-Reference.md#seq-r-) but, without touching the current action, enable or disable calling of actions within their sub-rules, respectively.

The [`tao::pegtl::action<>`](Rule-Reference.md#action-a-r-) rule also behaves similarly to [`seq<>`](Rule-Reference.md#seq-r-) but takes an action class template as first template parameter and, without enabling or disabling actions, uses its first template parameter as action for the sub-rules.

The following two lines effectively do the same thing, namely parse with `my_grammar` as top-level parsing rule without invoking actions (unless actions are enabled again somewhere else).

```c++
tao::pegtl::parse< my_grammar >( ... );
tao::pegtl::parse< tao::pegtl::disable< my_grammar >, my_actions >( ... );
```

Similarly the following two lines both start parsing `my_grammar` with `my_actions` (again only unless something changes somewhere else).

```c++
tao::pegtl::parse< my_grammar, my_actions >( ... );
tao::pegtl::parse< tao::pegtl::action< my_actions, my_grammar > >( ... );
```

User-defined parsing rules can use `action<>`, `enable<>` and `disable<>` just like any other combinator rules.
For example to disable actions in LISP-style comments the following rule could be used as per `src/example/pegtl/s_expression.cpp`.

```c++
struct comment
   : public tao::pegtl::seq< tao::pegtl::one< '#' >, tao::pegtl::disable< cons_list > > {};
```

This also allows using the same rules multiple times with different action class templates within a grammar.

### Via Actions

The action classes `tao::pegtl::disable_action` and `tao::pegtl::enable_action` can be used via inheritance to disable and enable actions, respectively, for any rule (and its sub-rules).
For example actions can be disabled for `my_rule` in a parsing run using `my_actions` as follows.

```c++
template< typename > struct my_actions;

template<>
struct my_actions< my_rule >
   : public tao::pegtl::disable_action {};

tao::pegtl::parse< my_grammar, my_actions >( ... );
```

Conversely `tao::pegtl::change_action<>` takes a new action class template as only template parameter and changes the current action in a parsing run to its template parameter.

Note that parsing proceeds with the rule to which the action changing action is attached to "as if" the new action had been the current action all along.
The new action can even perform an action change *on the same rule*, however care should be taken to not introduce infinite cycles of changes.

## Changing States

The states, too, can be changed in ways beyond supplying them, or not, to `tao::pegtl::parse()` at the start of a parsing run.

### Via Rules

The [`state<>`](Rule-Reference.md#state-s-r-) behaves similarly to [`seq`](Rule-Reference.md#seq-r-) but uses the first template parameter as type of a new object.
This new object is used replaces the current state(s) for the remainder of the implicit [`seq`](Rule-Reference.md#seq-r-).

The new object is constructed with a const-reference to the current input of the parsing run, and all previous states, if any, as arguments.
If the implicit [`seq`](Rule-Reference.md#seq-r-) of the sub-rules succeeds, then, by default, a member function named `success()` is called on this "new" object, receiving the same arguments as the constructor.
At this point the input will be advanced by whatever the sub-rules have consumed in the meantime.

Please consult `include/tao/pegtl/internal/state.hpp` to see how the default behaviour on success can be changed by overriding `tao::pegtl::state<>::success()` in a derived class when using that class instead.

Embedding a state change into the grammar with [`state<>`](Rule-Reference.md#state-s-r-) is only recommended when some state is used by custom parsing rules.

### Via Actions

The actions `tao::pegtl::change_state<>` and `tao::pegtl::change_states<>` can be used to change from the current to a new set of states while parsing the rules they are attached to.

The differences are summarised in this table; note that `change_state` is more similar to the legacy `change_state` control class as included with the 2.z versions of the PEGTL.

| Feature | `change_state` | `change_states` |
| --- | --- | --- |
| Number of new states | one | any |
| Construction of new states | with input and old states | default |
| Success function on action | if not on new state | required |

With `change_state` only a single new state type can be given as template parameter, and only a single new state will be created.
The constructor of the new state receives the same arguments as per `tao::pegtl::state<>`, the current input from the parsing run and all previous states.

A `success()` static member function is supplied that calls the `success()` member function on the new state, again with the current input from the parsing run and all previous states.
The supplied `success()` can of course be overridden in a derived class.

With `change_states`, being a variadic template, any number of new state types can be given and an appriate set of new states will be created (nearly) simultaneously.
All new states are default-constructed, if something else is required the reader is encouraged to copy and modify the implementation of `change_states` in his or her project.

The user *must* implement a custom `success()` static member function that takes the current input from the parsing run, the new states, and the old states as arguments.

Note that, *unlike* the `tao::pegtl::state<>` combinator, the success functions are *only called when actions are currently enabled*!

Using the changing actions is again done via inheritance as shown in the following example for `change_states`.

```c++
template<>
struct actions< rule >
   : public tao::pegtl::change_states< news1, news2 >
{
   template< typename Input >
   static void success( const Input&, news1&, news2&, /* the previous states*/ )
   {
      // Do whatever with both the new and the old states...
   }
};
```

For a more complete example of how to build a generic JSON data structure with `change_state` and friends see `src/example/pegtl/json_build.cpp`.

## Changing Actions and States

The actions `change_action_and_state<>` and `change_action_and_states<>` combine `change_action` with one of the `change_state<>` or `change_states<>` actions, respectively.
For `change_action_and_state<>` and `change_action_and_states<>` the new action class template is passed as first template parameter as for `change_action`, followed by the new state(s) as given to `change_state<>` and `change_states<>`.

Note that `change_action_and_state<>` and `change_action_and_states<>` behave like `change_action<>` in that they proceed to match the rule to which the changing action is attached to "as if" the new action had been the current action all along.

## Match

Besides `apply()` and `apply0()`, an action class specialization can also have a `match()` static member function.
The default control class template `normal` will detect the presence of a suitable `match()` function and call this function instead of `tao::pegtl::match()`.

```c++
template<>
struct my_actions< tao::pegtl::plus< tao::pegtl::digit > >
{
   template< typename Rule,
             apply_mode A,
             rewind_mode M,
             template< typename... > class Action,
             template< typename... > class Control,
             typename Input,
             typename... States >
   static bool match( Input& in, States&&... st )
   {
      // Call the function that would have been called otherwise,
      // in this case without changing anything...
      return tao::pegtl::match< Rule, A, M, Action, Control >( in, st... );
   }
}
```

Implementing a custom `match()` for an action is considered a rather advanced feature that is not used directly very often.
All "changing" action classes mentioned in this document are implemented as actions with `match()`.
Their implementations can be found in `<tao/pegtl/change_*.hpp>` and should be studied before implementing a custom action with `match()`.

## Nothing

TODO -- not mandatory, apply/0 auto-detection vs nothing, maybe_nothing, primary template with apply/0?

## Troubleshooting

### Boolean Return

Actions returning `bool` are an advanced use case that should be used with caution.
They prevent some internal optimisations, in particular when used with `apply0()`.
They can also have weird effects on the semantics of a parsing run, for example `at< rule >` can succeed for the same input for which `rule` fails when there is a `bool`-action attached to `rule` that returns `false` (remember that actions are disabled within `at<>`).

### State Mismatch

When an action's `apply()` or `apply0()` expects different states than those present in the parsing run there will either be a possible not very helpful compiler error, or it will compile without a call to the action, depending on whether `tao::pegtl::nothing<>` is used as base class of the primary action class template.

By deriving an action specialisation from either `require_apply` or `require_apply0`, as appropriate, a -- potentially more helpful -- compiler error can be provoked, so when the grammar contains `my_rule` and the action is `my_actions` then silently compiling without a call to `apply0()` is no longer possible.

```c++
template<>
struct my_actions< my_rule >
  : public require_apply0
{
   static void apply0( double )
   {
      // ...
   }
}
```

Note that deriving from `require_apply` or `require_apply0` is optional and usually only used for troubleshooting.

## Legacy Actions

See the [section on legacy-style action rules](Rule-Reference.md#action-rules).

Copyright (c) 2014-2019 Dr. Colin Hirsch and Daniel Frey

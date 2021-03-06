// RUN: %clang_cc1 -fsyntax-only -verify -std=c++0x %s

template<typename T> using Id = T; // expected-note {{type alias template 'Id' declared here}}
struct U { static Id<int> V; };
Id<int> ::U::V; // expected-error {{type 'Id<int>' (aka 'int') cannot be used prior to '::' because it has no members}} \
                   expected-error {{C++ requires a type specifier}}

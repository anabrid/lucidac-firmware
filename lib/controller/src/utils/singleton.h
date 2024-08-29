// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/**
 * This is a standard way of defining singletons, Usage is like:
 *
 *   class Foo : Singleton<Foo> { ... }
 *
 * The only advantage of having this kind of class is that it
 * simplifies finding Singletons in the code base.
 *
 * This is a CRTP pattern.
 **/
template <class T> class Singleton {
public:
  static T &get() {
    static T instance;
    return instance;
  }
};

// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

namespace utils {

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

/**
 * Define singletons which are not static-space allocated (and thus
 * consume valuable ICTM space). Instead, this allocates them in the
 * RAM2.
 * 
 * As usual with Singletons, we assume "eternal objects" which have no
 * destruction time.
 * 
 * A note on heap fragmentation: If the first call to get() happens late,
 * heap fragmentation *will* occur. Therefore, make sure to call get() 
 * early on frequently used objects.
 * 
 * Usage is like
 * 
 *    class Foo : HeapSingleton<Foo> { ... }
 * 
 * This is a CRTP pattern.
 **/
template <class T> class HeapSingleton {
public:
  static T &get() {
    static T *instance = nullptr;
    if(!instance) {
      instance = new T{};
    }
    return *instance;
  }
};

} // ns utils
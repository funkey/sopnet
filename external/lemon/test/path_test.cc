/* -*- mode: C++; indent-tabs-mode: nil; -*-
 *
 * This file is a part of LEMON, a generic C++ optimization library.
 *
 * Copyright (C) 2003-2009
 * Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
 * (Egervary Research Group on Combinatorial Optimization, EGRES).
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#include <string>
#include <iostream>

#include <lemon/concepts/path.h>
#include <lemon/concepts/digraph.h>

#include <lemon/path.h>
#include <lemon/list_graph.h>

#include "test_tools.h"

using namespace std;
using namespace lemon;

void check_concepts() {
  checkConcept<concepts::Path<ListDigraph>, concepts::Path<ListDigraph> >();
  checkConcept<concepts::Path<ListDigraph>, Path<ListDigraph> >();
  checkConcept<concepts::Path<ListDigraph>, SimplePath<ListDigraph> >();
  checkConcept<concepts::Path<ListDigraph>, StaticPath<ListDigraph> >();
  checkConcept<concepts::Path<ListDigraph>, ListPath<ListDigraph> >();
}

// Check if proper copy consructor is called (use valgrind for testing)
template<class _Path>
void checkCopy()
{
  ListDigraph g;
  ListDigraph::Arc a  = g.addArc(g.addNode(), g.addNode());
  
  _Path p,q;
  p.addBack(a);
  q=p;
  _Path r(p);
  StaticPath<ListDigraph> s(r);
}
  
int main() {
  check_concepts();

  checkCopy<Path<ListDigraph> >();
  checkCopy<SimplePath<ListDigraph> >();
  checkCopy<ListPath<ListDigraph> >();

  ListDigraph g;
  ListDigraph::Arc a  = g.addArc(g.addNode(), g.addNode());
  
  Path<ListDigraph> p;
  StaticPath<ListDigraph> q,r;
  p.addBack(a);
  q=p;
  r=q;
  StaticPath<ListDigraph> s(q);

  return 0;
}

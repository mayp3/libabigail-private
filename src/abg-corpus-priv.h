// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2016-2025 Red Hat, Inc.

/// @file
///
/// The private data and functions of the @ref abigail::ir::corpus type.
///
/// Interfaces declared/defined in this file are to be used by parts
/// of libabigail but *NOT* by clients of libabigail.
///

#ifndef __ABG_CORPUS_PRIV_H__
#define __ABG_CORPUS_PRIV_H__

#include "abg-internal.h"
#include "abg-ir.h"
#include "abg-regex.h"
#include "abg-sptr-utils.h"
#include "abg-symtab-reader.h"
#include "abg-interned-str.h"

namespace abigail
{

namespace sptr_utils
{
}// end namespace sptr_utils

namespace ir
{

using regex::regex_t_sptr;

/// A convenience typedef for std::vector<regex_t_sptr>.
typedef vector<regex_t_sptr> regex_t_sptrs_type;

// <corpus::exported_decls_builder>

/// Convenience typedef for a hash map which key is a string and which
/// data is a vector of abigail::ir::function_decl*
typedef unordered_map<string, vector<function_decl*> > str_fn_ptrs_map_type;

/// Convenience typedef for a hash map which key is a string and which
/// data is a set of abigail::ir::function_decl*
typedef unordered_map<string, std::unordered_set<function_decl*> >
str_fn_ptr_set_map_type;

/// Convenience typedef for a hash map which key is an interned_string
/// and which data is a set of abigail::ir::function_decl*
typedef unordered_map<interned_string,
		      std::unordered_set<function_decl*>,
		      hash_interned_string> istr_fn_ptr_set_map_type;

/// Convenience typedef for a hash map which key is a string and
/// which data is an abigail::ir::var_decl*.
typedef unordered_map<string, var_decl_sptr> str_var_ptr_map_type;

/// Convenience typedef for a hash map which key is an interned_string
/// and which data is an abigail::ir::var_decl*.
typedef unordered_map<interned_string,
		      var_decl_sptr,
		      hash_interned_string> istr_var_ptr_map_type;

/// Convenience typedef for a hash map which key is an interned_string
/// and which data is a set of abigail::ir::var_decl_sptr
typedef unordered_map<interned_string,
		      std::unordered_set<var_decl_sptr>,
		      hash_interned_string> istr_var_ptr_set_map_type;

/// The type of the private data of @ref
/// corpus::exported_decls_builder type.
class corpus::exported_decls_builder::priv
{
  friend class corpus::exported_decls_builder;
  friend class corpus;

  priv();

  functions&		fns_;
  variables&		vars_;
  // A map that associates a function ID (function symbol and its
  // version) to a vector of functions with that ID.  Normally, one
  // would think that in the corpus, there must only one function for
  // a given ID.  Actually, in c++, there can be two function template
  // instantiations that produce the same function ID because the
  // template parameters of the second instantiation are just typedefs
  // of the first instantiation, for instance.  So there can be cases
  // where one ID appertains to more than one function.
  istr_fn_ptr_set_map_type	id_fns_map_;
  istr_var_ptr_set_map_type	id_vars_map_;
  strings_type&	fns_suppress_regexps_;
  regex_t_sptrs_type	compiled_fns_suppress_regexp_;
  strings_type&	vars_suppress_regexps_;
  regex_t_sptrs_type	compiled_vars_suppress_regexp_;
  strings_type&	fns_keep_regexps_;
  regex_t_sptrs_type	compiled_fns_keep_regexps_;
  strings_type&	vars_keep_regexps_;
  regex_t_sptrs_type	compiled_vars_keep_regexps_;
  strings_type&	sym_id_of_fns_to_keep_;
  strings_type&	sym_id_of_vars_to_keep_;

public:

  priv(functions& fns,
       variables& vars,
       strings_type& fns_suppress_regexps,
       strings_type& vars_suppress_regexps,
       strings_type& fns_keep_regexps,
       strings_type& vars_keep_regexps,
       strings_type& sym_id_of_fns_to_keep,
       strings_type& sym_id_of_vars_to_keep)
    : fns_(fns),
      vars_(vars),
      fns_suppress_regexps_(fns_suppress_regexps),
      vars_suppress_regexps_(vars_suppress_regexps),
      fns_keep_regexps_(fns_keep_regexps),
      vars_keep_regexps_(vars_keep_regexps),
    sym_id_of_fns_to_keep_(sym_id_of_fns_to_keep),
    sym_id_of_vars_to_keep_(sym_id_of_vars_to_keep)
  {}

  /// Getter for the compiled regular expressions that designate the
  /// functions to suppress from the set of exported functions.
  ///
  /// @return a vector of the compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_fns_suppress()
  {
    if (compiled_fns_suppress_regexp_.empty())
      {
	for (vector<string>::const_iterator i =
	       fns_suppress_regexps_.begin();
	     i != fns_suppress_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = regex::compile(*i);
	    if (r)
	      compiled_fns_suppress_regexp_.push_back(r);
	  }
      }
    return compiled_fns_suppress_regexp_;
  }

  /// Getter for the compiled regular expressions that designates the
  /// functions to keep in the set of exported functions.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_fns_keep()
  {
    if (compiled_fns_keep_regexps_.empty())
      {
	for (vector<string>::const_iterator i =
	       fns_keep_regexps_.begin();
	     i != fns_keep_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = regex::compile(*i);
	    if (r)
	      compiled_fns_keep_regexps_.push_back(r);
	  }
      }
    return compiled_fns_keep_regexps_;
  }

  /// Getter of the compiled regular expressions that designate the
  /// variables to suppress from the set of exported variables.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_vars_suppress()
  {
    if (compiled_vars_suppress_regexp_.empty())
      {
	for (vector<string>::const_iterator i =
	       vars_suppress_regexps_.begin();
	     i != vars_suppress_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = regex::compile(*i);
	    if (r)
	      compiled_vars_suppress_regexp_.push_back(r);
	  }
      }
    return compiled_vars_suppress_regexp_;
  }

  /// Getter for the compiled regular expressions that designate the
  /// variables to keep in the set of exported variables.
  ///
  /// @return a vector of compiled regular expressions.
  regex_t_sptrs_type&
  compiled_regex_vars_keep()
  {
    if (compiled_vars_keep_regexps_.empty())
      {
	for (vector<string>::const_iterator i =
	       vars_keep_regexps_.begin();
	     i != vars_keep_regexps_.end();
	     ++i)
	  {
	    regex_t_sptr r = regex::compile(*i);
	    if (r)
	      compiled_vars_keep_regexps_.push_back(r);
	  }
      }
    return compiled_vars_keep_regexps_;
  }

  /// Getter for a map of the IDs of the functions that are present in
  /// the set of exported functions.
  ///
  /// This map is useful during the construction of the set of
  /// exported functions, at least to ensure that every function is
  /// present only once in that set.  Actually, for each symbol ID,
  /// there can be several functions, given that each of those have
  /// different declaration names; this can happen with function
  /// template instantiations which decl names differ because the type
  /// parameters of the templates are typedefs of each other.
  ///
  /// @return a map which key is a string and which data is a pointer
  /// to a function.
  const istr_fn_ptr_set_map_type&
  id_fns_map() const
  {return id_fns_map_;}

  /// Getter for a map of the IDs of the functions that are present in
  /// the set of exported functions.
  ///
  /// The map associates the ID of a function with the set of
  /// functions having the symbol that matches the ID.
  ///
  /// @return a map which key is a string and which data is a set of
  /// functions.
  istr_fn_ptr_set_map_type&
  id_fns_map()
  {return id_fns_map_;}

  /// Getter for a map of the IDs of the variables that are present in
  /// the set of exported variables.
  ///
  /// The map associates the ID of a variable with the set of variables
  /// having the symbol that matches the ID.
  ///
  /// @return a map which key is a string and which data is a set of
  /// variables.
  istr_var_ptr_set_map_type&
  id_vars_map()
  {return id_vars_map_;}

  /// Getter for a map of the IDs of the variables that are present in
  /// the set of exported variables.
  ///
  /// The map associates the ID of a variable with the set of variables
  /// having the symbol that matches the ID.
  ///
  /// @return a map which key is a string and which data is a set of
  /// variables.
  const istr_var_ptr_set_map_type&
  id_vars_map() const
  {return id_vars_map_;}

  /// Returns an ID for a given function.
  ///
  /// @param fn the function to calculate the ID for.
  ///
  /// @return a reference to a string representing the function ID.
  interned_string
  get_id(const function_decl& fn)
  {return fn.get_id();}

  /// Returns an ID for a given variable.
  ///
  /// @param var the variable to calculate the ID for.
  ///
  /// @return a reference to a string representing the variable ID.
  interned_string
  get_id(const var_decl& var)
  {return var.get_id();}

  /// Test if a given function ID is in the id-functions map.
  ///
  /// If it is, then return a pointer to the vector of functions with
  /// that ID.  If not, just return nil.
  ///
  /// @param fn_id the ID to consider.
  ///
  /// @return the pointer to the vector of functions with ID @p fn_id,
  /// or nil if no function with that ID exists.
  std::unordered_set<function_decl*>*
  fn_id_is_in_id_fns_map(const interned_string& fn_id)
  {
    istr_fn_ptr_set_map_type& m = id_fns_map();
    auto i = m.find(fn_id);
    if (i == m.end())
      return 0;
    return &i->second;
  }

  /// Test if a a function if the same ID as a given function is
  /// present in the id-functions map.
  ///
  /// @param fn the function to consider.
  ///
  /// @return a pointer to the vector of functions with the same ID as
  /// @p fn, that are present in the id-functions map, or nil if no
  /// function with the same ID as @p fn is present in the
  /// id-functions map.
  std::unordered_set<function_decl*>*
  fn_id_is_in_id_fns_map(const function_decl* fn)
  {
    interned_string fn_id = fn->get_id();
    return fn_id_is_in_id_fns_map(fn_id);
  }

  /// Test if a given function is present in a set of functions.
  ///
  /// The function compares the ID and the qualified name of
  /// functions.
  ///
  /// @param fn the function to consider.
  ///
  /// @parm fns the set of functions to consider.
  static bool
  fn_is_in_fns(function_decl* fn,
	       const std::unordered_set<function_decl*>& fns)
  {
    if (fns.empty())
      return false;

    if (fns.find(fn) != fns.end())
      return true;

    const string fn_id = fn->get_id();
    for (const auto f : fns)
      if (f->get_id() == fn_id
	  && f->get_qualified_name() == fn->get_qualified_name())
	return true;

    return false;
  }

  /// Test if a given function is present in a set of functions,
  /// by looking at the pretty representation of the function, in
  /// addition to looking at its ID.
  ///
  /// This is useful because sometimes a given ELF symbol (alias)
  /// might be for several different functions.  In that case, using
  /// the function pretty representation might be a way to
  /// differentiate the functions having the same ELF symbol alias.
  ///
  /// The function compares the ID and the qualified name of
  /// functions.
  ///
  /// @param fn the function to consider.
  ///
  /// @parm fns the set of functions to consider.
  ///
  /// @return true if @p fn is present in @p fns.
  static bool
  fn_is_in_fns_by_repr(function_decl* fn,
		       const std::unordered_set<function_decl*>& fns,
		       string& pretty_representation)
  {
    if (!fn_is_in_fns(fn, fns))
      return false;

    const string repr = fn->get_pretty_representation();
    const string fn_id = fn->get_id();
    for (const auto f : fns)
      if (f->get_id() == fn_id
	  && f->get_pretty_representation() == repr)
	{
	  pretty_representation = repr;
	  return true;
	}

    return false;
  }

  ///  Test if a function is in the id-functions map.
  ///
  ///  @param fn the function to consider.
  ///
  ///  @return true iff the function is in the id-functions map.
  bool
  fn_is_in_id_fns_map(function_decl* fn)
  {
    std::unordered_set<function_decl*>* fns = fn_id_is_in_id_fns_map(fn);
    if (fns && fn_is_in_fns(fn, *fns))
      return true;
    return false;
  }

  /// Add a given function to the map of functions that are present in
  /// the set of exported functions.
  ///
  /// @param fn the function to add to the map.
  void
  add_fn_to_id_fns_map(function_decl* fn)
  {
    if (!fn)
      return;

    // First associate the function id to the function.
    interned_string fn_id = fn->get_id();
    std::unordered_set<function_decl*>* fns = fn_id_is_in_id_fns_map(fn_id);
    if (!fns)
      fns = &(id_fns_map()[fn_id] = std::unordered_set<function_decl*>());
    fns->insert(fn);

    // Now associate all aliases of the underlying symbol to the
    // function too.
    elf_symbol_sptr sym = fn->get_symbol();
    ABG_ASSERT(sym);
    string sym_id;
    do
      {
	sym_id = sym->get_id_string();
	if (sym_id == fn_id)
	  goto loop;
	fns = fn_id_is_in_id_fns_map(fn_id);
	if (!fns)
	  fns = &(id_fns_map()[fn_id] = std::unordered_set<function_decl*>());
	fns->insert(fn);
      loop:
	sym = sym->get_next_alias();
      }
    while (sym && !sym->is_main_symbol());
  }

  /// Test if a given (ID of a) variable is present in the variable
  /// map.  In other words, it tests if a given variable is present in
  /// the set of exported variables.
  ///
  /// @param fn_id the ID of the variable to consider.
  ///
  /// @return a pointer to the set of variables that have the same ID
  /// as @p var_id.
  std::unordered_set<var_decl_sptr>*
  var_id_is_in_id_vars_map(const interned_string& var_id)
  {
    istr_var_ptr_set_map_type& m = id_vars_map();
    auto i = m.find(var_id);
    if (i != m.end())
      return &i->second;
    return nullptr;
  }

  /// Test if a given (ID of a) variable is present in the variable
  /// map.  In other words, it tests if a given variable is present in
  /// the set of exported variables.
  ///
  /// @param fn_id the ID of the variable to consider.
  ///
  /// @return a pointer to the set of variables that have the same ID
  /// as @p var_id.
  const std::unordered_set<var_decl_sptr>*
  var_id_is_in_id_vars_map(const interned_string& var_id) const
  {
    return const_cast<corpus::exported_decls_builder::priv*>(this)->
      var_id_is_in_id_vars_map(var_id);
  }

  /// Test if a given variable is present in a set of variables.
  ///
  /// The variable compares the ID and the qualified name of
  /// variables.
  ///
  /// @param fn the variable to consider.
  ///
  /// @parm fns the set of variables to consider.
  static bool
  var_is_in_vars(const var_decl_sptr& var,
		 const std::unordered_set<var_decl_sptr>& vars)
  {
    if (vars.empty())
      return false;

    if (vars.find(var) != vars.end())
      return true;

    const string var_id = var->get_id();
    for (const auto& v: vars)
      if (v->get_id() == var_id
	  && v->get_qualified_name() == var->get_qualified_name())
	return true;

    return false;
  }

  /// Test if a given variable is present in a set of variables.
  ///
  /// The variable compares the ID and the qualified name of
  /// variables.
  ///
  /// @param fn the variable to consider.
  ///
  /// @parm fns the set of variables to consider.
  bool
  var_is_in_id_vars_map(const var_decl_sptr& var)
  {
    if (!var)
      return false;

    interned_string var_id = var->get_id();
    const std::unordered_set<var_decl_sptr>* vars =
      var_id_is_in_id_vars_map(var_id);
    if (vars && var_is_in_vars(var, *vars))
      return true;
    return false;
  }

  /// Add a given variable to the map of variables that are present in
  /// the set of exported variables.
  ///
  /// @param fn the variable to add to the map.
  void
  add_var_to_id_vars_map(const var_decl_sptr& var)
  {
    if (!var)
      return;

    // First associate the var id to the variable.
    interned_string var_id = var->get_id();
    std::unordered_set<var_decl_sptr>* vars = var_id_is_in_id_vars_map(var_id);
    if (!vars)
      vars = &(id_vars_map()[var_id] = std::unordered_set<var_decl_sptr>());
    vars->insert(var);

    // Now associate all aliases of th underlying symbol to the
    // variable too.
    elf_symbol_sptr sym = var->get_symbol();
    ABG_ASSERT(sym);
    string sym_id;
    do
      {
	sym_id = sym->get_id_string();
	if (sym_id == var_id)
	  goto loop;
	vars = var_id_is_in_id_vars_map(var_id);
	if (!vars)
	  vars = &(id_vars_map()[var_id] = std::unordered_set<var_decl_sptr>());
	vars->insert(var);
      loop:
	sym = sym->get_next_alias();
      }
    while (sym && !sym->is_main_symbol());
  }

  /// Add a function to the set of exported functions.
  ///
  /// @param fn the function to add to the set of exported functions.
  void
  add_fn_to_exported(function_decl* fn)
  {
    if (!fn_is_in_id_fns_map(fn))
      {
	fns_.push_back(fn);
	add_fn_to_id_fns_map(fn);
      }
  }

  /// Add a variable to the set of exported variables.
  ///
  /// @param fn the variable to add to the set of exported variables.
  void
  add_var_to_exported(const var_decl_sptr& var)
  {
    if (!var_is_in_id_vars_map(var))
      {
	vars_.push_back(var);
	add_var_to_id_vars_map(var);
      }
  }

  /// Getter for the set of ids of functions to keep in the set of
  /// exported functions.
  ///
  /// @return the set of ids of functions to keep in the set of
  /// exported functions.
  const strings_type&
  sym_id_of_fns_to_keep() const
  {return sym_id_of_fns_to_keep_;}

  /// Getter for the set of ids of variables to keep in the set of
  /// exported variables.
  ///
  /// @return the set of ids of variables to keep in the set of
  /// exported variables.
  const strings_type&
  sym_id_of_vars_to_keep() const
  {return sym_id_of_vars_to_keep_;}

  /// Look at the set of functions to keep and tell if if a given
  /// function is to be kept, according to that set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_id_of_fns_to_keep(const function_decl* fn)
  {
    if (!fn)
      return false;

    bool keep = true;

    if (elf_symbol_sptr sym = fn->get_symbol())
      {
	if (!sym_id_of_fns_to_keep().empty())
	  keep = false;
	if (!keep)
	  {
	    for (vector<string>::const_iterator i =
		   sym_id_of_fns_to_keep().begin();
		 i != sym_id_of_fns_to_keep().end();
		 ++i)
	      {
		string sym_name, sym_version;
		ABG_ASSERT(elf_symbol::get_name_and_version_from_id(*i,
								    sym_name,
								    sym_version));
		if (sym_name == sym->get_name()
		    && sym_version == sym->get_version().str())
		  {
		    keep = true;
		    break;
		  }
	      }
	  }
      }
    else
      keep = false;

    return keep;
  }

  /// Look at the set of functions to suppress from the exported
  /// functions set and tell if if a given function is to be kept,
  /// according to that set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_regex_of_fns_to_suppress(const function_decl *fn)
  {
    if (!fn)
      return false;

    string frep = fn->get_qualified_name();
    bool keep = true;

    for (regex_t_sptrs_type::const_iterator i =
	   compiled_regex_fns_suppress().begin();
	 i != compiled_regex_fns_suppress().end();
	 ++i)
      if (regex::match(*i, frep))
	{
	  keep = false;
	  break;
	}

    return keep;
  }

  /// Look at the regular expressions of the functions to keep and
  /// tell if if a given function is to be kept, according to that
  /// set.
  ///
  /// @param fn the function to consider.
  ///
  /// @return true iff the function is to be kept.
  bool
  keep_wrt_regex_of_fns_to_keep(const function_decl *fn)
  {
    if (!fn)
      return false;

    string frep = fn->get_qualified_name();
    bool keep = true;

    if (!compiled_regex_fns_keep().empty())
      keep = false;

    if (!keep)
      for (regex_t_sptrs_type::const_iterator i =
	     compiled_regex_fns_keep().begin();
	   i != compiled_regex_fns_keep().end();
	   ++i)
	if (regex::match(*i, frep))
	  {
	    keep = true;
	    break;
	  }

    return keep;
  }

  /// Look at the regular expressions of the variables to keep and
  /// tell if if a given variable is to be kept, according to that
  /// set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_id_of_vars_to_keep(const var_decl_sptr& var)
  {
    if (!var)
      return false;

    bool keep = true;

    if (elf_symbol_sptr sym = var->get_symbol())
      {
	if (!sym_id_of_vars_to_keep().empty())
	  keep = false;
	if (!keep)
	  {
	    for (vector<string>::const_iterator i =
		   sym_id_of_vars_to_keep().begin();
		 i != sym_id_of_vars_to_keep().end();
		 ++i)
	      {
		string sym_name, sym_version;
		ABG_ASSERT(elf_symbol::get_name_and_version_from_id(*i,
								    sym_name,
								    sym_version));
		if (sym_name == sym->get_name()
		    && sym_version == sym->get_version().str())
		  {
		    keep = true;
		    break;
		  }
	      }
	  }
      }
    else
      keep = false;

    return keep;
  }

  /// Look at the set of variables to suppress from the exported
  /// variables set and tell if if a given variable is to be kept,
  /// according to that set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_regex_of_vars_to_suppress(const var_decl_sptr var)
  {
    if (!var)
      return false;

    string frep = var->get_qualified_name();
    bool keep = true;

    for (regex_t_sptrs_type::const_iterator i =
	   compiled_regex_vars_suppress().begin();
	 i != compiled_regex_vars_suppress().end();
	 ++i)
      if (regex::match(*i, frep))
	{
	  keep = false;
	  break;
	}

    return keep;
  }

  /// Look at the regular expressions of the variables to keep and
  /// tell if if a given variable is to be kept, according to that
  /// set.
  ///
  /// @param fn the variable to consider.
  ///
  /// @return true iff the variable is to be kept.
  bool
  keep_wrt_regex_of_vars_to_keep(const var_decl_sptr& var)
  {
    if (!var)
      return false;

    string frep = var->get_qualified_name();
    bool keep = true;

    if (!compiled_regex_vars_keep().empty())
      keep = false;

    if (!keep)
      {
	for (regex_t_sptrs_type::const_iterator i =
	       compiled_regex_vars_keep().begin();
	     i != compiled_regex_vars_keep().end();
	     ++i)
	  if (regex::match(*i, frep))
	    {
	      keep = true;
	      break;
	    }
      }

    return keep;
  }
}; // end struct corpus::exported_decls_builder::priv


/// The private data of the @ref corpus type.
struct corpus::priv
{
  mutable unordered_map<string, type_base_sptr> canonical_types_;
  string					format_major_version_number_;
  string					format_minor_version_number_;
  const environment&				env;
  corpus_group*				group;
  corpus::exported_decls_builder_sptr		exported_decls_builder;
  corpus::origin				origin_;
  vector<string>				regex_patterns_fns_to_suppress;
  vector<string>				regex_patterns_vars_to_suppress;
  vector<string>				regex_patterns_fns_to_keep;
  vector<string>				regex_patterns_vars_to_keep;
  vector<string>				sym_id_fns_to_keep;
  vector<string>				sym_id_vars_to_keep;
  string					path;
  vector<string>				needed;
  string					soname;
  string					architecture_name;
  translation_units				members;
  string_tu_map_type				path_tu_map;
  vector<const function_decl*>			fns;
  vector<var_decl_sptr>			vars;
  functions_set				undefined_fns;
  functions					sorted_undefined_fns;
  variables_set				undefined_vars;
  variables					sorted_undefined_vars;
  symtab_reader::symtab_sptr			symtab_;
  // The type maps contained in this data member are populated if the
  // corpus follows the One Definition Rule and thus if there is only
  // one copy of a type with a given name, per corpus. Otherwise, if
  // there can be several *different* types with the same name, then
  // the type maps are all empty.  The types are then maintained in
  // type maps that are in each translation units.
  //
  // In other words, to lookup a given type, if the corpus allows the
  // One Definition Rule, then lookup can be done by looking into this
  // data member.  Otherwise, the lookup must be made by looking into
  // the type maps of each translation unit.
  type_maps					types_;
  type_maps					type_per_loc_map_;
  mutable vector<type_base_wptr>		types_not_reachable_from_pub_ifaces_;
  unordered_set<interned_string, hash_interned_string> *pub_type_pretty_reprs_;
  bool 					do_log;

private:
  priv();

  mutable abg_compat::optional<elf_symbols> sorted_var_symbols;
  mutable abg_compat::optional<string_elf_symbols_map_type> var_symbol_map;
  mutable abg_compat::optional<elf_symbols> sorted_undefined_var_symbols;
  mutable abg_compat::optional<string_elf_symbols_map_type> undefined_var_symbol_map;
  mutable abg_compat::optional<elf_symbols> unrefed_var_symbols;
  mutable abg_compat::optional<elf_symbols> sorted_fun_symbols;
  mutable abg_compat::optional<string_elf_symbols_map_type> fun_symbol_map;
  mutable abg_compat::optional<elf_symbols> sorted_undefined_fun_symbols;
  mutable abg_compat::optional<string_elf_symbols_map_type> undefined_fun_symbol_map;
  mutable abg_compat::optional<elf_symbols> unrefed_fun_symbols;

public:
  priv(const string &		p,
       const environment&	e)
    : env(e),
      group(),
      origin_(ARTIFICIAL_ORIGIN),
      path(p),
      pub_type_pretty_reprs_(),
      do_log()
  {}

  type_maps&
  get_types();

  const type_maps&
  get_types() const;

  const elf_symbols&
  get_sorted_fun_symbols() const;

  const string_elf_symbols_map_type&
  get_fun_symbol_map() const;

  const elf_symbols&
  get_sorted_undefined_fun_symbols() const;

  const string_elf_symbols_map_type&
  get_undefined_fun_symbol_map() const;

  const elf_symbols&
  get_unreferenced_function_symbols() const;

  const elf_symbols&
  get_sorted_var_symbols() const;

  const string_elf_symbols_map_type&
  get_var_symbol_map() const;

  const elf_symbols&
  get_sorted_undefined_var_symbols() const;

  const string_elf_symbols_map_type&
  get_undefined_var_symbol_map() const;

  const elf_symbols&
  get_unreferenced_variable_symbols() const;

  unordered_set<interned_string, hash_interned_string>*
  get_public_types_pretty_representations();

  std::unordered_set<function_decl*>*
  lookup_functions(const interned_string& id);

  ~priv();
}; // end struct corpus::priv

void
maybe_update_scope_lookup_map(const scope_decl_sptr& member_scope);

void
maybe_update_scope_lookup_map(const decl_base_sptr& member_scope);

void
maybe_update_types_lookup_map(const type_decl_sptr& basic_type);

void
maybe_update_types_lookup_map(const class_decl_sptr& class_type);

void
maybe_update_types_lookup_map(const union_decl_sptr& union_type);

void
maybe_update_types_lookup_map(const enum_type_decl_sptr& enum_type);

void
maybe_update_types_lookup_map(const typedef_decl_sptr& typedef_type);

void
maybe_update_types_lookup_map(const qualified_type_def_sptr& qualified_type);

void
maybe_update_types_lookup_map(const pointer_type_def_sptr& pointer_type);

void
maybe_update_types_lookup_map(const reference_type_def_sptr& reference_type);

void
maybe_update_types_lookup_map(const array_type_def_sptr& array_type);

void
maybe_update_types_lookup_map(scope_decl *scope,
			      const function_type_sptr& function_type);

void
maybe_update_types_lookup_map(const decl_base_sptr& decl);

void
maybe_update_types_lookup_map(const type_base_sptr& type);

}// end namespace ir

}// end namespace abigail

#endif // __ABG_CORPUS_PRIV_H__

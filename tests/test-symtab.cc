// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2020-2025 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This program tests symtab invariants through abg-corpus.

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "abg-corpus.h"
#include "abg-dwarf-reader.h"
#include "abg-fwd.h"
#include "abg-ir.h"
#include "abg-tools-utils.h"
#include "lib/catch.hpp"
#include "test-utils.h"

using namespace abigail;

using ir::environment;
using ir::environment_sptr;
using suppr::suppressions_type;

static const std::string test_data_dir =
    std::string(abigail::tests::get_src_dir()) + "/tests/data/test-symtab/";

// There are 4 undefined global variables per binary, at least in this
// testsuite. For instance, here are the undefined global variables
// what we see in the
// "tests/data/test-symtab/basic/single_variable.so" binary:
// __cxa_finalize, _ITM_registerTMCloneTable,
// _ITM_deregisterTMCloneTable, __gmon_start__.
static const int NB_UNDEFINED_VARS_PER_BINARY = 4;

fe_iface::status
read_corpus(const std::string&		    path,
	    corpus_sptr&		    result,
	    const std::vector<std::string>& whitelist_paths =
		std::vector<std::string>())
{
  const std::string& absolute_path = test_data_dir + path;

  environment env;
  const std::vector<char**> debug_info_root_paths;
  abigail::elf_based_reader_sptr rdr =
    dwarf::create_reader(absolute_path, debug_info_root_paths,
			 env, /* load_all_type = */ true,
			 /* linux_kernel_mode = */ true);

  if (!whitelist_paths.empty())
    {
      const suppressions_type& wl_suppr =
	tools_utils::gen_suppr_spec_from_kernel_abi_whitelists(
	  whitelist_paths);
      REQUIRE_FALSE(wl_suppr.empty());
      rdr->add_suppressions(wl_suppr);
    }

  fe_iface::status status = fe_iface::STATUS_UNKNOWN;
  result = rdr->read_corpus(status);

  REQUIRE(status != fe_iface::STATUS_UNKNOWN);
  return status;
}

TEST_CASE("Symtab::Empty", "[symtab, basic]")
{
  const std::string	     binary = "basic/empty.so";
  corpus_sptr		     corpus_ptr;
  read_corpus(binary, corpus_ptr);
  REQUIRE(corpus_ptr->get_fun_symbol_map().empty());
  REQUIRE(corpus_ptr->get_var_symbol_map().empty());
}

TEST_CASE("Symtab::NoDebugInfo", "[symtab, basic]")
{
  const std::string	     binary = "basic/no_debug_info.so";
  corpus_sptr		     corpus_ptr;
  const fe_iface::status status = read_corpus(binary, corpus_ptr);
  REQUIRE(corpus_ptr);

  REQUIRE(status
	  == (fe_iface::STATUS_OK
	      | fe_iface::STATUS_DEBUG_INFO_NOT_FOUND));
}

// this value indicates in the following helper method, that we do not want to
// assert for this particular value. In other words, N is a placeholder for an
// arbitrary value.
#define N std::numeric_limits<size_t>::max()

corpus_sptr
assert_symbol_count(const std::string& path,
		    size_t	       function_symbols = 0,
		    size_t	       variable_symbols = 0,
		    size_t	       undefined_function_symbols = 0,
		    size_t	       undefined_variable_symbols = 0,
		    const std::vector<std::string>& whitelist_paths =
			std::vector<std::string>())
{
  corpus_sptr		     corpus_ptr;
  const fe_iface::status status =
    read_corpus(path, corpus_ptr, whitelist_paths);
  REQUIRE(corpus_ptr);

  REQUIRE((status & fe_iface::STATUS_OK));
  const corpus& corpus = *corpus_ptr;

  size_t total_symbols = 0;

  if (function_symbols != N)
    {
      CHECK(corpus.get_sorted_fun_symbols().size() == function_symbols);
      CHECK(corpus.get_fun_symbol_map().size() == function_symbols);
      total_symbols += function_symbols;
    }
  if (variable_symbols != N)
    {
      CHECK(corpus.get_sorted_var_symbols().size() == variable_symbols);
      CHECK(corpus.get_var_symbol_map().size() == variable_symbols);
      total_symbols += variable_symbols;
    }
  if (undefined_variable_symbols != N)
    {
      CHECK(corpus.get_sorted_undefined_fun_symbols().size()
	    == undefined_function_symbols);
      CHECK(corpus.get_undefined_fun_symbol_map().size()
	    == undefined_function_symbols);
      total_symbols += undefined_function_symbols;
    }
  if (undefined_function_symbols != N)
    {
      CHECK(corpus.get_sorted_undefined_var_symbols().size()
	    == undefined_variable_symbols);
      CHECK(corpus.get_undefined_var_symbol_map().size()
	    == undefined_variable_symbols);
      total_symbols += undefined_variable_symbols;
    }

  // assert the corpus reports being empty consistently with the symbol count
  CHECK(corpus.is_empty() == (total_symbols == 0));

  return corpus_ptr;
}

TEST_CASE("Symtab::SimpleSymtabs", "[symtab, basic]")
{
  GIVEN("a binary with no exported symbols")
  {
    // TODO: should pass, but does currently not as empty tables are treated
    //       like the error case, but this is an edge case anyway.
    // assert_symbol_count("empty.so");
  }

  GIVEN("a binary with a single exported function")
  {
    const std::string	   binary = "basic/single_function.so";
    const corpus_sptr&	   corpus =
      assert_symbol_count(binary, 1, 0, 0, NB_UNDEFINED_VARS_PER_BINARY);
    const elf_symbol_sptr& symbol =
	corpus->lookup_function_symbol("exported_function");
    REQUIRE(symbol);
    CHECK(!corpus->lookup_variable_symbol("exported_function"));
    CHECK(symbol == corpus->lookup_function_symbol(*symbol));
    CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
  }

  GIVEN("a binary with a single exported variable")
  {
    const std::string	   binary = "basic/single_variable.so";
    const corpus_sptr&	   corpus =
      assert_symbol_count(binary, 0, 1, 0, NB_UNDEFINED_VARS_PER_BINARY);
    const elf_symbol_sptr& symbol =
	corpus->lookup_variable_symbol("exported_variable");
    REQUIRE(symbol);
    CHECK(!corpus->lookup_function_symbol("exported_variable"));
    CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
    CHECK(symbol != corpus->lookup_function_symbol(*symbol));
  }

  GIVEN("a binary with one function and one variable exported")
  {
    const std::string  binary = "basic/one_function_one_variable.so";
    const corpus_sptr& corpus =
      assert_symbol_count(binary, 1, 1, 0, NB_UNDEFINED_VARS_PER_BINARY);
    CHECK(corpus->lookup_function_symbol("exported_function"));
    CHECK(!corpus->lookup_variable_symbol("exported_function"));
    CHECK(corpus->lookup_variable_symbol("exported_variable"));
    CHECK(!corpus->lookup_function_symbol("exported_variable"));
  }

  GIVEN("a binary with a single undefined function")
  {
    const std::string  binary = "basic/single_undefined_function.so";
    const corpus_sptr corpus =
      assert_symbol_count(binary, 0, 0, 1, NB_UNDEFINED_VARS_PER_BINARY);
  }

  GIVEN("a binary with a single undefined variable")
  {
    const std::string  binary = "basic/single_undefined_variable.so";
    const corpus_sptr corpus =
      assert_symbol_count(binary, 0, 0, 0, NB_UNDEFINED_VARS_PER_BINARY + 1);
  }

  GIVEN("a binary with one function and one variable undefined")
  {
    const std::string  binary = "basic/one_function_one_variable_undefined.so";
    const corpus_sptr corpus =
      assert_symbol_count(binary, 0, 0, 1, NB_UNDEFINED_VARS_PER_BINARY + 1);
  }
}

TEST_CASE("Symtab::SymtabWithWhitelist", "[symtab, whitelist]")
{
  GIVEN("a binary with one function and one variable exported")
  {
    const std::string binary = "basic/one_function_one_variable.so";

    GIVEN("we read the binary without any whitelists")
    {
      const corpus_sptr& corpus =
	assert_symbol_count(binary, 1, 1, 0, NB_UNDEFINED_VARS_PER_BINARY);
      CHECK(corpus->lookup_function_symbol("exported_function"));
      CHECK(!corpus->lookup_variable_symbol("exported_function"));
      CHECK(corpus->lookup_variable_symbol("exported_variable"));
      CHECK(!corpus->lookup_function_symbol("exported_variable"));
    }

    GIVEN("we read the binary with all symbols on the whitelists")
    {
      std::vector<std::string> whitelists;
      whitelists.push_back(test_data_dir
			   + "basic/one_function_one_variable_all.whitelist");
      const corpus_sptr& corpus =
	assert_symbol_count(binary, 1, 1, 0,
			    NB_UNDEFINED_VARS_PER_BINARY,
			    whitelists);
      CHECK(corpus->lookup_function_symbol("exported_function"));
      CHECK(!corpus->lookup_variable_symbol("exported_function"));
      CHECK(corpus->lookup_variable_symbol("exported_variable"));
      CHECK(!corpus->lookup_function_symbol("exported_variable"));
    }

    GIVEN("we read the binary with only irrelevant symbols whitelisted")
    {
      std::vector<std::string> whitelists;
      whitelists.push_back(
	test_data_dir
	+ "basic/one_function_one_variable_irrelevant.whitelist");

      corpus_sptr		 corpus_ptr;
      read_corpus(binary, corpus_ptr, whitelists);
      REQUIRE(corpus_ptr->get_fun_symbol_map().empty());
      REQUIRE(corpus_ptr->get_var_symbol_map().empty());
    }

    GIVEN("we read the binary with only the function whitelisted")
    {
      std::vector<std::string> whitelists;
      whitelists.push_back(
	test_data_dir + "basic/one_function_one_variable_function.whitelist");
      const corpus_sptr& corpus =
	assert_symbol_count(binary, 1, 0, 0,
			    NB_UNDEFINED_VARS_PER_BINARY,
			    whitelists);
      CHECK(corpus->lookup_function_symbol("exported_function"));
      CHECK(!corpus->lookup_variable_symbol("exported_function"));
      CHECK(!corpus->lookup_variable_symbol("exported_variable"));
      CHECK(!corpus->lookup_function_symbol("exported_variable"));
    }

    GIVEN("we read the binary with only the variable whitelisted")
    {
      std::vector<std::string> whitelists;
      whitelists.push_back(
	test_data_dir + "basic/one_function_one_variable_variable.whitelist");
      const corpus_sptr& corpus =
	assert_symbol_count(binary, 0, 1, 0,
			    NB_UNDEFINED_VARS_PER_BINARY,
			    whitelists);
      CHECK(!corpus->lookup_function_symbol("exported_function"));
      CHECK(!corpus->lookup_variable_symbol("exported_function"));
      CHECK(corpus->lookup_variable_symbol("exported_variable"));
      CHECK(!corpus->lookup_function_symbol("exported_variable"));
    }
  }
}

TEST_CASE("Symtab::AliasedFunctionSymbols", "[symtab, functions, aliases]")
{
  const std::string  binary = "basic/aliases.so";
  const corpus_sptr& corpus =
    assert_symbol_count(binary, 5, 5, 0,
			NB_UNDEFINED_VARS_PER_BINARY);

  // The main symbol is not necessarily the one that is aliased to in the
  // code So, this can't be decided by just looking at ELF. Hence acquire the
  // main symbol.
  const elf_symbol_sptr& main_symbol =
    corpus->lookup_function_symbol("exported_function")->get_main_symbol();
  REQUIRE(main_symbol);

  // But since we know that 'exported_function' is the main symbol and this
  // can be discovered from DWARF
  CHECK(corpus->lookup_function_symbol("exported_function")->is_main_symbol());

  CHECK(corpus->lookup_function_symbol("exported_function")
	  ->get_number_of_aliases() == 4);

  CHECK(main_symbol->has_aliases());
  CHECK(main_symbol->get_number_of_aliases() == 4);
  CHECK(main_symbol->get_main_symbol() == main_symbol);
}

TEST_CASE("Symtab::AliasedVariableSymbols", "[symtab, variables, aliases]")
{
  const std::string  binary = "basic/aliases.so";
  const corpus_sptr& corpus =
    assert_symbol_count(binary, 5, 5, 0, NB_UNDEFINED_VARS_PER_BINARY);
  // The main symbol is not necessarily the one that is aliased to in the
  // code So, this can't be decided by just looking at ELF. Hence acquire the
  // main symbol.
  const elf_symbol_sptr& main_symbol =
    corpus->lookup_variable_symbol("exported_variable")->get_main_symbol();
  REQUIRE(main_symbol);

  // But since we know that 'exported_function' is the main symbol and this
  // can be discovered from DWARF
  CHECK(corpus->lookup_variable_symbol("exported_variable")->is_main_symbol());

  CHECK(corpus->lookup_variable_symbol("exported_variable")
	  ->get_number_of_aliases() == 4);

  CHECK(main_symbol->has_aliases());
  CHECK(main_symbol->get_number_of_aliases() == 4);
  CHECK(main_symbol->get_main_symbol() == main_symbol);
}

static const char* kernel_versions[] = { "4.14", "4.19", "5.4", "5.6" };
static const size_t nr_kernel_versions =
    sizeof(kernel_versions) / sizeof(kernel_versions[0]);

TEST_CASE("Symtab::SimpleKernelSymtabs", "[symtab, basic, kernel, ksymtab]")
{
  for (size_t i = 0; i < nr_kernel_versions; ++i)
    {
      const std::string base_path =
	  "kernel-" + std::string(kernel_versions[i]) + "/";

      GIVEN("The binaries in " + base_path)
      {

	GIVEN("a kernel module with no exported symbols")
	{
	  // TODO: should pass, but does currently not as empty tables are
	  // treated
	  //       like the error case, but this is an edge case anyway.
	  // assert_symbol_count(base_path + "empty.so");
	}

	GIVEN("a kernel module with a single exported function")
	{
	  const std::string	 binary = base_path + "single_function.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 1, 0);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_function_symbol("exported_function");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_variable_symbol("exported_function"));
	  CHECK(symbol == corpus->lookup_function_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
	}

	GIVEN("a kernel module with a single GPL exported function")
	{
	  const std::string	 binary = base_path + "single_function_gpl.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 1, 0);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_function_symbol("exported_function_gpl");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_variable_symbol("exported_function_gpl"));
	  CHECK(symbol == corpus->lookup_function_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_variable_symbol(*symbol));
	}

	GIVEN("a binary with a single exported variable")
	{
	  const std::string	 binary = base_path + "single_variable.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 0, 1);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_variable_symbol("exported_variable");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_function_symbol("exported_variable"));
	  CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_function_symbol(*symbol));
	}

	GIVEN("a binary with a single GPL exported variable")
	{
	  const std::string	 binary = base_path + "single_variable_gpl.ko";
	  const corpus_sptr&	 corpus = assert_symbol_count(binary, 0, 1);
	  const elf_symbol_sptr& symbol =
	      corpus->lookup_variable_symbol("exported_variable_gpl");
	  REQUIRE(symbol);
	  CHECK(!corpus->lookup_function_symbol("exported_variable_gpl"));
	  CHECK(symbol == corpus->lookup_variable_symbol(*symbol));
	  CHECK(symbol != corpus->lookup_function_symbol(*symbol));
	}

	GIVEN("a binary with one function and one variable (GPL) exported")
	{
	  const std::string  binary = base_path + "one_of_each.ko";
	  const corpus_sptr& corpus = assert_symbol_count(binary, 2, 2);
	  CHECK(corpus->lookup_function_symbol("exported_function"));
	  CHECK(!corpus->lookup_variable_symbol("exported_function"));
	  CHECK(corpus->lookup_function_symbol("exported_function_gpl"));
	  CHECK(!corpus->lookup_variable_symbol("exported_function_gpl"));
	  CHECK(corpus->lookup_variable_symbol("exported_variable"));
	  CHECK(!corpus->lookup_function_symbol("exported_variable"));
	  CHECK(corpus->lookup_variable_symbol("exported_variable_gpl"));
	  CHECK(!corpus->lookup_function_symbol("exported_variable_gpl"));
	}
      }
    }
}

TEST_CASE("Symtab::KernelSymtabsWithCRC", "[symtab, crc, kernel, ksymtab]")
{
  const std::string base_path = "kernel-modversions/";

  GIVEN("a binary with one function and one variable (GPL) exported")
  {
    const std::string  binary = base_path + "one_of_each.ko";
    const corpus_sptr& corpus = assert_symbol_count(binary, 2, 2);
    CHECK(corpus->lookup_function_symbol("exported_function")->get_crc());
    CHECK(corpus->lookup_function_symbol("exported_function_gpl")->get_crc());
    CHECK(corpus->lookup_variable_symbol("exported_variable")->get_crc());
    CHECK(corpus->lookup_variable_symbol("exported_variable_gpl")->get_crc());
  }
}

// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2025 Red Hat, Inc.
// Copyright (C) 2020-2025 Google, Inc.
//
// Author: Matthias Maennich

/// @file
///
/// This contains the definition of the symtab reader

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "abg-elf-helpers.h"
#include "abg-fwd.h"
#include "abg-internal.h"
#include "abg-tools-utils.h"

// Though this is an internal header, we need to export the symbols to be able
// to test this code.  TODO: find a way to export symbols just for unit tests.
ABG_BEGIN_EXPORT_DECLARATIONS
#include "abg-symtab-reader.h"
ABG_END_EXPORT_DECLARATIONS

namespace abigail
{

namespace symtab_reader
{

/// symtab_filter implementations

/// Determine whether a symbol is matching the filter criteria of this filter
/// object. In terms of a filter functionality, you would _not_ filter out
/// this symbol if it passes this (i.e. returns true).
///
/// @param symbol The Elf symbol under test.
///
/// @return whether the symbol matches all relevant / required criteria
bool
symtab_filter::matches(const elf_symbol& symbol) const
{
  if (functions_ && *functions_ != symbol.is_function())
    return false;
  if (variables_ && *variables_ != symbol.is_variable())
    return false;
  if (public_symbols_ && *public_symbols_ != symbol.is_public())
    return false;
  if (undefined_symbols_ && *undefined_symbols_ == symbol.is_defined())
    return false;
  if (kernel_symbols_ && *kernel_symbols_ != symbol.is_in_ksymtab())
    return false;

  return true;
}

/// symtab implementations

/// Obtain a suitable default filter for iterating this symtab object.
///
/// The symtab_filter obtained is populated with some sensible default
/// settings, such as public_symbols(true) and kernel_symbols(true) if the
/// binary has been identified as Linux Kernel binary.
///
/// @return a symtab_filter with sensible populated defaults
symtab_filter
symtab::make_filter() const
{
  symtab_filter filter;
  filter.set_public_symbols();
  if (is_kernel_binary_)
    filter.set_kernel_symbols();
  return filter;
}

/// Get a vector of symbols that are associated with a certain name
///
/// @param name the name the symbols need to match
///
/// @return a vector of symbols, empty if no matching symbols have been found
const elf_symbols&
symtab::lookup_symbol(const std::string& name) const
{
  static const elf_symbols empty_result;
  const auto it = name_symbol_map_.find(name);
  if (it != name_symbol_map_.end())
      return it->second;
  return empty_result;
}

/// Lookup a symbol by its address
///
/// @param symbol_addr the starting address of the symbol
///
/// @return a symbol if found, else an empty sptr
const elf_symbol_sptr&
symtab::lookup_symbol(GElf_Addr symbol_addr) const
{
  static const elf_symbol_sptr empty_result;
  const auto addr_it = addr_symbol_map_.find(symbol_addr);
  if (addr_it != addr_symbol_map_.end())
    return addr_it->second;
  else
    {
      // check for a potential entry address mapping instead,
      // relevant for ppc ELFv1 binaries
      const auto entry_it = entry_addr_symbol_map_.find(symbol_addr);
      if (entry_it != entry_addr_symbol_map_.end())
	return entry_it->second;
    }
  return empty_result;
}

/// Lookup an undefined function symbol with a given name.
///
/// @param sym_name the name of the function symbol to lookup.
///
/// @return the undefined function symbol found or nil if none was
/// found.
const elf_symbol_sptr
symtab::lookup_undefined_function_symbol(const std::string& sym_name)
{
  symtab_filter f = make_filter();
  f.set_variables(false);
  f.set_public_symbols(false);
  f.set_functions(true);
  f.set_undefined_symbols(true);

  elf_symbol_sptr result;
  for (auto sym : filtered_symtab(*this, f))
    if (sym_name == sym->get_name())
      {
	result = sym;
	break;
      }

  return result;
}

/// Lookup an undefined variable symbol with a given name.
///
/// @param sym_name the name of the variable symbol to lookup.
///
/// @return the undefined variable symbol found or nil if none was
/// found.
const elf_symbol_sptr
symtab::lookup_undefined_variable_symbol(const std::string& sym_name)
{
  symtab_filter f = make_filter();
  f.set_functions(false);
  f.set_public_symbols(false);
  f.set_undefined_symbols(true);
  f.set_variables(true);

  elf_symbol_sptr result;
  for (auto sym : filtered_symtab(*this, f))
    if (sym_name == sym->get_name())
      {
	result = sym;
	break;
      }
  return result;
}

/// Test if a given function symbol has been exported.
///
/// Note that this doesn't test if the symbol is defined or not, but
/// assumes the symbol is defined.
///
/// @param name the name of the symbol we are looking for.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
symtab::function_symbol_is_exported(const string& name)
{
  const elf_symbols& syms = lookup_symbol(name);
  for (auto s : syms)
    if (s->is_function() && s->is_public())
      return s;

  return elf_symbol_sptr();
}

/// Test if a given function symbol has been exported.
///
/// Note that this doesn't test if the symbol is defined or not, but
/// assumes the symbol is defined.
///
/// @param symbol_address the address of the symbol we are looking
/// for.  Note that this address must be a relative offset from the
/// beginning of the .text section, just like the kind of addresses
/// that are present in the .symtab section.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
symtab::function_symbol_is_exported(const GElf_Addr symbol_address)
{
  elf_symbol_sptr symbol = lookup_symbol(symbol_address);
  if (!symbol)
    return symbol;

  if (!symbol->is_function() || !symbol->is_public())
    return elf_symbol_sptr();

  return symbol;
}

/// Test if a given variable symbol has been exported.
///
/// Note that this assumes the symbol is exported but doesn't test for
/// it.
///
/// @param name the name of the symbol we are looking
/// for.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
symtab::variable_symbol_is_exported(const string& name)
{
  const elf_symbols& syms = lookup_symbol(name);
  for (auto s : syms)
    if (s->is_variable() && s->is_public())
      return s;

  return elf_symbol_sptr();
}

/// Test if a given variable symbol has been exported.
///
/// Note that this assumes the symbol is exported but doesn't test for
/// it.
///
/// @param symbol_address the address of the symbol we are looking
/// for.  Note that this address must be a relative offset from the
/// beginning of the .text section, just like the kind of addresses
/// that are present in the .symtab section.
///
/// @return the elf symbol if found, or nil otherwise.
elf_symbol_sptr
symtab::variable_symbol_is_exported(const GElf_Addr symbol_address)
{
  elf_symbol_sptr symbol = lookup_symbol(symbol_address);
  if (!symbol)
    return symbol;

  if (!symbol->is_variable() || !symbol->is_public())
    return elf_symbol_sptr();

  return symbol;
}

/// Test if a name is a the name of an undefined function symbol.
///
/// @param sym_name the symbol name to consider.
///
/// @return the undefined symbol if found, nil otherwise.
elf_symbol_sptr
symtab::function_symbol_is_undefined(const string& sym_name)
{
  collect_undefined_fns_and_vars_linkage_names();
  if (undefined_function_linkage_names_.count(sym_name))
    {
      elf_symbol_sptr sym = lookup_undefined_function_symbol(sym_name);
      ABG_ASSERT(sym);
      ABG_ASSERT(sym->is_function());
      ABG_ASSERT(!sym->is_defined());
      return sym;
    }
  return elf_symbol_sptr();
}

/// Test if a name is a the name of an undefined variable symbol.
///
/// @param sym_name the symbol name to consider.
///
// @return the undefined symbol if found, nil otherwise.
elf_symbol_sptr
symtab::variable_symbol_is_undefined(const string& sym_name)
{
  collect_undefined_fns_and_vars_linkage_names();
  if (undefined_variable_linkage_names_.count(sym_name))
    {
      elf_symbol_sptr sym = lookup_undefined_variable_symbol(sym_name);
      ABG_ASSERT(sym);
      ABG_ASSERT(sym->is_variable());
      ABG_ASSERT(!sym->is_defined());
      return sym;
    }
  return elf_symbol_sptr();
}

/// A symbol sorting functor.
static struct
{
  bool
  operator()(const elf_symbol_sptr& left, const elf_symbol_sptr& right)
  {return left->get_id_string() < right->get_id_string();}
} symbol_sort;

/// Construct a symtab object and instantiate it from an ELF
/// handle. Also pass in the ir::environment we are living in. If
/// specified, the symbol_predicate will be respected when creating
/// the full vector of symbols.
///
/// @param elf_handle the elf handle to load the symbol table from
///
/// @param env the environment we are operating in
///
/// @param is_suppressed a predicate function to determine if a symbol should
/// be suppressed
///
/// @return a smart pointer handle to symtab, set to nullptr if the load was
/// not completed
symtab_ptr
symtab::load(Elf*	      elf_handle,
	     const ir::environment& env,
	     symbol_predicate is_suppressed)
{
  ABG_ASSERT(elf_handle);

  symtab_ptr result(new symtab);
  if (!result->load_(elf_handle, env, is_suppressed))
    return {};

  return result;
}

/// Construct a symtab object from existing name->symbol lookup maps.
/// They were possibly read from a different representation (XML maybe).
///
/// @param function_symbol_map a map from ELF function name to elf_symbol
///
/// @param variable_symbol_map a map from ELF variable name to elf_symbol
///
/// @return a smart pointer handle to symtab, set to nullptr if the load was
/// not completed
symtab_ptr
symtab::load(string_elf_symbols_map_sptr function_symbol_map,
	     string_elf_symbols_map_sptr variables_symbol_map)
{
  symtab_ptr result(new symtab);
  if (!result->load_(function_symbol_map, variables_symbol_map))
    return {};

  return result;
}

/// Default constructor of the @ref symtab type.
symtab::symtab()
  : is_kernel_binary_(false), has_ksymtab_entries_(false),
    cached_undefined_symbol_names_(false)
{}

/// Load the symtab representation from an Elf binary presented to us by an
/// Elf* handle.
///
/// This method iterates over the entries of .symtab and collects all
/// interesting symbols (functions and variables).
///
/// In case of a Linux Kernel binary, it also collects information about the
/// symbols exported via EXPORT_SYMBOL in the Kernel that would then end up
/// having a corresponding __ksymtab entry.
///
/// Symbols that are suppressed will be omitted from the symbols_ vector, but
/// still be discoverable through the name->symbol and addr->symbol lookup
/// maps.
///
/// @param elf_handle the elf handle to load the symbol table from
///
/// @param env the environment we are operating in
///
/// @param is_suppressed a predicate function to determine if a symbol should
/// be suppressed
///
/// @return true if the load succeeded
bool
symtab::load_(Elf*	       elf_handle,
	      const ir::environment& env,
	      symbol_predicate is_suppressed)
{
  GElf_Ehdr ehdr_mem;
  GElf_Ehdr* header = gelf_getehdr(elf_handle, &ehdr_mem);
  if (!header)
    {
      std::cerr << "Could not get ELF header: Skipping symtab load.\n";
      return false;
    }

  Elf_Scn* symtab_section = elf_helpers::find_symbol_table_section(elf_handle);
  if (!symtab_section)
    {
      std::cerr << "No symbol table found: Skipping symtab load.\n";
      return false;
    }

  GElf_Shdr symtab_sheader;
  gelf_getshdr(symtab_section, &symtab_sheader);

  // check for bogus section header
  if (symtab_sheader.sh_entsize == 0)
    {
      std::cerr << "Invalid symtab header found: Skipping symtab load.\n";
      return false;
    }

  const size_t number_syms =
      symtab_sheader.sh_size / symtab_sheader.sh_entsize;

  Elf_Data* symtab = elf_getdata(symtab_section, 0);
  if (!symtab)
    {
      std::cerr << "Could not load elf symtab: Skipping symtab load.\n";
      return false;
    }

  // The __kstrtab_strings sections is basically an ELF strtab but does not
  // support elf_strptr lookups. A single call to elf_getdata gives a handle to
  // washed section data.
  //
  // The value of a __kstrtabns_FOO (or other similar) symbol is an address
  // within the __kstrtab_strings section. To look up the string value, we need
  // to translate from vmlinux load address to section offset by subtracting the
  // base address of the section. This adjustment is not needed for loadable
  // modules which are relocatable and so identifiable by ELF type ET_REL.
  Elf_Scn* strings_section = elf_helpers::find_ksymtab_strings_section(elf_handle);
  size_t strings_offset = 0;
  const char* strings_data = nullptr;
  size_t strings_size = 0;
  if (strings_section)
    {
      GElf_Shdr strings_sheader;
      gelf_getshdr(strings_section, &strings_sheader);
      strings_offset = header->e_type == ET_REL ? 0 : strings_sheader.sh_addr;
      Elf_Data* data = elf_getdata(strings_section, nullptr);
      ABG_ASSERT(data->d_off == 0);
      strings_data = reinterpret_cast<const char *>(data->d_buf);
      strings_size = data->d_size;
    }

  const bool is_kernel = elf_helpers::is_linux_kernel(elf_handle);
  std::unordered_set<std::string> exported_kernel_symbols;
  std::unordered_map<std::string, uint32_t> crc_values;
  std::unordered_map<std::string, std::string> namespaces;

  for (size_t i = 0; i < number_syms; ++i)
    {
      GElf_Sym *sym, sym_mem;
      sym = gelf_getsym(symtab, i, &sym_mem);
      if (!sym)
	{
	  std::cerr << "Could not load symbol with index " << i
		    << ": Skipping symtab load.\n";
	  return false;
	}

      const char* const name_str =
	  elf_strptr(elf_handle, symtab_sheader.sh_link, sym->st_name);

      // no name, no game
      if (!name_str)
	continue;

      const std::string name = name_str;
      if (name.empty())
	continue;

      // Handle ksymtab entries. Every symbol entry that starts with __ksymtab_
      // indicates that the symbol in question is exported through ksymtab. We
      // do not know whether this is ksymtab_gpl or ksymtab, but that is good
      // enough for now.
      //
      // We could follow up with this entry:
      //
      // symbol_value -> ksymtab_entry in either ksymtab_gpl or ksymtab
      //              -> addr/name/namespace (in case of PREL32: offset)
      //
      // That way we could also detect ksymtab<>ksymtab_gpl changes or changes
      // of the symbol namespace.
      //
      // As of now this lookup is fragile, as occasionally ksymtabs are empty
      // (seen so far for kernel modules and LTO builds). Hence we stick to the
      // fairly safe assumption that ksymtab exported entries are having an
      // appearence as __ksymtab_<symbol> in the symtab.
      if (is_kernel && name.rfind("__ksymtab_", 0) == 0)
	{
	  ABG_ASSERT(exported_kernel_symbols.insert(name.substr(10)).second);
	  continue;
	}
      if (is_kernel && name.rfind("__crc_", 0) == 0)
	{
	  uint32_t crc_value;
	  ABG_ASSERT(elf_helpers::get_crc_for_symbol(elf_handle,
						     sym, crc_value));
	  ABG_ASSERT(crc_values.emplace(name.substr(6), crc_value).second);
	  continue;
	}
      if (strings_section && is_kernel && name.rfind("__kstrtabns_", 0) == 0)
	{
	  // This symbol lives in the __ksymtab_strings section but st_value may
	  // be a vmlinux load address so we need to subtract the offset before
	  // looking it up in that section.
	  const size_t value = sym->st_value;
	  const size_t offset = value - strings_offset;
	  // check offset
	  ABG_ASSERT(offset < strings_size);
	  // find the terminating NULL
	  const char* first = strings_data + offset;
	  const char* last = strings_data + strings_size;
	  const char* limit = std::find(first, last, 0);
	  // check NULL found
	  ABG_ASSERT(limit < last);
	  // interpret the empty namespace name as no namespace name
	  if (first < limit)
	    ABG_ASSERT(namespaces.emplace(
		name.substr(12), std::string(first, limit - first)).second);
	  continue;
	}

      // filter out uninteresting entries and only keep functions/variables for
      // now. The rest might be interesting in the future though.
      const int sym_type = GELF_ST_TYPE(sym->st_info);
      if (!(sym_type == STT_FUNC
	    || sym_type == STT_GNU_IFUNC
	    // If the symbol is for an OBJECT, the index of the
	    // section it refers to cannot be absolute.
	    // Otherwise that OBJECT is not a variable.
	    || (sym_type == STT_OBJECT && sym->st_shndx != SHN_ABS)
	    // Undefined global variable symbols have symbol type
	    // STT_NOTYPE.  No idea why.
	    || (sym_type == STT_NOTYPE && sym->st_shndx == SHN_UNDEF)
	    || sym_type == STT_TLS))
	continue;

      const bool sym_is_defined = sym->st_shndx != SHN_UNDEF;
      // this occurs in relocatable files.
      const bool sym_is_common = sym->st_shndx == SHN_COMMON;

      elf_symbol::version ver;
      elf_helpers::get_version_for_symbol(elf_handle, i, sym_is_defined, ver);

      const elf_symbol_sptr& symbol_sptr =
	elf_symbol::create
	(env, i, sym->st_size, name,
	 elf_helpers::stt_to_elf_symbol_type(GELF_ST_TYPE(sym->st_info)),
	 elf_helpers::stb_to_elf_symbol_binding(GELF_ST_BIND(sym->st_info)),
	 sym_is_defined, sym_is_common, ver,
	 elf_helpers::stv_to_elf_symbol_visibility
	 (GELF_ST_VISIBILITY(sym->st_other)));

      // We do not take suppressed symbols into our symbol vector to avoid
      // accidental leakage. But we ensure supressed symbols are otherwise set
      // up for lookup.
      if (!(is_suppressed && is_suppressed(symbol_sptr)))
	// add to the symbol vector
	symbols_.push_back(symbol_sptr);
      else
	symbol_sptr->set_is_suppressed(true);

      // add to the name->symbol lookup
      name_symbol_map_[name].push_back(symbol_sptr);

      // add to the addr->symbol lookup
      if (symbol_sptr->is_common_symbol())
	{
	  const auto it = name_symbol_map_.find(name);
	  ABG_ASSERT(it != name_symbol_map_.end());
	  const elf_symbols& common_sym_instances = it->second;
	  ABG_ASSERT(!common_sym_instances.empty());
	  if (common_sym_instances.size() > 1)
	    {
	      elf_symbol_sptr main_common_sym = common_sym_instances[0];
	      ABG_ASSERT(main_common_sym->get_name() == name);
	      ABG_ASSERT(main_common_sym->is_common_symbol());
	      ABG_ASSERT(symbol_sptr.get() != main_common_sym.get());
	      main_common_sym->add_common_instance(symbol_sptr);
	    }
	}
      else if (symbol_sptr->is_defined())
	setup_symbol_lookup_tables(elf_handle, sym, symbol_sptr);
    }

  add_alternative_address_lookups(elf_handle);

  is_kernel_binary_ = elf_helpers::is_linux_kernel(elf_handle);

  // Now apply the ksymtab_exported attribute to the symbols we collected.
  for (const auto& symbol : exported_kernel_symbols)
    {
      const auto r = name_symbol_map_.find(symbol);
      if (r == name_symbol_map_.end())
	continue;

      for (const auto& elf_symbol : r->second)
	  if (elf_symbol->is_public())
	    elf_symbol->set_is_in_ksymtab(true);
      has_ksymtab_entries_ = true;
    }

  // Now add the CRC values
  for (const auto& crc_entry : crc_values)
    {
      const auto r = name_symbol_map_.find(crc_entry.first);
      if (r == name_symbol_map_.end())
	continue;

      for (const auto& symbol : r->second)
	symbol->set_crc(crc_entry.second);
    }

  // Now add the namespaces
  for (const auto& namespace_entry : namespaces)
    {
      const auto r = name_symbol_map_.find(namespace_entry.first);
      if (r == name_symbol_map_.end())
	continue;

      for (const auto& symbol : r->second)
	symbol->set_namespace(namespace_entry.second);
    }

  // sort the symbols for deterministic output
  std::sort(symbols_.begin(), symbols_.end(), symbol_sort);

  return true;
}

/// Load the symtab representation from a function/variable lookup map pair.
///
/// This method assumes the lookup maps are correct and sets up the data
/// vector as well as the name->symbol lookup map. The addr->symbol lookup
/// map cannot be set up in this case.
///
/// @param function_symbol_map a map from ELF function name to elf_symbol
///
/// @param variable_symbol_map a map from ELF variable name to elf_symbol
///
/// @return true if the load succeeded
bool
symtab::load_(string_elf_symbols_map_sptr function_symbol_map,
	     string_elf_symbols_map_sptr variables_symbol_map)

{
  if (function_symbol_map)
    for (const auto& symbol_map_entry : *function_symbol_map)
      {
	for (const auto& symbol : symbol_map_entry.second)
	  {
	    if (!symbol->is_suppressed())
	      symbols_.push_back(symbol);
	  }
	ABG_ASSERT(name_symbol_map_.insert(symbol_map_entry).second);
      }

  if (variables_symbol_map)
    for (const auto& symbol_map_entry : *variables_symbol_map)
      {
	for (const auto& symbol : symbol_map_entry.second)
	  {
	    if (!symbol->is_suppressed())
	      symbols_.push_back(symbol);
	  }
	ABG_ASSERT(name_symbol_map_.insert(symbol_map_entry).second);
      }

  // sort the symbols for deterministic output
  std::sort(symbols_.begin(), symbols_.end(), symbol_sort);

  return true;
}

/// Notify the symtab about the name of the main symbol at a given address.
///
/// From just alone the symtab we can't guess the main symbol of a bunch of
/// aliased symbols that all point to the same address. During processing of
/// additional information (such as DWARF), this information becomes apparent
/// and we can adjust the addr->symbol lookup map as well as the alias
/// reference of the symbol objects.
///
/// @param addr the addr that we are updating the main symbol for
/// @param name the name of the main symbol
void
symtab::update_main_symbol(GElf_Addr addr, const std::string& name)
{
  // get one symbol (i.e. the current main symbol)
  elf_symbol_sptr symbol = lookup_symbol(addr);

  // The caller might not know whether the addr is associated to an ELF symbol
  // that we care about. E.g. the addr could be associated to an ELF symbol,
  // but not one in .dynsym when looking at a DSO. Hence, early exit if the
  // lookup failed.
  if (!symbol)
    return;

  // determine the new main symbol by attempting an update
  elf_symbol_sptr new_main = symbol->update_main_symbol(name);

  // also update the default symbol we return when looked up by address
  if (new_main)
    addr_symbol_map_[addr] = new_main;
}

/// Various adjustments and bookkeeping may be needed to provide a correct
/// interpretation (one that matches DWARF addresses) of raw symbol values.
///
/// This is a sub-routine for symtab::load_and
/// symtab::add_alternative_address_lookups and must be called only
/// once (per symbol) during the execution of the former.
///
/// @param elf_handle the ELF handle
///
/// @param elf_symbol the ELF symbol
///
/// @param symbol_sptr the libabigail symbol
///
/// @return a possibly-adjusted symbol value
GElf_Addr
symtab::setup_symbol_lookup_tables(Elf* elf_handle,
				   GElf_Sym* elf_symbol,
				   const elf_symbol_sptr& symbol_sptr)
{
  const bool is_arm32 = elf_helpers::architecture_is_arm32(elf_handle);
  const bool is_arm64 = elf_helpers::architecture_is_arm64(elf_handle);
  const bool is_ppc64 = elf_helpers::architecture_is_ppc64(elf_handle);
  const bool is_ppc32 = elf_helpers::architecture_is_ppc32(elf_handle);

  GElf_Addr symbol_value =
    elf_helpers::maybe_adjust_et_rel_sym_addr_to_abs_addr(elf_handle,
							  elf_symbol);

  if (is_arm32 && symbol_sptr->is_function())
    // Clear bit zero of ARM32 addresses as per "ELF for the Arm
    // Architecture" section 5.5.3.
    // https://static.docs.arm.com/ihi0044/g/aaelf32.pdf
    symbol_value &= ~1;

  if (is_arm64)
    // Copy bit 55 over bits 56 to 63 which may be tag information.
    symbol_value = symbol_value & (1ULL<<55)
		   ? symbol_value | (0xffULL<<56)
		   : symbol_value &~ (0xffULL<<56);

  if (symbol_sptr->is_defined())
    {
      const auto result =
	addr_symbol_map_.emplace(symbol_value, symbol_sptr);
      if (!result.second)
	// A symbol with the same address already exists.  This
	// means this symbol is an alias of the main symbol with
	// that address.  So let's register this new alias as such.
	result.first->second->get_main_symbol()->add_alias(symbol_sptr);
    }

  // Please note that update_function_entry_address_symbol_map depends
  // on the symbol aliases been setup.  This is why, the
  // elf_symbol::add_alias call is done above BEFORE this point.
  if ((is_ppc64 || is_ppc32) && symbol_sptr->is_function())
    update_function_entry_address_symbol_map(elf_handle, elf_symbol,
					     symbol_sptr);

  return symbol_value;
}

/// Update the function entry symbol map to later allow lookups of this symbol
/// by entry address as well. This is relevant for ppc64 ELFv1 binaries.
///
/// For ppc64 ELFv1 binaries, we need to build a function entry point address
/// -> function symbol map. This is in addition to the function pointer ->
/// symbol map.  This is because on ppc64 ELFv1, a function pointer is
/// different from a function entry point address.
///
/// On ppc64 ELFv1, the DWARF DIE of a function references the address of the
/// entry point of the function symbol; whereas the value of the function
/// symbol is the function pointer. As these addresses are different, if I we
/// want to get to the symbol of a function from its entry point address (as
/// referenced by DWARF function DIEs) we must have the two maps I mentionned
/// right above.
///
/// In other words, we need a map that associates a function entry point
/// address with the symbol of that function, to be able to get the function
/// symbol that corresponds to a given function DIE, on ppc64.
///
/// The value of the function pointer (the value of the symbol) usually refers
/// to the offset of a table in the .opd section.  But sometimes, for a symbol
/// named "foo", the corresponding symbol named ".foo" (note the dot before
/// foo) which value is the entry point address of the function; that entry
/// point address refers to a region in the .text section.
///
/// So we are only interested in values of the symbol that are in the .opd
/// section.
///
/// @param elf_handle the ELF handle to operate on
///
/// @param native_symbol the native Elf symbol to update the entry for
///
/// @param symbol_sptr the internal symbol to associte the entry address with
void
symtab::update_function_entry_address_symbol_map(
  Elf* elf_handle, GElf_Sym* native_symbol, const elf_symbol_sptr& symbol_sptr)
{
  const GElf_Addr fn_desc_addr = native_symbol->st_value;
  const GElf_Addr fn_entry_point_addr =
    elf_helpers::lookup_ppc64_elf_fn_entry_point_address(elf_handle,
							 fn_desc_addr);

  const std::pair<addr_symbol_map_type::const_iterator, bool>& result =
    entry_addr_symbol_map_.emplace(fn_entry_point_addr, symbol_sptr);

  const addr_symbol_map_type::const_iterator it = result.first;
  const bool was_inserted = result.second;
  if (!was_inserted
      && elf_helpers::address_is_in_opd_section(elf_handle, fn_desc_addr))
    {
      // Either
      //
      // 'symbol' must have been registered as an alias for
      // it->second->get_main_symbol()
      //
      // Or
      //
      // if the name of 'symbol' is foo, then the name of it2->second is
      // ".foo". That is, foo is the name of the symbol when it refers to the
      // function descriptor in the .opd section and ".foo" is an internal name
      // for the address of the entry point of foo.
      //
      // In the latter case, we just want to keep a reference to "foo" as .foo
      // is an internal name.

      const bool two_symbols_alias =
	it->second->get_main_symbol()->does_alias(*symbol_sptr);
      const bool symbol_is_foo_and_prev_symbol_is_dot_foo =
	(it->second->get_name() == std::string(".") + symbol_sptr->get_name());

      ABG_ASSERT(two_symbols_alias
		 || symbol_is_foo_and_prev_symbol_is_dot_foo);

      if (symbol_is_foo_and_prev_symbol_is_dot_foo)
	// Let's just keep a reference of the symbol that the user sees in the
	// source code (the one named foo). The symbol which name is prefixed
	// with a "dot" is an artificial one.
	entry_addr_symbol_map_[fn_entry_point_addr] = symbol_sptr;
    }
}

/// Fill up the lookup maps with alternative keys
///
/// Due to special features like Control-Flow-Integrity (CFI), the symbol
/// lookup could be done indirectly. E.g. enabling CFI causes clang to
/// associate the DWARF information with the actual CFI protected function
/// (suffix .cfi) instead of with the entry symbol in the symtab.
///
/// This function adds additional lookup keys to compensate for that.
///
/// So far, this only implements CFI support, by adding addr->symbol pairs
/// where
///    addr   :  symbol value of the <foo>.cfi value
///    symbol :  symbol_sptr looked up via "<foo>"
///
/// @param elf_handle the ELF handle to operate on
void
symtab::add_alternative_address_lookups(Elf* elf_handle)
{
  Elf_Scn* symtab_section = elf_helpers::find_symtab_section(elf_handle);
  if (!symtab_section)
    return;
  GElf_Shdr symtab_sheader;
  gelf_getshdr(symtab_section, &symtab_sheader);

  const size_t number_syms =
      symtab_sheader.sh_size / symtab_sheader.sh_entsize;

  Elf_Data* symtab = elf_getdata(symtab_section, 0);

  for (size_t i = 0; i < number_syms; ++i)
    {
      GElf_Sym *sym, sym_mem;
      sym = gelf_getsym(symtab, i, &sym_mem);
      if (!sym)
	{
	  std::cerr << "Could not load symbol with index " << i
		    << ": Skipping alternative symbol load.\n";
	  continue;
	}

      const char* const name_str =
	  elf_strptr(elf_handle, symtab_sheader.sh_link, sym->st_name);

      // no name, no game
      if (!name_str)
	continue;

      const std::string name = name_str;
      if (name.empty())
	continue;

      // Add alternative lookup addresses for CFI symbols
      static const std::string cfi = ".cfi";
      if (name.size() > cfi.size()
	  && name.compare(name.size() - cfi.size(), cfi.size(), cfi) == 0)
	// ... name.ends_with(".cfi")
	{
	  const auto candidate_name = name.substr(0, name.size() - cfi.size());

	  auto symbols = lookup_symbol(candidate_name);
          // lookup_symbol returns a vector of symbols. For this case we handle
          // only the case that there has been exactly one match. Otherwise we
          // can't reasonably handle it and need to bail out.
          ABG_ASSERT(symbols.size() <= 1);
	  if (symbols.size() == 1)
	    {
	      const auto& symbol_sptr = symbols[0];
		setup_symbol_lookup_tables(elf_handle, sym, symbol_sptr);
	    }
	}
    }
}

/// Collect the names of the variable and function symbols that are
/// undefined.  Cache those names into sets to speed up their lookup.
///
/// Once the names are cached into sets, subsequent invocations of
/// this function are essentially a no-op.
void
symtab::collect_undefined_fns_and_vars_linkage_names()
{
  if (!cached_undefined_symbol_names_)
    {
      {
	symtab_filter f = make_filter();
	f.set_variables(false);
	f.set_functions(true);
	f.set_public_symbols(false);
	f.set_undefined_symbols(true);
	for (auto sym : filtered_symtab(*this, f))
	  undefined_function_linkage_names_.insert(sym->get_name());
      }

      {
	symtab_filter f = make_filter();
	f.set_variables(true);
	f.set_functions(false);
	f.set_public_symbols(false);
	f.set_undefined_symbols(true);
	for (auto sym : filtered_symtab(*this, f))
	  undefined_variable_linkage_names_.insert(sym->get_name());
      }
    }
  cached_undefined_symbol_names_ = true;
}
} // end namespace symtab_reader
} // end namespace abigail

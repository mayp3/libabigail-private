// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// -*- Mode: C++ -*-
//
// Copyright (C) 2014-2025 Red Hat, Inc.
//
// Author: Dodji Seketeli

/// @file
///
/// Given a program P that links against a library L of version V
/// denoted L(V), this program checks if P is still ABI compatible
/// with a subsequent version of L denoted L(V+N), N being a positive
/// integer.  The result of the check is a report that is compared
/// against a reference report.  This program actually performs these
/// checks for a variety of tuple {P, L(V), L(V+N)}
///
/// The set of input files and reference reports to consider should be
/// present in the source distribution.

#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include "abg-tools-utils.h"
#include "test-utils.h"

using std::string;
using std::cerr;
using std::cout;
using abigail::tools_utils::abidiff_status;

struct InOutSpec
{
  const char* in_app_path;
  const char* in_lib1_path;
  const char* in_lib2_path;
  const char* suppressions;
  const char* options;
  abidiff_status status;
  const char* in_report_path;
  const char* out_report_path;
};

InOutSpec in_out_specs[] =
{
  {
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test0-fn-changed-report-0.txt",
    "output/test-abicompat/test0-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "data/test-abicompat/test0-fn-changed-0.suppr",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abicompat/test0-fn-changed-report-1.txt",
    "output/test-abicompat/test0-fn-changed-report-1.txt",
  },
  { // Previous test, but emitting loc info.
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "",
    "--show-base-names --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test0-fn-changed-report-2.txt",
    "output/test-abicompat/test0-fn-changed-report-2.txt",
  },
  {
    "data/test-abicompat/test0-fn-changed-app",
    "data/test-abicompat/libtest0-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest0-fn-changed-libapp-v1.so",
    "data/test-abicompat/test0-fn-changed-1.suppr",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abicompat/test0-fn-changed-report-3.txt",
    "output/test-abicompat/test0-fn-changed-report-3.txt",
  },
  {
    "data/test-abicompat/test1-fn-removed-app",
    "data/test-abicompat/libtest1-fn-removed-v0.so",
    "data/test-abicompat/libtest1-fn-removed-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abicompat/test1-fn-removed-report-0.txt",
    "output/test-abicompat/test1-fn-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test2-var-removed-app",
    "data/test-abicompat/libtest2-var-removed-v0.so",
    "data/test-abicompat/libtest2-var-removed-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abicompat/test2-var-removed-report-0.txt",
    "output/test-abicompat/test2-var-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test3-fn-removed-app",
    "data/test-abicompat/libtest3-fn-removed-v0.so",
    "data/test-abicompat/libtest3-fn-removed-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abicompat/test3-fn-removed-report-0.txt",
    "output/test-abicompat/test3-fn-removed-report-0.txt",
  },
  {
    "data/test-abicompat/test4-soname-changed-app",
    "data/test-abicompat/libtest4-soname-changed-v0.so",
    "data/test-abicompat/libtest4-soname-changed-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE
    | abigail::tools_utils::ABIDIFF_ABI_INCOMPATIBLE_CHANGE,
    "data/test-abicompat/test4-soname-changed-report-0.txt",
    "output/test-abicompat/test4-soname-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test5-fn-changed-app",
    "data/test-abicompat/libtest5-fn-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --no-show-locs --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test5-fn-changed-report-0.txt",
    "output/test-abicompat/test5-fn-changed-report-0.txt",
  },
  { // Previous test, but emitting loc info.
    "data/test-abicompat/test5-fn-changed-app",
    "data/test-abicompat/libtest5-fn-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test5-fn-changed-report-1.txt",
    "output/test-abicompat/test5-fn-changed-report-1.txt",
  },
  {
    "data/test-abicompat/libtest6-undefined-var.so",
    "data/test-abicompat/libtest6-var-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --no-show-locs --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test6-var-changed-report-0.txt",
    "output/test-abicompat/test6-var-changed-report-0.txt",
  },
  { // Previous test, but emitting loc info.
    "data/test-abicompat/libtest6-undefined-var.so",
    "data/test-abicompat/libtest6-var-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test6-var-changed-report-1.txt",
    "output/test-abicompat/test6-var-changed-report-1.txt",
  },
  { // Previous test, but in reverse direction.
    "data/test-abicompat/libtest6-var-changed-libapp-v1.so",
    "data/test-abicompat/libtest6-undefined-var.so",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test6-var-changed-report-2.txt",
    "output/test-abicompat/test6-var-changed-report-2.txt",
  },
  {
    "data/test-abicompat/test7-fn-changed-app",
    "data/test-abicompat/libtest7-fn-changed-libapp-v0.so",
    "data/test-abicompat/libtest7-fn-changed-libapp-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test7-fn-changed-report-0.txt",
    "output/test-abicompat/test7-fn-changed-report-0.txt",
  },
#ifdef WITH_BTF
  {
    "data/test-abicompat/test7-fn-changed-app.btf",
    "data/test-abicompat/libtest7-fn-changed-libapp-btf-v0.so",
    "data/test-abicompat/libtest7-fn-changed-libapp-btf-v1.so",
    "",
    "--show-base-names --no-show-locs --no-redundant --btf",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test7-fn-changed-report-0.1.txt",
    "output/test-abicompat/test7-fn-changed-report-0.1.txt",
  },
#endif
  {
    "data/test-abicompat/test7-fn-changed-app",
    "data/test-abicompat/libtest7-fn-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --no-show-locs --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test7-fn-changed-report-1.txt",
    "output/test-abicompat/test7-fn-changed-report-1.txt",
  },
  { // Previous test, but emitting loc info.
    "data/test-abicompat/test7-fn-changed-app",
    "data/test-abicompat/libtest7-fn-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test7-fn-changed-report-2.txt",
    "output/test-abicompat/test7-fn-changed-report-2.txt",
  },
#ifdef WITH_BTF
  {
    "data/test-abicompat/test7-fn-changed-app.btf",
    "data/test-abicompat/libtest7-fn-changed-libapp-btf-v1.so",
    "",
    "",
    "--show-base-names --no-show-locs --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test7-fn-changed-report-2.1.txt",
    "output/test-abicompat/test7-fn-changed-report-2.1.txt",
  },
#endif
  {
    "data/test-abicompat/test8-fn-changed-app",
    "data/test-abicompat/libtest8-fn-changed-libapp-v1.so",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test8-fn-changed-report-0.txt",
    "output/test-abicompat/test8-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test9-fn-changed-app",
    "data/test-abicompat/libtest9-fn-changed-v1.so ",
    "",
    "",
    "--show-base-names --weak-mode",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test9-fn-changed-report-0.txt",
    "output/test-abicompat/test9-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols",
    "data/test-abicompat/test10/libtest10-with-exported-symbols.so",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abicompat/test10/test10-fn-changed-report-0.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols.abi",
    "data/test-abicompat/test10/libtest10-with-exported-symbols.so",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_OK,
    "data/test-abicompat/test10/test10-fn-changed-report-0.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-0.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols",
    "data/test-abicompat/test10/libtest10-with-incompatible-exported-symbols.so",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test10/test10-fn-changed-report-1.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-1.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols",
    "data/test-abicompat/test10/libtest10-with-incompatible-exported-symbols.so.abi",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test10/test10-fn-changed-report-2.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-2.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols.abi",
    "data/test-abicompat/test10/libtest10-with-incompatible-exported-symbols.so",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test10/test10-fn-changed-report-3.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-3.txt",
  },
  {
    "data/test-abicompat/test10/test10-app-with-undefined-symbols.abi",
    "data/test-abicompat/test10/libtest10-with-incompatible-exported-symbols.so.abi",
    "",
    "",
    "--show-base-names",
    abigail::tools_utils::ABIDIFF_ABI_CHANGE,
    "data/test-abicompat/test10/test10-fn-changed-report-4.txt",
    "output/test-abicompat/test10/test10-fn-changed-report-4.txt",
  },
  // This entry must be the last one.
  {0, 0, 0, 0, 0, abigail::tools_utils::ABIDIFF_OK, 0, 0}
};

int
main()
{
  using abigail::tests::get_src_dir;
  using abigail::tests::get_build_dir;
  using abigail::tools_utils::ensure_parent_dir_created;
  using abigail::tools_utils::abidiff_status;

  unsigned int cnt_total = 0, cnt_passed = 0, cnt_failed = 0;
  string in_app_path, in_lib1_path, in_lib2_path, suppression_path,
    abicompat_options, ref_report_path, out_report_path, abicompat, cmd, diffcmd;

  for (InOutSpec* s = in_out_specs; s->in_app_path; ++s)
    {
      bool is_ok = true;
      in_app_path = string(get_src_dir()) + "/tests/" + s->in_app_path;
      in_lib1_path = string(get_src_dir()) + "/tests/" + s->in_lib1_path;
      if (s->in_lib2_path && strcmp(s->in_lib2_path, ""))
	in_lib2_path = string(get_src_dir()) + "/tests/" + s->in_lib2_path;
      else
	in_lib2_path.clear();
      if (s->suppressions == 0 || !strcmp(s->suppressions, ""))
	suppression_path.clear();
      else
	suppression_path = string(get_src_dir()) + "/tests/" + s->suppressions;
      abicompat_options = s->options;
      ref_report_path = string(get_src_dir()) + "/tests/" + s->in_report_path;
      out_report_path =
	string(get_build_dir()) + "/tests/" + s->out_report_path;

      if (!ensure_parent_dir_created(out_report_path))
	{
	  cerr << "could not create parent directory for "
	       << out_report_path;
	  is_ok = false;
	  continue;
	}

      abicompat = string(get_build_dir()) + "/tools/abicompat";
      if (!suppression_path.empty())
	abicompat += " --suppressions " + suppression_path;
      abicompat += " " + abicompat_options;

      cmd = abicompat + " " + in_app_path + " " + in_lib1_path;
      if (!in_lib2_path.empty())
	cmd += string(" ") + in_lib2_path;

      cmd += " > " + out_report_path;

      bool abicompat_ok = true;
      int code = system(cmd.c_str());
      abidiff_status status = static_cast<abidiff_status>(WEXITSTATUS(code));
      if (status != s->status)
	abicompat_ok = false;

      if (abicompat_ok)
	{
	  diffcmd = "diff -u " + ref_report_path + " " + out_report_path;
	  if (system(diffcmd.c_str()))
	    is_ok = false;
	}
      else
	is_ok = false;

      if (is_ok)
	cnt_passed++;
      else
        {
	  cout << BRIGHT_RED_COLOR
               << "Test Failed:"
               << DEFAULT_TERMINAL_COLOR
               << cmd
               << std::endl;
	  if (status != s->status)
	    cout << BRIGHT_RED_COLOR
		 << "expected abicompat exit code: " << s->status << ", got: " << status << std::endl;
	  cnt_failed++;
	}
      cnt_total++;
    }
  cout << "Summary: " << cnt_total << " tested!"
       << " Test Passed: " << cnt_passed
       << ", Test Failed: " << cnt_failed
       << ".\n";

  return cnt_failed;
}

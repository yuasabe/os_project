/* Declarations and definitions of codes relating to the DWARF2 symbolic
   debugging information format.
   Copyright (C) 1992, 1993, 1995, 1996, 1997, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.

   Written by Gary Funck (gary@intrepid.com) The Ada Joint Program
   Office (AJPO), Florida State Unviversity and Silicon Graphics Inc.
   provided support for this effort -- June 21, 1995.

   Derived from the DWARF 1 implementation written by Ron Guilmette
   (rfg@netcom.com), November 1990.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */

/* This file is derived from the DWARF specification (a public document)
   Revision 2.0.0 (July 27, 1993) developed by the UNIX International
   Programming Languages Special Interest Group (UI/PLSIG) and distributed
   by UNIX International.  Copies of this specification are available from
   UNIX International, 20 Waterview Boulevard, Parsippany, NJ, 07054.

   This file also now contains definitions from the DWARF 3 specification.  */

/* This file is shared between GCC and GDB, and should not contain
   prototypes.  */

/* Tag names and codes.  */
enum dwarf_tag
  {
    DW_TAG_padding = 0x00,
    DW_TAG_array_type = 0x01,
    DW_TAG_class_type = 0x02,
    DW_TAG_entry_point = 0x03,
    DW_TAG_enumeration_type = 0x04,
    DW_TAG_formal_parameter = 0x05,
    DW_TAG_imported_declaration = 0x08,
    DW_TAG_label = 0x0a,
    DW_TAG_lexical_block = 0x0b,
    DW_TAG_member = 0x0d,
    DW_TAG_pointer_type = 0x0f,
    DW_TAG_reference_type = 0x10,
    DW_TAG_compile_unit = 0x11,
    DW_TAG_string_type = 0x12,
    DW_TAG_structure_type = 0x13,
    DW_TAG_subroutine_type = 0x15,
    DW_TAG_typedef = 0x16,
    DW_TAG_union_type = 0x17,
    DW_TAG_unspecified_parameters = 0x18,
    DW_TAG_variant = 0x19,
    DW_TAG_common_block = 0x1a,
    DW_TAG_common_inclusion = 0x1b,
    DW_TAG_inheritance = 0x1c,
    DW_TAG_inlined_subroutine = 0x1d,
    DW_TAG_module = 0x1e,
    DW_TAG_ptr_to_member_type = 0x1f,
    DW_TAG_set_type = 0x20,
    DW_TAG_subrange_type = 0x21,
    DW_TAG_with_stmt = 0x22,
    DW_TAG_access_declaration = 0x23,
    DW_TAG_base_type = 0x24,
    DW_TAG_catch_block = 0x25,
    DW_TAG_const_type = 0x26,
    DW_TAG_constant = 0x27,
    DW_TAG_enumerator = 0x28,
    DW_TAG_file_type = 0x29,
    DW_TAG_friend = 0x2a,
    DW_TAG_namelist = 0x2b,
    DW_TAG_namelist_item = 0x2c,
    DW_TAG_packed_type = 0x2d,
    DW_TAG_subprogram = 0x2e,
    DW_TAG_template_type_param = 0x2f,
    DW_TAG_template_value_param = 0x30,
    DW_TAG_thrown_type = 0x31,
    DW_TAG_try_block = 0x32,
    DW_TAG_variant_part = 0x33,
    DW_TAG_variable = 0x34,
    DW_TAG_volatile_type = 0x35,
    /* DWARF 3.  */
    DW_TAG_dwarf_procedure = 0x36,
    DW_TAG_restrict_type = 0x37,
    DW_TAG_interface_type = 0x38,
    DW_TAG_namespace = 0x39,
    DW_TAG_imported_module = 0x3a,
    DW_TAG_unspecified_type = 0x3b,
    DW_TAG_partial_unit = 0x3c,
    DW_TAG_imported_unit = 0x3d,
    /* SGI/MIPS Extensions.  */
    DW_TAG_MIPS_loop = 0x4081,
    /* GNU extensions.  */
    DW_TAG_format_label = 0x4101,	/* For FORTRAN 77 and Fortran 90.  */
    DW_TAG_function_template = 0x4102,	/* For C++.  */
    DW_TAG_class_template = 0x4103,	/* For C++.  */
    DW_TAG_GNU_BINCL = 0x4104,
    DW_TAG_GNU_EINCL = 0x4105
  };

#define DW_TAG_lo_user	0x4080
#define DW_TAG_hi_user	0xffff

/* Flag that tells whether entry has a child or not.  */
#define DW_children_no   0
#define	DW_children_yes  1

/* Form names and codes.  */
enum dwarf_form
  {
    DW_FORM_addr = 0x01,
    DW_FORM_block2 = 0x03,
    DW_FORM_block4 = 0x04,
    DW_FORM_data2 = 0x05,
    DW_FORM_data4 = 0x06,
    DW_FORM_data8 = 0x07,
    DW_FORM_string = 0x08,
    DW_FORM_block = 0x09,
    DW_FORM_block1 = 0x0a,
    DW_FORM_data1 = 0x0b,
    DW_FORM_flag = 0x0c,
    DW_FORM_sdata = 0x0d,
    DW_FORM_strp = 0x0e,
    DW_FORM_udata = 0x0f,
    DW_FORM_ref_addr = 0x10,
    DW_FORM_ref1 = 0x11,
    DW_FORM_ref2 = 0x12,
    DW_FORM_ref4 = 0x13,
    DW_FORM_ref8 = 0x14,
    DW_FORM_ref_udata = 0x15,
    DW_FORM_indirect = 0x16
  };

/* Attribute names and codes.  */

enum dwarf_attribute
  {
    DW_AT_sibling = 0x01,
    DW_AT_location = 0x02,
    DW_AT_name = 0x03,
    DW_AT_ordering = 0x09,
    DW_AT_subscr_data = 0x0a,
    DW_AT_byte_size = 0x0b,
    DW_AT_bit_offset = 0x0c,
    DW_AT_bit_size = 0x0d,
    DW_AT_element_list = 0x0f,
    DW_AT_stmt_list = 0x10,
    DW_AT_low_pc = 0x11,
    DW_AT_high_pc = 0x12,
    DW_AT_language = 0x13,
    DW_AT_member = 0x14,
    DW_AT_discr = 0x15,
    DW_AT_discr_value = 0x16,
    DW_AT_visibility = 0x17,
    DW_AT_import = 0x18,
    DW_AT_string_length = 0x19,
    DW_AT_common_reference = 0x1a,
    DW_AT_comp_dir = 0x1b,
    DW_AT_const_value = 0x1c,
    DW_AT_containing_type = 0x1d,
    DW_AT_default_value = 0x1e,
    DW_AT_inline = 0x20,
    DW_AT_is_optional = 0x21,
    DW_AT_lower_bound = 0x22,
    DW_AT_producer = 0x25,
    DW_AT_prototyped = 0x27,
    DW_AT_return_addr = 0x2a,
    DW_AT_start_scope = 0x2c,
    DW_AT_stride_size = 0x2e,
    DW_AT_upper_bound = 0x2f,
    DW_AT_abstract_origin = 0x31,
    DW_AT_accessibility = 0x32,
    DW_AT_address_class = 0x33,
    DW_AT_artificial = 0x34,
    DW_AT_base_types = 0x35,
    DW_AT_calling_convention = 0x36,
    DW_AT_count = 0x37,
    DW_AT_data_member_location = 0x38,
    DW_AT_decl_column = 0x39,
    DW_AT_decl_file = 0x3a,
    DW_AT_decl_line = 0x3b,
    DW_AT_declaration = 0x3c,
    DW_AT_discr_list = 0x3d,
    DW_AT_encoding = 0x3e,
    DW_AT_external = 0x3f,
    DW_AT_frame_base = 0x40,
    DW_AT_friend = 0x41,
    DW_AT_identifier_case = 0x42,
    DW_AT_macro_info = 0x43,
    DW_AT_namelist_items = 0x44,
    DW_AT_priority = 0x45,
    DW_AT_segment = 0x46,
    DW_AT_specification = 0x47,
    DW_AT_static_link = 0x48,
    DW_AT_type = 0x49,
    DW_AT_use_location = 0x4a,
    DW_AT_variable_parameter = 0x4b,
    DW_AT_virtuality = 0x4c,
    DW_AT_vtable_elem_location = 0x4d,
    /* DWARF 3 values.  */
    DW_AT_allocated     = 0x4e,
    DW_AT_associated    = 0x4f,
    DW_AT_data_location = 0x50,
    DW_AT_stride        = 0x51,
    DW_AT_entry_pc      = 0x52,
    DW_AT_use_UTF8      = 0x53,
    DW_AT_extension     = 0x54,
    DW_AT_ranges        = 0x55,
    DW_AT_trampoline    = 0x56,
    DW_AT_call_column   = 0x57,
    DW_AT_call_file     = 0x58,
    DW_AT_call_line     = 0x59,
    /* SGI/MIPS Extensions.  */
    DW_AT_MIPS_fde = 0x2001,
    DW_AT_MIPS_loop_begin = 0x2002,
    DW_AT_MIPS_tail_loop_begin = 0x2003,
    DW_AT_MIPS_epilog_begin = 0x2004,
    DW_AT_MIPS_loop_unroll_factor = 0x2005,
    DW_AT_MIPS_software_pipeline_depth = 0x2006,
    DW_AT_MIPS_linkage_name = 0x2007,
    DW_AT_MIPS_stride = 0x2008,
    DW_AT_MIPS_abstract_name = 0x2009,
    DW_AT_MIPS_clone_origin = 0x200a,
    DW_AT_MIPS_has_inlines = 0x200b,
    /* GNU extensions.  */
    DW_AT_sf_names   = 0x2101,
    DW_AT_src_info   = 0x2102,
    DW_AT_mac_info   = 0x2103,
    DW_AT_src_coords = 0x2104,
    DW_AT_body_begin = 0x2105,
    DW_AT_body_end   = 0x2106,
    DW_AT_GNU_vector = 0x2107,
    /* VMS Extensions.  */
    DW_AT_VMS_rtnbeg_pd_address = 0x2201
  };

#define DW_AT_lo_user	0x2000	/* Implementation-defined range start.  */
#define DW_AT_hi_user	0x3ff0	/* Implementation-defined range end.  */

/* Location atom names and codes.  */
enum 
/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef OS_CPU_BSD_AARCH64_PAUTH_BSD_AARCH64_INLINE_HPP
#define OS_CPU_BSD_AARCH64_PAUTH_BSD_AARCH64_INLINE_HPP

inline bool pauth_ptr_is_raw(address ptr);

#ifdef __APPLE__
#include <ptrauth.h>
#endif

// Only the PAC instructions in the NOP space can be used. This ensures the
// binaries work on systems without PAC. Write these instructions using their
// alternate "hint" instructions to ensure older compilers can still be used.
// For Apple, use the provided interface as this may provide additional
// optimization.

#define XPACLRI "hint #0x7;"
#define PACIA1716 "hint #0x8;"
#define AUTIA1716 "hint #0xc;"

inline address pauth_strip_pointer(address ptr) {
#ifdef __APPLE__
  return ptrauth_strip(ptr, ptrauth_key_asib);
#else
  register address result __asm__("x30") = ptr;
  asm (XPACLRI : "+r"(result));
  return result;
#endif
}

inline address pauth_sign_return_address(address ret_addr, address sp) {
  if (UseROPProtection) {
    // A pointer cannot be double signed.
    guarantee(pauth_ptr_is_raw(ret_addr), "Return address is already signed");
#ifdef __APPLE__
    ret_addr = ptrauth_sign_unauthenticated(ret_addr, ptrauth_key_asib, sp);
#else
    register address r17 __asm("r17") = ret_addr;
    register address r16 __asm("r16") = sp;
    asm volatile (PACIA1716 : "+r"(r17) : "r"(r16));
    ret_addr = r17;
#endif
  }
  return ret_addr;
}

inline address pauth_authenticate_return_address(address ret_addr, address sp) {
  if (UseROPProtection) {
#ifdef __APPLE__
    ret_addr = ptrauth_auth_data(ret_addr, ptrauth_key_asib, sp);
#else
    register address r17 __asm("r17") = ret_addr;
    register address r16 __asm("r16") = sp;
    asm volatile (AUTIA1716 : "+r"(r17) : "r"(r16));
    ret_addr = r17;
#endif
    // Ensure that the pointer authenticated.
    guarantee(pauth_ptr_is_raw(ret_addr), "Return address did not authenticate");
  }
  return ret_addr;
}

#undef XPACLRI
#undef PACIA1716
#undef AUTIA1716

#endif // OS_CPU_BSD_AARCH64_PAUTH_BSD_AARCH64_INLINE_HPP


/**
 * @file miktex-first.h
 * @author Christian Schenk
 * @brief First include file
 *
 * @copyright Copyright © 2021-2022 Christian Schenk
 *
 * This file is free software; the copyright holder gives unlimited permission
 * to copy and/or distribute it, with or without modifications, as long as this
 * notice is preserved.
 */

#pragma once

#if defined(MIKTEX_WINDOWS)
#  define MIKTEX_UTF8_WRAP_ALL 1
#  include <miktex/utf8wrap.h>
#endif

#include "miktex-pdftex-version.h"
